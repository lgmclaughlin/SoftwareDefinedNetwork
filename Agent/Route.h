/*
 * Route.h
 *
 *  Created on: Nov 26, 2014
 *      Author: atlancer
 */

#ifndef ROUTE_H_
#define ROUTE_H_

#include <string>
#include "Command.h"

using namespace std;

class Route {
private:
	Command *cmdObj;
public:
	Route();
	virtual ~Route();
	string show(string args);
	void add(string args);
	void remove(string args);
};

#endif /* ROUTE_H_ */
