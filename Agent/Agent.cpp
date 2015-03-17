/*
 * Agent.cpp
 *
 *  Created on: Nov 9, 2014
 *      Author: atlancer
 */

#include "Agent.h"
#include <sys/ioctl.h>
#include <net/if.h>

using namespace Agent;

void cleanUp() {
	close(controllerSockFd);
	close(packetSocketFd);
}

/*
 Main function.
 */
int main(int argc, char** argv) {
	string controllerFqdn;
	string port;
	string tunDev;

	atexit(cleanUp);
	cout << "+----------------------------+" << endl;
	cout << "|Batyr and Lucas's Host Agent|" << endl;
	cout << "|      Team 5, CS4516        |" << endl;
	cout << "+----------------------------+" << endl;

	if (argc == 6) {
		controllerFqdn.append(argv[1]);
		port.append(argv[2]);
		hostFqdn.append(argv[3]);
		preSharedSecret.append(argv[4]);
		tunDev.append(argv[5]);

		for (unsigned int i = 0; i < port.size(); i++) {
			if (!isdigit(port.at(i))) {
				cerr << "The port should contain only digits." << endl;
			}
		}
	} else {
		cerr
				<< "The number of the arguments is incorrect. Format: {controller FQDN} {port} {host FQDN} {pre-shared secret} {tunnel device name}"
				<< endl;
		exit(1);
	}

	bool waitingForDecision = true;
	string decision;
	cout << "Would you like to add a new tunnel '" + tunDev + "'? [y/n]"
			<< endl;
	while (waitingForDecision) {
		cin >> decision;
		if (decision == "y" || decision == "n") {
			waitingForDecision = false;
		} else {
			cout << "Please, type in y or n." << endl;
		}
	}

	if (decision == "y") {
		createNewTunnel(tunDev);
	}

	setupPacketSockets(tunDev);

	setupConnectionWithController(controllerFqdn, port);
	requestAuthentication();

	pthread_t mainElevThreadId;
	if (pthread_create(&mainElevThreadId, NULL, listenForElevation,
			(void *) &tunDev) != 0) {
		perror("pthread_create");
		exit(1);
	}
	cout << "-> Elevation service is active." << endl;

	cout << "-> Command listening service is active." << endl;
	listenForCommands();

	pthread_join(mainElevThreadId, NULL);

	return 0;
}

/*
 * Creates a new tunnel after the user's request.
 */
void createNewTunnel(string tunnelName) {
	Command cmdObj = Command();

	try {
		cmdObj.execute(
				"ip -6 tunnel add name " + tunnelName
						+ " remote ::1/128 local ::1/128");
		cmdObj.execute("ip -6 link set " + tunnelName + " up");
		cmdObj.execute("ip -6 route add default dev " + tunnelName);
	} catch (exception& e) {
		throw; //re-throw the exception
	}
}

/*
 * Implements the functionality of the main elevation thread
 * (handles the incoming requests for elevation from localhost).
 */
void* listenForElevation(void *arg) {
	string tunnelDevice = *(string *) arg;
	char buffer[BUF_SIZE];

	while (true) {
		int numbytes = recv(packetSocketFd, buffer, BUF_SIZE, 0);
		string receivedPacket = string(buffer, numbytes);
		elevationPackets[seqNum] = receivedPacket;
		string packetToSend;

		if (receivedPacket.size() > 1200) {
			packetToSend = receivedPacket.substr(0, 1200); //truncate the received packet
		} else {
			packetToSend = receivedPacket;
		}

		cout
				<< "Received elevation request from the tunnel. Sending the packet to the controller..."
				<< endl;

		//create a transferable message for the controller
		ProtocolMessage msgObj(seqNum++, 0, 0, false, true, false,
				ProtocolMessage::NoType, ProtocolMessage::NoCommand,
				packetToSend);
		string transferableMessage = msgObj.convertToTransferableMessage();
		if ((numbytes = send(controllerSockFd, transferableMessage.c_str(),
				transferableMessage.size(), 0)) == -1) {
			perror("Request authentication send()");
			exit(1);
		}
	}
}

/*
 * Setup packet sockets.
 */
void setupPacketSockets(string tunnelDevice) {
	packetSocketFd = createPacketSocket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL),
			tunnelDevice);
	//create a raw socket for sending out a diagram as well
	rawSocketFd = createPacketSocket(AF_INET6, SOCK_RAW, IPPROTO_IPV6, "");
}

/*
 * Creates a new socket of the specified, binds it to the specific device and returns sockFd of that socket.
 */
int createPacketSocket(int domain, int socketType, int protocol, string deviceToBindTo) {
	int sockFd;

	//create a raw socket that will receive packets from the tunnel
	if ((sockFd = socket(domain, socketType, protocol)) < 0) {
		perror("socket() failed ");
		exit(-1);
	}

	if (!deviceToBindTo.empty()) {
		//bind the packet socket to the tunnel device
		struct sockaddr_ll link;
		memset(&link, 0, sizeof(struct sockaddr_ll));
		link.sll_ifindex = if_nametoindex(deviceToBindTo.c_str());
		if (link.sll_ifindex == 0) {
			perror("if_nametoindex() failed: ");
			exit(-1);
		}
		link.sll_protocol = protocol;
		link.sll_family = AF_PACKET;

		if (bind(sockFd, (struct sockaddr *) &link, sizeof(struct sockaddr_ll))
				< 0) {
			perror("bind() failed: ");
			exit(-1);
		}
	}

	return sockFd;
}

/*
 * Listens for the commands from the host and executes them.
 */
void listenForCommands() {
	string msg;
	string responseBody;

	while (true) {
		msg = getNextMessageFromController();
		ProtocolMessage *msgObj = new ProtocolMessage(msg);

		if (msgObj->ack != 0) { //it's the response
			handleElevationResponse(msgObj);
		}
		if (msgObj->seq != 0) { //it's the request
			if (msgObj->seq != lastProcessedSeqNum + 1) {
				requestMessageBuffer[msgObj->seq] = msgObj;
			} else {
				processCommand(msgObj);
				lastProcessedSeqNum++;
				while (requestMessageBuffer.count(lastProcessedSeqNum + 1)) {
					processCommand(
							requestMessageBuffer[lastProcessedSeqNum + 1]);
					requestMessageBuffer.erase(lastProcessedSeqNum + 1);
					lastProcessedSeqNum++;
				}
			}
		}
	}
}

/*
 * Process elevation response.
 */
void handleElevationResponse(ProtocolMessage* elevRespMsgObjRef) {
	if (!elevRespMsgObjRef->isElev) {
		cerr
				<< "Received the unknown (not elevation) response from the controller."
				<< endl;
		return;
	}

	if (elevRespMsgObjRef->command == ProtocolMessage::Drop) {
		cout << "Received elevation response back. Dropping packet..." << endl;
		//drop the message
		elevationPackets.erase(elevRespMsgObjRef->ack);
	} else if (elevRespMsgObjRef->command == ProtocolMessage::Retry) {
		cout << "Received elevation response back. Attempting to resend..."
				<< endl;
		//send a message using a raw socket
		string dest_addr = elevRespMsgObjRef->body;

		struct sockaddr_in6 addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin6_family = AF_INET6;
		if (inet_pton(AF_INET6, dest_addr.c_str(), &addr.sin6_addr) != 1)
			perror("inet_pton()");

		const char *originalPacket = elevationPackets[elevRespMsgObjRef->ack].c_str();
		int originalPacketLen = elevationPackets[elevRespMsgObjRef->ack].size();

		if (sendto(rawSocketFd, originalPacket, originalPacketLen, 0,
				(struct sockaddr*) &addr, sizeof(addr)) < 0) {
			perror("Failed to resend the packet to the original destination");
		}

		elevationPackets.erase(elevRespMsgObjRef->ack);
	} else {
		cerr << "The elevation response received from the controller "
				"didn't contain a recognizable elevation command." << endl;
	}
}

/*
 * Processes the command.
 */
void processCommand(ProtocolMessage* commandMsgObjRef) {
	int numbytes;
	enum ProtocolMessage::type type = commandMsgObjRef->type;
	enum ProtocolMessage::command command = commandMsgObjRef->command;
	istringstream iss(commandMsgObjRef->body);
	string address;
	string interface;
	string chain;
	string position;
	string rule;
	string lladr;
	string device;
	IPv6 ipHandler = IPv6();
	NAT natHandler = NAT();
	Firewall firewallHandler = Firewall();
	NeighborDiscoveryCache neighborHandler = NeighborDiscoveryCache();
	Table rtHandler = Table();
	Rule ruleHandler = Rule();
	Route routeHandler = Route();

	try {
		switch (type) {
		case ProtocolMessage::IP:
			iss >> address;
			iss >> interface;
			if (command == ProtocolMessage::Show) {
				commandMsgObjRef->body = ipHandler.show();
			} else if (command == ProtocolMessage::Add) {
				ipHandler.add(address, interface);
				commandMsgObjRef->body = "The ip address " + address
						+ " was successfully added to interface " + interface
						+ ".\n";
			} else if (command == ProtocolMessage::Remove) {
				ipHandler.remove(address, interface);
				commandMsgObjRef->body = "The ip address " + address
						+ " was successfully removed from interface "
						+ interface + ".\n";
			} else {
				commandMsgObjRef->isError = true;
				commandMsgObjRef->body =
						"The type 'ip' should have a command.\n";
			}
			break;
			//<===============================================================================>
		case ProtocolMessage::NAT:
			if (command == ProtocolMessage::Show) {
				commandMsgObjRef->body = natHandler.show();
			} else if (command == ProtocolMessage::Add) {
				iss >> chain;
				iss >> position;

				//trim argument string
				iss >> rule;
				string temp;
				while (iss >> temp) {
					rule += " " + temp;
				}

				natHandler.add(chain, position, rule);
				commandMsgObjRef->body = "The nat rule " + rule
						+ " was successfully added to chain " + chain
						+ " at position " + position + ".\n";
			} else if (command == ProtocolMessage::Remove) {
				iss >> chain;

				//trim argument string
				iss >> rule;
				string temp;
				while (iss >> temp) {
					rule += " " + temp;
				}

				natHandler.remove(chain, rule);
				commandMsgObjRef->body = "The nat rule " + rule
						+ " was successfully removed from chain " + chain
						+ ".\n";
			} else {
				commandMsgObjRef->isError = true;
				commandMsgObjRef->body =
						"The type 'nat' should have a command.\n";
			}
			break;
			//<===============================================================================>
		case ProtocolMessage::Firewall:
			if (command == ProtocolMessage::Show) {
				commandMsgObjRef->body = firewallHandler.show();
			} else if (command == ProtocolMessage::Add) {
				iss >> chain;
				iss >> position;

				//trim argument string
				iss >> rule;
				string temp;
				while (iss >> temp) {
					rule += " " + temp;
				}

				firewallHandler.add(chain, position, rule);
				commandMsgObjRef->body = "The firewall rule " + rule
						+ " was successfully added to chain " + chain
						+ " at position " + position + ".\n";
			} else if (command == ProtocolMessage::Remove) {
				iss >> chain;

				//trim argument string
				iss >> rule;
				string temp;
				while (iss >> temp) {
					rule += " " + temp;
				}

				firewallHandler.remove(chain, rule);
				commandMsgObjRef->body = "The firewall rule " + rule
						+ " was successfully removed from chain " + chain
						+ ".\n";
			} else {
				commandMsgObjRef->isError = true;
				commandMsgObjRef->body =
						"The type 'firewall' should have a command.\n";
			}
			break;
			//<===============================================================================>
		case ProtocolMessage::Neighbor:
			iss >> address;
			iss >> lladr;
			iss >> device;

			if (command == ProtocolMessage::Show) {
				commandMsgObjRef->body = neighborHandler.show();
			} else if (command == ProtocolMessage::Add) {
				neighborHandler.add(address, lladr, device);
				commandMsgObjRef->body = "The neighbor discovery cache entry '"
						+ commandMsgObjRef->body
						+ "' was successfully added.\n";
			} else if (command == ProtocolMessage::Remove) {
				neighborHandler.remove(address, lladr, device);
				commandMsgObjRef->body = "The neighbor discovery cache entry '"
						+ commandMsgObjRef->body
						+ "' was successfully removed.\n";
			} else {
				commandMsgObjRef->isError = true;
				commandMsgObjRef->body =
						"The type 'neighbor' should have a command.\n";
			}
			break;
			//<===============================================================================>
		case ProtocolMessage::RoutingTable:
			if (command == ProtocolMessage::Show) {
				commandMsgObjRef->body = rtHandler.show();
			} else if (command == ProtocolMessage::Add) {
				rtHandler.add(commandMsgObjRef->body);
				commandMsgObjRef->body = "The routing table '"
						+ commandMsgObjRef->body
						+ "' was successfully added.\n";
			} else if (command == ProtocolMessage::Remove) {
				rtHandler.remove(commandMsgObjRef->body);
				commandMsgObjRef->body = "The routing table '"
						+ commandMsgObjRef->body
						+ "' was successfully removed.\n";
			} else {
				commandMsgObjRef->isError = true;
				commandMsgObjRef->body =
						"The type 'routingTable' should have a command.\n";
			}
			break;
			//<===============================================================================>
		case ProtocolMessage::Rule:
			if (command == ProtocolMessage::Show) {
				commandMsgObjRef->body = ruleHandler.show();
			} else if (command == ProtocolMessage::Add) {
				ruleHandler.add(commandMsgObjRef->body);
				commandMsgObjRef->body = "The rule '" + commandMsgObjRef->body
						+ "' was successfully added.\n";
			} else if (command == ProtocolMessage::Remove) {
				ruleHandler.remove(commandMsgObjRef->body);
				commandMsgObjRef->body = "The rule '" + commandMsgObjRef->body
						+ "' was successfully removed.\n";
			} else {
				commandMsgObjRef->isError = true;
				commandMsgObjRef->body =
						"The type 'rule' should have a command.\n";
			}
			break;
			//<===============================================================================>
		case ProtocolMessage::Route:
			if (command == ProtocolMessage::Show) {
				commandMsgObjRef->body = routeHandler.show(
						commandMsgObjRef->body);
			} else if (command == ProtocolMessage::Add) {
				routeHandler.add(commandMsgObjRef->body);
				commandMsgObjRef->body = "The route '" + commandMsgObjRef->body
						+ "' was successfully added.\n";
			} else if (command == ProtocolMessage::Remove) {
				routeHandler.remove(commandMsgObjRef->body);
				commandMsgObjRef->body = "The route '" + commandMsgObjRef->body
						+ "' was successfully removed.\n";
			} else {
				commandMsgObjRef->isError = true;
				commandMsgObjRef->body =
						"The type 'route' should have a command.\n";
			}
			break;
		default:
			commandMsgObjRef->isError = true;
			commandMsgObjRef->body = "The type 'ip' should have a command.\n";
		}
	} catch (exception& exception) {
		commandMsgObjRef->isError = true;
		commandMsgObjRef->body = exception.what();
	}

	commandMsgObjRef->ack = commandMsgObjRef->seq; //swap ack and seq to convert the request into the response
	commandMsgObjRef->seq = 0;
	string transferableMessage =
			commandMsgObjRef->convertToTransferableMessage();

	if ((numbytes = send(controllerSockFd, transferableMessage.c_str(),
			transferableMessage.size(), 0)) == -1) {
		perror("Request authentication send()");
		exit(1);
	}

	delete commandMsgObjRef;
}

/*
 * Gets the next message from the TCP buffer.
 */
string getNextMessageFromController() {
	char buf[MAXDATASIZE];
	int numbytes;
	unsigned int msgLength;
	string cumulativeBuffer;
	string msg;

	if (!receiveBuffer.empty()) {
		cumulativeBuffer.append(receiveBuffer.c_str(), receiveBuffer.size());
		receiveBuffer.clear();
	}

	while (cumulativeBuffer.size() < 4) {
		if ((numbytes = recv(controllerSockFd, buf, MAXDATASIZE - 1, 0))
				== -1) {
			perror("recv");
			exit(1);
		}

		if (numbytes == 0) {
			throw runtime_error("The connection has been closed.\n");
		}

		cumulativeBuffer.append(buf, numbytes);
	}

	msgLength = (cumulativeBuffer.c_str()[0] << 24)
			+ (cumulativeBuffer.c_str()[1] << 16)
			+ (cumulativeBuffer.c_str()[2] << 8)
			+ (cumulativeBuffer.c_str()[3]);

	msgLength = ntohl(msgLength);

	while (msgLength > cumulativeBuffer.size()) {
		if ((numbytes = recv(controllerSockFd, buf, MAXDATASIZE - 1, 0))
				== -1) {
			perror("recv");
			exit(1);
		}

		if (numbytes == 0) {
			throw runtime_error("The connection has been closed.\n");
		}

		cumulativeBuffer.append(buf, numbytes);
	}

	msg = cumulativeBuffer.substr(0, msgLength);
	//put the rest of the message into the buffer for that host
	receiveBuffer = cumulativeBuffer.substr(msgLength,
			cumulativeBuffer.size() - msgLength);

	return msg;
}

/*
 * Sets up the connection with the controller.
 */
void setupConnectionWithController(string controllerFqdn, string port) {
	struct addrinfo hints, *servinfo, *p;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(controllerFqdn.c_str(), port.c_str(), &hints,
			&servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		exit(1);
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((controllerSockFd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(controllerSockFd, p->ai_addr, p->ai_addrlen) == -1) {
			close(controllerSockFd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		exit(1);
	}
}

/*
 * Sends the authentication request to the controller and checks the response received.
 */
void requestAuthentication() {
	int numbytes;

	ProtocolMessage msgObj(seqNum++, 0, 0, true, false, false,
			ProtocolMessage::NoType, ProtocolMessage::NoCommand,
			hostFqdn + "\n" + preSharedSecret + "\n");
	string transferableMessage = msgObj.convertToTransferableMessage();

	if ((numbytes = send(controllerSockFd, transferableMessage.c_str(),
			transferableMessage.size(), 0)) == -1) {
		perror("Request authentication send()");
		exit(1);
	}

	string response = getNextMessageFromController();

	ProtocolMessage responseObj(response);
	if (responseObj.isError) {
		cerr
				<< "The host refused to authenticate returning the following error:\n"
				<< responseObj.body << endl;
		cerr << "Exiting..." << endl;
		exit(1);
	}
}
