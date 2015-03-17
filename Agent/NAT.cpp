/*
 * NAT.cpp
 *
 *  Created on: Nov 9, 2014
 *      Author: atlancer
 */

#include "NAT.h"

using namespace std;

NAT::NAT() {
	cmdObj = new Command();
}

/*
 * Returns NAT rule table.
 */
string NAT::show() {
	string result;

	try {
		result = cmdObj->execute("ip6tables -t nat -L");
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}

	return result;
}

/*
 * Allows to add an NAT rule.
 */
void NAT::add(string chain, string position, string rule) {
	try {
		string result = cmdObj->execute("ip6tables -t nat -I " + chain + " " + position + " " + rule);
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}
}

/*
 * Allows to delete a NAT rule.
 */
void NAT::remove(string chain, string rule) {
	try {
		string result = cmdObj->execute("ip6tables -t nat -D " + chain + " " + rule);
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}
}

NAT::~NAT() {
	delete cmdObj;
}

