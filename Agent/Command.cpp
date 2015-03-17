/*
 * Command.cpp
 *
 *  Created on: Nov 8, 2014
 *      Author: atlancer
 */

#include "Command.h"
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdexcept>

using namespace std;

Command::Command() {
}

/*
 * Parses the string into the separate commands.
 */
void Command::parseCmdIntoArgs(string cmd) {
	istringstream iss(cmd);

	string token;
	while(iss >> token) {
	  char *arg = new char[token.size() + 1];
	  copy(token.begin(), token.end(), arg);
	  arg[token.size()] = '\0';
	  args.push_back(arg);
	}
	args.push_back('\0');
}

/*
 * Executes the command.
 */
string Command::execute(string cmd) {
	parseCmdIntoArgs(cmd);

	int pipeForStdOut[2], pipeForStdErr[2];
	string stdOutStr, stdErrStr;

	char buf[32] = { 0 };
	ssize_t bytesRead;
	pid_t childPid;

	pipe(pipeForStdOut);
	pipe(pipeForStdErr);

	childPid = fork();
	if (childPid == -1) {
		throw runtime_error(strerror(errno));
		exit(-1);
	} else if (childPid == 0) {
		// remember 0 is for 1nput and 1 is for 0utput
		close(pipeForStdOut[0]); // parent keeps the input
		if (dup2(pipeForStdOut[1], 1) < 0) { // child gets the output end
			perror("dup2.1");
			exit(-1);
		}

		close(pipeForStdErr[0]); // parent keeps the input
		if (dup2(pipeForStdErr[1], 2) < 0) {
			perror("dup2.2");
			exit(-1);
		}

		if (execvp(args[0], &args[0]) == -1) {
			perror("execv");
			exit(-1);
		}
		exit(0);
	}
	else {
		wait(NULL);
	}

	fcntl(pipeForStdOut[0], F_SETFL, O_NONBLOCK | O_ASYNC);
	while (1) {
		bytesRead = read(pipeForStdOut[0], buf, sizeof(buf) - 1);
		if (bytesRead <= 0)
			break;
		buf[bytesRead] = '\0'; // append null terminator
		stdOutStr += buf; // append what wuz read to the string
	}

	fcntl(pipeForStdErr[0], F_SETFL, O_NONBLOCK | O_ASYNC);
	while (1) {
		bytesRead = read(pipeForStdErr[0], buf, sizeof(buf) - 1);
		if (bytesRead <= 0)
			break;
		buf[bytesRead] = '\0'; // append null terminator
		stdErrStr += buf; // append what wuz read to the string
	}

	if (!stdErrStr.empty()) {
		throw runtime_error(stdErrStr);
	}

	close(pipeForStdOut[0]);
	close(pipeForStdErr[0]);

	args.clear();

	return stdOutStr;
}

/*
 * Default destructor.
 */
Command::~Command() {
	vector<char *>().swap(args); //clean up the vector with arguments
}

