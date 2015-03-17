/*
 * Route.cpp
 *
 *  Created on: Nov 26, 2014
 *      Author: atlancer
 */

#include "Route.h"

Route::Route() {
	cmdObj = new Command();
}

string Route::show(string args) {
	string result;

	try {
		result = cmdObj->execute("ip -6 route show " + args);
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}

	return result;
}

void Route::add(string args) {
	try {
		string result = cmdObj->execute("ip -6 route add " + args);
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}
}

void Route::remove(string args) {
	try {
		string result = cmdObj->execute("ip -6 route del " + args);
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}
}

Route::~Route() {
	delete cmdObj;
}

