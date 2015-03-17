/*
 * Firewall.h
 *
 *  Created on: Nov 26, 2014
 *      Author: atlancer
 */

#ifndef FIREWALL_H_
#define FIREWALL_H_

#include <string>
#include "Command.h"

using namespace std;

class Firewall {
private:
	Command *cmdObj;
public:
	Firewall();
	virtual ~Firewall();
	string show();
	void add(string chain, string position, string rule);
	void remove(string chain, string rule);
};

#endif /* FIREWALL_H_ */
