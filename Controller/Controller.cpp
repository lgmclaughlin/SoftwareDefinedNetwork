/*
 * Controller.cpp
 *
 *  Created on: Nov 9, 2014
 *      Author: atlancer
 */

#include "Controller.h"

using namespace std;
using namespace Controller;

/*
 Main function.
 */
int main(int argc, char **argv) {
	atexit(cleanUp);
	cout << "+----------------------------+" << endl;
	cout << "|Batyr and Lucas's Controller|" << endl;
	cout << "|      Team 5, CS4516        |" << endl;
	cout << "+----------------------------+" << endl;

	if (argc == 3) {
		populateHostDb(hostDb, argv[1]);
		cout << "-> Host database loaded." << endl;
		populateConfigDb(configDb, argv[2]);
		cout << "-> Configuration loaded." << endl << endl;
	} else {
		cerr
				<< "The program takes 2 arguments. Format: {host info file} {configuration file}."
				<< endl;
		exit(-1);
	}

	pthread_t cmdThreadId;

	if (pthread_create(&cmdThreadId, NULL, runCmd, (void *) 0) != 0) {
		perror("pthread_create");
		exit(1);
	}

	runListener();
	pthread_join(cmdThreadId, NULL);

	return 0;
}

/*
 * Runs the connection listener that listens and accepts the incoming connections.
 * The function also authenticates the host before allowing to connect.
 */
void runListener() {
	struct addrinfo hints, *servinfo, *pointer;
	struct sigaction sigstruct;
	int newsockfd, ecode;
	struct sockaddr client_addr;
	socklen_t client_addr_size;
	int yes = 1;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((ecode = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "Error occurred in getaddrinfo(): %s\n",
				gai_strerror(ecode));
		exit(1);
	}

	for (pointer = servinfo; pointer != NULL; pointer = pointer->ai_next) {
		if ((sockfd = socket(pointer->ai_family, pointer->ai_socktype,
				pointer->ai_protocol)) == -1) {
			perror("Error occurred in socket()");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
				== -1) {
			perror("Error occurred in setsockopt()");
			exit(1);
		}

		if (bind(sockfd, pointer->ai_addr, pointer->ai_addrlen) == -1) {
			close(sockfd);
			perror("Error occurred in bind()");
			continue;
		}

		break;
	}

	if (pointer == NULL) {
		close(sockfd);
		fprintf(stderr, "Failed to bind the port to the socket!");
		exit(1);
	}

	//we're done with this structure
	freeaddrinfo(servinfo);

	if (listen(sockfd, BACKLOG) == -1) {
		close(sockfd);
		perror("Error occurred in listen()");
		exit(1);
	}

	//set up the sigaction to catch SIGTERM in order to terminate the program
	sigstruct.sa_handler = sigterm_handler;
	if (sigaction(SIGTERM, &sigstruct, NULL) == -1) {
		close(sockfd);
		perror("Error occured in sigaction()");
		exit(1);
	}

	//set up the sigaction to catch the interrupts when the child procs are done
	sigstruct.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sigstruct.sa_mask);
	sigstruct.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sigstruct, NULL) == -1) {
		close(sockfd);
		perror("Error occured in sigaction()");
		exit(1);
	}

	while (!terminateExec) {
		client_addr_size = sizeof client_addr;

		if ((newsockfd = accept(sockfd, (struct sockaddr*) &client_addr,
				&client_addr_size)) == -1) {
			perror("Accept() <normal behavior if interrupt was received>");
			continue; //get the next client
		}

		string *hostNamePtr = new string();
		int ackNum;
		ProtocolMessage authenticationResponse(0, 0, 0, true, false, false,
				ProtocolMessage::NoType, ProtocolMessage::NoCommand, ""); //create a stub authentication response
		string transferableMsg;

		try {
			hostNamePtr->append(authenticateHost(newsockfd, ackNum)); //create  a pointer to the value returned from authenticateHost
		} catch (exception& e) {
			string errorDesc(e.what());
			authenticationResponse.body = errorDesc;
			authenticationResponse.isError = true;
		}

		authenticationResponse.ack = ackNum;
		transferableMsg = authenticationResponse.convertToTransferableMessage();

		if (send(newsockfd, transferableMsg.c_str(), transferableMsg.size(), 0)
				== -1)
			perror("send");

		if (authenticationResponse.isError) {
			delete hostNamePtr; //clean the memory before proceeding to the next host
			continue;
		}

		//get the lock to update the sockfd and isConnected simultaneously
		pthread_rwlock_wrlock(&hostDb[*hostNamePtr].rwlock);
		hostDb[*hostNamePtr].isConnected = true;
		hostDb[*hostNamePtr].sockfd = newsockfd;
		pthread_rwlock_unlock(&hostDb[*hostNamePtr].rwlock);

		if (pthread_create(&hostDb[*hostNamePtr].threadId, NULL,
				processConnectedHost, (void *) hostNamePtr) != 0) {
			perror("pthread_create");
			exit(1);
		}
	}

	close(sockfd);
}

/*
 * Process a request for connection from a host
 */
void* processConnectedHost(void *arg) {
	string hostName = *(string *) arg;
	delete (string *) arg; //we are done with this string

	while (true) {
		try {
			ProtocolMessage* msgObj = new ProtocolMessage(
					getNextMessageFromHost(hostName));
			if (msgObj->ack != 0) { //it's the response
				if (msgObj->ack != hostDb[hostName].lastProcessedAckNum + 1) {
					hostDb[hostName].responseMessageBuffer[msgObj->ack] =
							msgObj;
				} else {
					processCommandResponse(msgObj, hostName);
					hostDb[hostName].lastProcessedAckNum++;
					while (hostDb[hostName].responseMessageBuffer.count(
							hostDb[hostName].lastProcessedAckNum + 1)) {
						processCommandResponse(
								hostDb[hostName].responseMessageBuffer[hostDb[hostName].lastProcessedAckNum
										+ 1], hostName);
						hostDb[hostName].responseMessageBuffer.erase(
								hostDb[hostName].lastProcessedAckNum + 1);
						hostDb[hostName].lastProcessedAckNum++;
					}
				}
			}
			if (msgObj->seq != 0) { //it's the request
				if (msgObj->isElev) {
					cout
							<< "Received an elevation request packet from "
									+ hostName + "." << endl;
					processElevationRequest(msgObj, hostName);
					cout << "Done handling elevation." << endl;
				} else {
					cerr
							<< "Received a request from " + hostName
									+ " that was not elevation." << endl;
					continue;
				}
			}
		} catch (exception& e) {
			cout << "The host " + hostName + " closed the connection." << endl;
			resetHostInfo(hostName);
			break;
		}
	}
}

/*
 * Processes the command response object passed by reference.
 */
void processCommandResponse(ProtocolMessage* commandResponseObjRef,
		string hostName) {
	cout << commandResponseObjRef->body << endl;
	delete commandResponseObjRef;
}

/*
 * Process elevation request.
 */
void processElevationRequest(ProtocolMessage *msgObjRef, string hostName) {
	string fullIPv6Header = msgObjRef->body.substr(0, 40);
	string sourceRaw = fullIPv6Header.substr(8, 16);
	string destRaw = fullIPv6Header.substr(24, 16);

	cout << "Extracting IP header from elevation request payload." << endl;
	char addrBuffer[128]; //including
	inet_ntop(AF_INET6, sourceRaw.c_str(), addrBuffer, 128);
	string sourceAddr(addrBuffer);
	inet_ntop(AF_INET6, destRaw.c_str(), addrBuffer, 128);
	string destAddr(addrBuffer);

	struct configDbKey key;
	key.source = sourceAddr;
	key.destination = destAddr;
	key.hostName = hostName;
	vector<string> commandVector = getValueFromConfigDb(key);
	if (commandVector.empty()) {
		cout
				<< "The record matching host<" + hostName + "> src<"
						+ sourceAddr + "> , dest<" + destAddr
						+ "> was not found in Config Db." << endl;
		//since the record was not found, command the host agent to drop the packet
		sendElevResp(hostName, "drop", destAddr, msgObjRef->seq);
	}

	for (int i = 0; i < commandVector.size(); i++) {
		string firstWord;
		istringstream iss(commandVector.at(i));
		iss >> firstWord;

		if (firstWord != "retry" && firstWord != "drop") {
			processCmd(commandVector[i]);
		} else {
			if (firstWord == "retry" || firstWord == "drop") {
				sendElevResp(hostName, firstWord, destAddr, msgObjRef->seq);
			}
		}
		firstWord.clear();
	}
}

/*
 * Sends the elevation request back to the host specified by the host argument.
 */
void sendElevResp(string host, string command, string destination,
		int elevReqId) {
	enum ProtocolMessage::command cmdNum = ProtocolMessage::getEnumCommand(
			command);
	ProtocolMessage msgObj(0, elevReqId, 0, false, true, false,
			ProtocolMessage::NoType, cmdNum, destination);
	string msgToSend = msgObj.convertToTransferableMessage();
	cout
			<< "Done sending commands. Sending an elevation response to the host agent..."
			<< endl;
	if (send(hostDb[host].sockfd, msgToSend.c_str(), msgToSend.size(), 0) == -1)
		perror("send");
}

/*
 * Authenticates the host and passes its name as a return value.
 */
string authenticateHost(int sockfd, int &ackNum) {
	char buf[MAXDATASIZE];
	int numbytes;
	int msgLength;
	string cumulativeBuffer;
	string msg;

	while (cumulativeBuffer.size() < 4) {
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
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
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
			perror("recv");
			exit(1);
		}

		if (numbytes == 0) {
			throw runtime_error("The connection has been closed.\n");
		}

		cumulativeBuffer.append(buf, numbytes);
	}

	//get the entire message
	msg = cumulativeBuffer.substr(0, msgLength);

	ProtocolMessage messageObject(msg);
	if (!messageObject.isAuthentication) {
		throw runtime_error("Unauthenticated host tried to send a request.\n");
	}

	//check the identity and extract the hostName
	istringstream iss(messageObject.body);
	string hostName;
	string preSharedSecret;
	if (getline(iss, hostName) && getline(iss, preSharedSecret)) {
		if (hostDb.count(hostName) == 0) { //the host doesn't exist
			throw runtime_error("The host was not recognized.\n");
		}
		if (hostDb[hostName].isConnected) {
			throw runtime_error("The host is already connected.\n");
		}
		if (hostDb[hostName].preSharedSecret != preSharedSecret) {
			throw runtime_error("The preSharedSecret is not valid.\n");
		}
	} else {
		throw runtime_error(
				"There should be two arguments in the message body for the authentication request.\n");
	}

	string copyTest;

	//put the rest of the message into the buffer for that host
	hostDb[hostName].receiveBuffer = cumulativeBuffer.substr(msgLength,
			cumulativeBuffer.size() - msgLength);

	ackNum = messageObject.seq;

	cout
			<< "The host " + hostName
					+ " was authenticated and has been connected." << endl;

	return hostName;
}

/*
 * Gets the next message from the host.
 */
string getNextMessageFromHost(string hostName) {
	char buf[MAXDATASIZE];
	int numbytes;
	unsigned int msgLength;
	int sockfd = hostDb[hostName].sockfd;
	string cumulativeBuffer;
	string msg;

	if (!hostDb[hostName].receiveBuffer.empty()) {
		cumulativeBuffer.append(hostDb[hostName].receiveBuffer.c_str(),
				hostDb[hostName].receiveBuffer.size());
		hostDb[hostName].receiveBuffer.clear();
	}

	while (cumulativeBuffer.size() < 4) {
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
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
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
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
	hostDb[hostName].receiveBuffer = cumulativeBuffer.substr(msgLength,
			cumulativeBuffer.size() - msgLength);

	return msg;
}

/*
 * Read in a console command
 */
void *runCmd(void *arg) {
	string input;

	cout << "SDN has been primed and is ready for action." << endl;
	cout << "Enter commands below:" << endl;

	while (true) {
		getline(cin, input);
		processCmd(input);
	}
}

/*
 * Process a command, checking if it is valid and calling send if it is
 */
void processCmd(string input) {
	string cmd;
	string type;
	string host;
	string argument;
	int wordCount;
	wordCount = countWordsInString(input);

	istringstream iss(input);

	if (wordCount >= 3) {
		iss >> cmd;
		iss >> type;
		iss >> host;

		//trim argument string
		iss >> argument;
		string temp;
		while (iss >> temp) {
			argument += " " + temp;
		}

		if (!vectorContainsStr(ProtocolMessage::getValidCommands(), cmd)) {
			cerr << "Command " + cmd + " is not recognized." << endl;
			return;
		}

		if (!vectorContainsStr(ProtocolMessage::getValidTypes(), type)) {
			cerr << "Type of command " + type + " is not recognized." << endl;
			return;
		}

		if (hostDb.count(host) == 0) {
			cerr << "Host " + host + " doesn't exist in the database." << endl;
			return;
		}

		if (!hostDb[host].isConnected) {
			cerr << "Host " + host + " is not currently connected." << endl;
			return;
		}

		sendCommand(type, cmd, argument, host);
	} else if (wordCount == 2) {
		iss >> cmd;
		iss >> argument;

		vector<string> cmdLines;
		if (!cmd.compare("file")) {
			try {
				readCommandsIntoVector(cmdLines, argument.c_str());
				for (unsigned int i = 0; i < cmdLines.size(); i++) {
					processCmd(cmdLines.at(i));
				}
			} catch (exception& e) {
				cerr << e.what() << endl;
			}
		} else {
			printInvalidCmdError();
		}
	} else if (wordCount == 1) {
		iss >> cmd;

		if (!cmd.compare("exit")) {
			exit(0);
		} else {
			printInvalidCmdError();
		}
	}
}

/*
 * Sends the command to the host specified by the host argument.
 */
void sendCommand(string type, string command, string argument, string host) {
	enum ProtocolMessage::type typeNum = ProtocolMessage::getEnumType(type);
	enum ProtocolMessage::command cmdNum = ProtocolMessage::getEnumCommand(
			command);

	pthread_rwlock_wrlock(&hostDb[host].rwlock);
	ProtocolMessage msgObj(hostDb[host].seqNum++, 0, 0, false, false, false,
			typeNum, cmdNum, argument);
	pthread_rwlock_unlock(&hostDb[host].rwlock);
	string msgToSend = msgObj.convertToTransferableMessage();
	if (send(hostDb[host].sockfd, msgToSend.c_str(), msgToSend.size(), 0) == -1)
		perror("send");
}

/*
 * Count the number of words in a std string
 */
unsigned int countWordsInString(string const& str) {
	stringstream stream(str);
	return distance(istream_iterator<string>(stream),
			istream_iterator<string>());
}

/*
 * Prints the error message that indicates proper commands.
 */
void printInvalidCmdError() {
	cerr << "Command not recognized.\n"
			"Format: {command} {type} {host} [arguments]" << endl;
}

/*
 * Handles SIGCHLD interrupt.
 */
void sigchld_handler(int signal) {
	while (waitpid(-1, NULL, WNOHANG) > 0)
		;
}

/*
 * Handles SIGTERM interrupt.
 */
void sigterm_handler(int signal) {
	printf("Received SIGTERM, the program is being terminated...\n");
	terminateExec = 1;
}

/*
 * Populates the hostDb passed by reference from the text file specified by the input.
 */
void populateHostDb(map<string, hostInfo> &db, char* fileToReadFrom) {
	string line;
	ifstream file(fileToReadFrom);

	if (!file.is_open()) {
		cerr << "Unable to open the host info file." << endl;
		exit(-1);
	}

	while (getline(file, line)) {
		istringstream iss(line);
		string host;
		string secret;

		if (!(iss >> host) || !(iss >> secret)) {
			cerr
					<< "The database file for the hosts is not structured properly!\n"
							"The first column should indicate the host name and the second one should indicate the pre-shared secret.\n"
							"Columns should be separated with the whitespace character. Rows should be separated with the newline character."
					<< endl;
		}

		db[host] = hostInfo(secret);
	}

	file.close();
}

/*
 * Populates the configuration database from the configuration file.
 */
void populateConfigDb(multimap<configDbKey, vector<string> > &db,
		char* fileToReadFrom) {
	string line;
	ifstream file(fileToReadFrom);
	string sourceParameterName = "source=";
	string destParameterName = "dest=";
	string actionParameterName = "action=";
	string commandString;
	struct configDbKey key = configDbKey();
	vector<string> value;
	int currentPosition;
	int startingPosition;
	int endingPosition;

	if (!file.is_open()) {
		cerr << "Unable to open the configuration file." << endl;
		exit(-1);
	}

	while (getline(file, line)) {
		//cout << " ---- " << endl;
		if ((currentPosition = line.find(" ")) != string::npos) {
			if (currentPosition == 0) {
				cerr
						<< "The beginning of the record should start from the name of the host, not the white space."
						<< endl;
				exit(-1);
			} else {
				key.hostName = line.substr(0, currentPosition);
			}
		} else {
			cerr << "The parameters should be delimited with white spaces."
					<< endl;
			exit(-1);
		}
		//cout << key.hostName << endl;
		if ((currentPosition = line.find(sourceParameterName))
				!= string::npos) { //if the parameter was found
			startingPosition = currentPosition + sourceParameterName.length();
			endingPosition = line.find(",", currentPosition);
			key.source = line.substr(startingPosition,
					endingPosition - startingPosition);
		} else {
			key.source = "*";
		}
		//cout << key.source << endl;
		if ((currentPosition = line.find(destParameterName)) != string::npos) { //if the parameter was found
			startingPosition = currentPosition + destParameterName.length();
			endingPosition = line.find(",", startingPosition);
			key.destination = line.substr(startingPosition,
					endingPosition - startingPosition);
		} else {
			key.destination = "*";
		}
		//cout << key.destination << endl;
		if ((currentPosition = line.find(actionParameterName))
				!= string::npos) {
			startingPosition = line.find("(", currentPosition);
			endingPosition = line.find(")", startingPosition);
			commandString = line.substr(startingPosition + 1,
					endingPosition - startingPosition - 1);

			value = extractCommands(commandString, " ; ");
		} else {
			cerr
					<< "Some of the records it the file doesn't contain action parameter."
					<< endl;
			exit(-1);
		}

		configDb.insert(pair<configDbKey, vector<string> >(key, value));
	}

	file.close();
}

/*
 * Gets value from configDb.
 */
vector<string> getValueFromConfigDb(configDbKey key) {
	vector<string> result;

	typedef multimap<struct configDbKey, vector<string> >::iterator it_type;
	struct configDbKey dbKey;
	for (it_type iterator = configDb.begin(); iterator != configDb.end();
			iterator++) {
		dbKey = iterator->first;
		if (key.match(dbKey)) {
			cout
					<< "Found Record Key => Host: " + dbKey.hostName + "; Src: "
							+ dbKey.source + "; Dest: " + dbKey.destination
							+ ";" << endl;
			result = iterator->second;
			break;
		}
	}

	return result;
}

/*
 * Prints the configuration map that was read from the configuration file.
 */
void printConfigMap() {
	typedef multimap<struct configDbKey, vector<string> >::iterator it_type;
	struct configDbKey key;
	for (it_type iterator = configDb.begin(); iterator != configDb.end();
			iterator++) {
		// iterator->first = key
		// iterator->second = value
		key = iterator->first;
		cout
				<< "Host:" + key.hostName + "; Src:" + key.source + "; Dest:"
						+ key.destination + ";" << endl;
	}
}

/*
 * Extract commands separated by delimiter.
 */
vector<string> extractCommands(string commandString, string delimiter) {
	vector<string> commandVector;
	int lastSearchPosition = 0;
	int delimiterPosition = 0;

	delimiterPosition = commandString.find(delimiter);
	while (delimiterPosition != string::npos) {
		commandVector.push_back(
				commandString.substr(lastSearchPosition,
						delimiterPosition - lastSearchPosition));
		//cout << commandString.substr(lastSearchPosition,
		//		delimiterPosition - lastSearchPosition) << endl;
		lastSearchPosition = delimiterPosition + delimiter.length();
		delimiterPosition = commandString.find(delimiter,
				lastSearchPosition);
	}

	commandVector.push_back(commandString.substr(lastSearchPosition, delimiterPosition - lastSearchPosition));

//	cout << "Vector: " << endl;
//	for (int i = 0; i < commandVector.size(); i++) {
//		cout << commandVector.at(i) << endl;
//	}

	return commandVector;
}

/*
 * Read commands from a file into a vector line by line for execution
 */
void readCommandsIntoVector(vector<string> &cmds, const char* fileToReadFrom) {
	string line;
	ifstream file(fileToReadFrom);

	if (file.is_open()) {
		while (getline(file, line)) {
			cmds.push_back(line);
		}

		file.close();
	} else {
		throw runtime_error("Unable to open file.\n");
	}
}

/*
 * Check if a given vector contains the given string
 */
bool vectorContainsStr(const vector<string> search, string find) {
	for (unsigned int i = 0; i < search.size(); i++) {
		if (search.at(i) == find) {
			return true;
		}
	}

	return false;
}

/*
 * Destroys message buffers.
 */
void destroyMessageBuffers(string hostName) {
	map<int, ProtocolMessage *>::iterator iterator;

	for (iterator = hostDb[hostName].requestMessageBuffer.begin();
			iterator != hostDb[hostName].requestMessageBuffer.end();
			++iterator) {
		delete iterator->second;
	}

	for (iterator = hostDb[hostName].responseMessageBuffer.begin();
			iterator != hostDb[hostName].responseMessageBuffer.end();
			++iterator) {
		delete iterator->second;
	}
}

/*
 * Cleans everything and closes the connections with the hosts.
 */
void cleanUp() {
	map<string, hostInfo>::iterator iterator;

	for (iterator = hostDb.begin(); iterator != hostDb.end(); ++iterator) {
		destroyMessageBuffers(iterator->first);
		if (iterator->second.isConnected) {
			cout << "Closing the connection to " + iterator->first + "."
					<< endl;
			close(iterator->second.sockfd);
		}
		pthread_rwlock_destroy(&iterator->second.rwlock);
	}

	close(sockfd);
}

/*
 * Resets the hostInfo record to the default values in hostDb.
 */
void resetHostInfo(string hostName) {
	string secret = hostDb[hostName].preSharedSecret;
	destroyMessageBuffers(hostName);
	hostDb.erase(hostName);
	hostDb[hostName] = hostInfo(secret);
}
