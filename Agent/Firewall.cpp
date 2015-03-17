/*
 * Firewall.cpp
 *
 *  Created on: Nov 26, 2014
 *      Author: atlancer
 */

#include "Firewall.h"

Firewall::Firewall() {
	cmdObj = new Command();
}

/*
 * Returns firewall rule table.
 */
string Firewall::show() {
	string result;

	try {
		result = cmdObj->execute("ip6tables -L");
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}

	return result;
}

/*
 * Allows to add a firewall rule.
 */
void Firewall::add(string chain, string position, string rule) {
	try {
		string result = cmdObj->execute("ip6tables -I " + chain + " " + position + " " + rule);
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}
}

/*
 * Allows to delete a firewall rule.
 */
void Firewall::remove(string chain, string rule) {
	try {
		string result = cmdObj->execute("ip6tables -D " + chain + " " + rule);
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}
}

Firewall::~Firewall() {
	delete cmdObj;
}

