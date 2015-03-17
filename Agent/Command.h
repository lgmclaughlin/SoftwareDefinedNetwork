/*
 * Command.h
 *
 *  Created on: Nov 8, 2014
 *      Author: atlancer
 */

#ifndef COMMAND_H_
#define COMMAND_H_

#include <string>
#include <vector>

using namespace std;

class Command {
private:
	vector<char *> args;
	void parseCmdIntoArgs(string cmd);
public:
	Command();
	string execute(string cmd);
	virtual ~Command();
};

#endif /* COMMAND_H_ */
