/*
 * IPv6.cpp
 *
 *  Created on: Nov 8, 2014
 *      Author: atlancer
 */

#include "IPv6.h"

using namespace std;

/*
 * Constructor.
 */
IPv6::IPv6() {
	cmdObj = new Command();
}

/*
 * Returns ifconfig.
 */
string IPv6::show() {
	string result;

	try {
		result = cmdObj->execute("ifconfig");
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}

	return result;
}

/*
 * Allows to add an IPv6 alias.
 */
void IPv6::add(string address, string device = "eth0") {
	try {
		string result = cmdObj->execute("ip -6 address add " + address +  " dev " + device);
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}
}

/*
 * Removes the IPv6 alias.
 */
void IPv6::remove(string address, string device = "eth0") {
	try {
		string result = cmdObj->execute("ip -6 address del " + address +  " dev " + device);
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}
}

/*
 * Destructor.
 */
IPv6::~IPv6() {
	delete cmdObj;
}

