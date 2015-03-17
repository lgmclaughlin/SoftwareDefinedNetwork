/*
 * ProtocolMessage.h
 *
 *  Created on: Nov 14, 2014
 *      Author: atlancer
 */
#ifndef PROTOCOLMESSAGE_H_
#define PROTOCOLMESSAGE_H_

#include <string>
#include <stdint.h>
#include <vector>

using namespace std;

class ProtocolMessage {
public:
	enum type {
		NoType = 0, IP = 1, NAT = 2, Firewall = 3, Neighbor = 4, RoutingTable = 5, Route = 6, Rule = 7
	};
	enum command {
		NoCommand = 0, Show = 1, Add = 2, Remove = 3, Retry = 4, Drop = 5
	};

	uint32_t length;
	uint16_t seq;
	uint16_t ack;
	uint16_t elevId;
	bool isAuthentication;
	bool isElev;
	bool isError;
	enum type type;
	enum command command;
	string body;

	ProtocolMessage(uint16_t seq, uint16_t ack, uint16_t elevId,
			bool isAuthentication, bool isElev, bool isError, enum type type,
			enum command command, string body);
	ProtocolMessage(string message);
	string convertToTransferableMessage();
	void printMessageObject();
	string getStringType();
	static enum type getEnumType(string type);
	string getStringCommand();
	static enum command getEnumCommand(string command);
	static vector<string> getValidCommands();
	static vector<string> getValidTypes();
	virtual ~ProtocolMessage();
};

#endif /* PROTOCOLMESSAGE_H_ */
