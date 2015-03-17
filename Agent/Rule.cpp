/*
 * Rule.cpp
 *
 *  Created on: Nov 26, 2014
 *      Author: atlancer
 */

#include "Rule.h"

Rule::Rule() {
	cmdObj = new Command();
}

/*
 * Returns list of rules.
 */
string Rule::show() {
	string result;

	try {
		result = cmdObj->execute("ip -6 rule show");
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}

	return result;
}

/*
 * Allows to add a rule.
 */
void Rule::add(string args) {
	try {
		string result = cmdObj->execute("ip -6 rule add " + args);
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}
}

/*
 * Allows to delete a rule.
 */
void Rule::remove(string args) {
	try {
		string result = cmdObj->execute("ip -6 rule del " + args);
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}
}

Rule::~Rule() {
	delete cmdObj;
}

