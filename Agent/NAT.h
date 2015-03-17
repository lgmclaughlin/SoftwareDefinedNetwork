/*
 * NAT.h
 *
 *  Created on: Nov 9, 2014
 *      Author: atlancer
 */

#ifndef NAT_H_
#define NAT_H_

#include <string>
#include "Command.h"

using namespace std;

class NAT {
private:
	Command *cmdObj;
public:
	NAT();
	virtual ~NAT();
	string show();
	void add(string chain, string position, string rule);
	void remove(string chain, string rule);
};

#endif /* NAT_H_ */
