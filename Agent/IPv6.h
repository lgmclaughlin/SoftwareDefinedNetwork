/*
 * IPv6.h
 *
 *  Created on: Nov 8, 2014
 *      Author: atlancer
 */

#ifndef IPV6_H_
#define IPV6_H_

#include <string>
#include "Command.h"

using namespace std;

class IPv6 {
private:
	Command *cmdObj;
public:
	IPv6();
	virtual ~IPv6();
	string show();
	void add(string address, string device);
	void remove(string address, string device);
};

#endif /* IPV6_H_ */
