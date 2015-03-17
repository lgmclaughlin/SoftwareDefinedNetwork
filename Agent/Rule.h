/*
 * Rule.h
 *
 *  Created on: Nov 26, 2014
 *      Author: atlancer
 */

#ifndef RULE_H_
#define RULE_H_

#include <string>
#include "Command.h"

using namespace std;

class Rule {
private:
	Command *cmdObj;
public:
	Rule();
	virtual ~Rule();
	string show();
	void add(string args); //ip rule add from 192.168.1.20 table 120
	void remove(string args); //ip rule add from 192.168.1.20 table 120 ??
};

#endif /* RULE_H_ */
