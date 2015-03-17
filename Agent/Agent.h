/*
 * Agent.h
 *
 *  Created on: Nov 16, 2014
 *      Author: atlancer
 */

#ifndef AGENT_H_
#define AGENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <stdexcept>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <iostream>
#include <sstream>
#include <map>
#include <pthread.h>

//Raw socket includes:
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */

//Custom includes:
#include "NAT.h"
#include "IPv6.h"
#include "NeighborDiscoveryCache.h"
#include "Firewall.h"
#include "Table.h"
#include "Rule.h"
#include "Route.h"
#include "ProtocolMessage.h"

using namespace std;

#define MSG_LENGTH 500 /* The maximum length of the message */
#define BUF_SIZE 2000 /* The buffer should be large enough to fit the entire header */
#define MAXDATASIZE 30

void createNewTunnel(string tunnelName);
void setupPacketSockets(string tunnelDevice);
int createPacketSocket(int domain, int socketType, int protocol, string deviceToBindTo);
void* listenForElevation(void *arg);
void setupConnectionWithController(string controllerFqdn, string port);
void requestAuthentication();
void handleElevationResponse(ProtocolMessage* elevRespMsgObjRef);
void listenForCommands();
string getNextMessageFromController();
void processCommand(ProtocolMessage* commandMsgObjRef);

namespace Agent {
	string hostFqdn;
	string preSharedSecret;

	int controllerSockFd;
	int packetSocketFd;
	int rawSocketFd;
	string receiveBuffer;
	int seqNum = 1;
	int ackNum = 1;
	int lastProcessedSeqNum = 0;
	int lastProcessedAckNum = 0;
	map<int, ProtocolMessage *> requestMessageBuffer;
	map<int, string> elevationPackets;
}

#endif /* AGENT_H_ */
