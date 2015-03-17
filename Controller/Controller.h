/*
 * Contoller.h
 *
 *  Created on: Nov 14, 2014
 *      Author: atlancer
 */

#ifndef CONTOLLER_H_
#define CONTOLLER_H_

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <cstdlib>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <signal.h>
#include <iterator>
#include <pthread.h>
#include "ProtocolMessage.h"

#define PORT "3490"
#define BACKLOG 10
#define BUF_SIZE 500
#define MSG_LENGTH 500
#define MAXDATASIZE 30

using namespace std;

namespace Controller {
struct hostInfo {
	hostInfo() {
		isConnected = false;
		sockfd = 0;
		pthread_rwlock_init(&rwlock, NULL); //initialize the lock
		threadId = 0;
		seqNum = 1;
		ackNum = 1;
		lastProcessedSeqNum = 0;
		lastProcessedAckNum = 0;
	}

	hostInfo(string preSharedSecret) :
			preSharedSecret(preSharedSecret) {
		isConnected = false;
		sockfd = 0;
		pthread_rwlock_init(&rwlock, NULL); //initialize the lock
		threadId = 0;
		seqNum = 1;
		ackNum = 1;
		lastProcessedSeqNum = 0;
		lastProcessedAckNum = 0;
	}

	string preSharedSecret;
	bool isConnected;
	int sockfd;
	pthread_rwlock_t rwlock;
	string receiveBuffer; //will keep the partially received messages
	pthread_t threadId;
	int seqNum;
	int ackNum;
	int lastProcessedSeqNum;
	int lastProcessedAckNum;
	map<int, ProtocolMessage *> requestMessageBuffer;
	map<int, ProtocolMessage *> responseMessageBuffer;
};

struct configDbKey {
	string hostName;
	string source;
	string destination;

	bool operator<(const configDbKey& param) const {
		if (hostName < param.hostName)
			return false;
		if (hostName > param.hostName)
			return true;

		if ((source != "*") && (param.source != "*")) {
			if (source < param.source)
				return false;
			if (source > param.source)
				return true;
		}
		else if (source == "*") {
			return false;
		}
		else if (param.source == "*") {
			return true;
		}

		if ((destination != "*") && (param.destination != "*")) {
			if (destination < param.destination)
				return false;
			if (destination > param.destination)
				return true;
		}
		else if (destination == "*") {
			return false;
		}
		else if (param.destination == "*") {
			return true;
		}

		return true;
	}

	bool match(const configDbKey& dbKey) {
		bool result = true;

		if (hostName != dbKey.hostName) {
			result = false;
		}
		else {
			if (dbKey.source != "*") {
				if (source != dbKey.source) {
					result = false;
				}
			}

			if (dbKey.destination != "*") {
				if (destination != dbKey.destination) {
					result = false;
				}
			}
		}

		return result;
	}
};

map<string, hostInfo> hostDb; //stores info and pre-shared secrets for the hosts
multimap<struct configDbKey, vector<string> > configDb;
int terminateExec = 0;
int sockfd;
}

void sigterm_handler(int signum);
void sigchld_handler(int signal);
void process_request(int sockfd);
void populateHostDb(map<string, Controller::hostInfo> &db,
		char* fileToReadFrom);
void populateConfigDb(multimap<Controller::configDbKey, vector<string> > &db,
		char* fileToReadFrom);
vector<string> getValueFromConfigDb(Controller::configDbKey key);
void printConfigMap();
vector<string> extractCommands(string commandString, string delimiter);
void *runCmd(void *arg);
void readCommandsIntoVector(vector<string> &cmds, const char* fileToReadFrom);
void printInvalidCmdError();
void processCmd(string input);
void destroyLocks();
void runListener();
void* processConnectedHost(void *arg);
void sendCommand(string type, string command, string argument, string host);
void sendElevResp(string host, string command, string destination, int elevReqId);
unsigned int countWordsInString(string const& str);
bool vectorContainsStr(vector<string> search, string find);
string authenticateHost(int sockfd, int &ackNum);
void processElevationRequest(ProtocolMessage *msgObjRef, string hostName);
string getNextMessageFromHost(string hostName);
void processCommandResponse(ProtocolMessage* commandResponseObjRef,
		string hostName);
void resetHostInfo(string hostName);
void cleanUp();
#endif /* CONTOLLER_H_ */
