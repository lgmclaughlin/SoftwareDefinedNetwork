/*
 * ProtocolMessage.cpp
 *
 *  Created on: Nov 14, 2014
 *      Author: atlancer
 */

#include "ProtocolMessage.h"
#include <netinet/in.h>
#include <cstdio>
#include <iostream>

using namespace std;

/*
 * The constructor for the ProtocolMessage class.
 */
ProtocolMessage::ProtocolMessage(uint16_t seq, uint16_t ack, uint16_t elevId,
		bool isAuthentication, bool isElev, bool isError, enum type type,
		enum command command, string body) :
		seq(seq), ack(ack), elevId(elevId), isAuthentication(isAuthentication), isElev(
				isElev), isError(isError), type(type), command(command), body(
				body) {
	length = body.size() + 13; //13 - header length
}

/*
 * The constructor for the ProtocolMessage class.
 */
ProtocolMessage::ProtocolMessage(string message) {
	string header = message.substr(0, 12);
	uint32_t networkEncodedLength = (message.c_str()[0] << 24)
			+ (message.c_str()[1] << 16) + (message.c_str()[2] << 8)
			+ (message.c_str()[3]);
	length = ntohl(networkEncodedLength);
	uint16_t networkEncodedSeq = (message.c_str()[4] << 8)
			+ (message.c_str()[5]);
	seq = ntohs(networkEncodedSeq);
	uint16_t networkEncodedAck = (message.c_str()[6] << 8)
			+ (message.c_str()[7]);
	ack = ntohs(networkEncodedAck);
	uint16_t networkEncodedElevId = (message.c_str()[8] << 8)
			+ (message.c_str()[9]);
	elevId = ntohs(networkEncodedElevId);
	isAuthentication = (message.c_str()[10] & 1) > 0;
	isElev = (message.c_str()[10] & 2) > 0;
	isError = (message.c_str()[10] & 4) > 0;
	switch (message.c_str()[11]) {
	case ProtocolMessage::IP:
		type = ProtocolMessage::IP;
		break;
	case ProtocolMessage::NAT:
		type = ProtocolMessage::NAT;
		break;
	case ProtocolMessage::Firewall:
		type = ProtocolMessage::Firewall;
		break;
	case ProtocolMessage::Neighbor:
		type = ProtocolMessage::Neighbor;
		break;
	case ProtocolMessage::RoutingTable:
		type = ProtocolMessage::RoutingTable;
		break;
	case ProtocolMessage::Route:
		type = ProtocolMessage::Route;
		break;
	case ProtocolMessage::Rule:
		type = ProtocolMessage::Rule;
		break;
	case ProtocolMessage::NoType:
	default: //if the command is unrecognized, we ignore it
		type = ProtocolMessage::NoType;
		break;
	}

	switch (message.c_str()[12]) {
	case ProtocolMessage::Show:
		command = ProtocolMessage::Show;
		break;
	case ProtocolMessage::Add:
		command = ProtocolMessage::Add;
		break;
	case ProtocolMessage::Remove:
		command = ProtocolMessage::Remove;
		break;
	case ProtocolMessage::Retry:
		command = ProtocolMessage::Retry;
		break;
	case ProtocolMessage::Drop:
		command = ProtocolMessage::Drop;
		break;
	case ProtocolMessage::NoCommand:
	default: //if the command is unrecognized, we ignore it
		command = ProtocolMessage::NoCommand;
		break;
	}

	if (message.size() > 13) {
		body = message.substr(13, message.size() - 1); //put the body into the field right away
	}
}

/*
 * Converts the message into the transferable string.
 */
string ProtocolMessage::convertToTransferableMessage() {
	string header;
	length = body.size() + 13; //13 - header length
	uint32_t networkEncodedLength = htonl(length);
	header.push_back((char) (networkEncodedLength >> 24));
	header.push_back((char) (networkEncodedLength >> 16));
	header.push_back((char) (networkEncodedLength >> 8));
	header.push_back((char) networkEncodedLength);
	uint16_t networkEncodedSeq = htons(seq);
	header.push_back((char) (networkEncodedSeq >> 8));
	header.push_back((char) networkEncodedSeq);
	uint16_t networkEncodedAck = htons(ack);
	header.push_back((char) (networkEncodedAck >> 8));
	header.push_back((char) networkEncodedAck);
	uint16_t networkEncodedElevId = htons(elevId);
	header.push_back((char) (networkEncodedElevId >> 8));
	header.push_back((char) networkEncodedElevId);

	//check flags
	char flags = 0;
	if (isAuthentication) {
		flags |= 1;
	}
	if (isElev) {
		flags |= 2;
	}
	if (isError) {
		flags |= 4;
	}
	header.push_back(flags);

	header.push_back(type);
	header.push_back(command);

	return header + body;
}

/*
 * Prints the message object (primarily used for debugging purposes).
 */
void ProtocolMessage::printMessageObject() {
	cout << "length: " << (int) this->length << endl;
	cout << "seq: " << (int) this->seq << endl;
	cout << "ack: " << (int) this->ack << endl;
	cout << "elevId: " << (int) this->elevId << endl;
	cout << "isAuthentication: " << this->isAuthentication << endl;
	cout << "isElev: " << this->isElev << endl;
	cout << "isError: " << this->isError << endl;
	cout << "type: " << this->type << endl;
	cout << "command: " << this->command << endl;
	cout << "body: " << this->body << endl;
}

/*
 * Gets the type in the string format.
 */
string ProtocolMessage::getStringType() {
	switch (this->type) {
	case ProtocolMessage::IP:
		return "ip";
	case ProtocolMessage::NAT:
		return "nat";
	case ProtocolMessage::Firewall:
		return "firewall";
	case ProtocolMessage::Neighbor:
		return "neighbor";
	case ProtocolMessage::RoutingTable:
		return "routingTable";
	case ProtocolMessage::Rule:
		return "rule";
	case ProtocolMessage::Route:
		return "route";
	default:
		return "";
	}
}

/*
 * Gets the type in the enum format by converting the input argument.
 */
enum ProtocolMessage::type ProtocolMessage::getEnumType(string type) {
	enum ProtocolMessage::type typeNum;

	if (type == "ip") {
		typeNum = ProtocolMessage::IP;
	} else if (type == "nat") {
		typeNum = ProtocolMessage::NAT;
	} else if (type == "firewall") {
		typeNum = ProtocolMessage::Firewall;
	} else if (type == "neighbor") {
		typeNum = ProtocolMessage::Neighbor;
	} else if (type == "routingTable") {
		typeNum = ProtocolMessage::RoutingTable;
	} else if (type == "rule") {
		typeNum = ProtocolMessage::Rule;
	} else if (type == "route") {
		typeNum = ProtocolMessage::Route;
	} else {
		typeNum = ProtocolMessage::NoType;
	}

	return typeNum;
}

/*
 * Gets the command in the string format.
 */
string ProtocolMessage::getStringCommand() {
	switch (this->command) {
	case ProtocolMessage::Show:
		return "show";
	case ProtocolMessage::Add:
		return "add";
	case ProtocolMessage::Remove:
		return "remove";
	case ProtocolMessage::Retry:
		return "retry";
	case ProtocolMessage::Drop:
		return "drop";
	default:
		return "";
	}
}

/*
 * Gets the command in the enum format by converting the input argument.
 */
enum ProtocolMessage::command ProtocolMessage::getEnumCommand(string command) {
	enum ProtocolMessage::command cmdNum;

	if (command == "add") {
		cmdNum = ProtocolMessage::Add;
	} else if (command == "remove") {
		cmdNum = ProtocolMessage::Remove;
	} else if (command == "show") {
		cmdNum = ProtocolMessage::Show;
	} else if (command == "retry") {
			cmdNum = ProtocolMessage::Retry;
	} else if (command == "drop") {
			cmdNum = ProtocolMessage::Drop;
	} else {
		cmdNum = ProtocolMessage::NoCommand;
	}

	return cmdNum;
}

/*
 * Returns the list of valid commands for the user.
 * NOTE: The list doesn't include the commands for the elevation
 * because they are not supposed to be issued by the user.
 */
vector<string> ProtocolMessage::getValidCommands() {
	vector<string> validCommands;

	validCommands.push_back("add");
	validCommands.push_back("remove");
	validCommands.push_back("show");

	return validCommands;
}

/*
 * Returns the list of valid types for the user.
 */
vector<string> ProtocolMessage::getValidTypes() {
	vector<string> validTypes;

	validTypes.push_back("nat");
	validTypes.push_back("ip");
	validTypes.push_back("firewall");
	validTypes.push_back("neighbor");
	validTypes.push_back("routingTable");
	validTypes.push_back("rule");
	validTypes.push_back("route");

	return validTypes;
}

ProtocolMessage::~ProtocolMessage() {
// TODO Auto-generated destructor stub
}

