/*
 * Table.cpp
 *
 *  Created on: Nov 26, 2014
 *      Author: atlancer
 */

#include "Table.h"
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>

Table::Table() {
	cmdObj = new Command();
}

string Table::show() {
	ifstream rt_file("/etc/iproute2/rt_tables");
	if (!rt_file.is_open()) {
		throw runtime_error("Wasn't able to open rt_tables file.\n");
	}
	stringstream buffer;
	buffer << rt_file.rdbuf();

	return buffer.str();
}

/*
 * Private function that returns true if the string specified consists of digits only.
 */
bool isNumber(string word) {
	bool isDigit = true;

	for (unsigned int i = 0; i < word.size(); i++) {
		if (!isdigit(word.at(i))) {
			isDigit = false;
		}
	}

	return isDigit;
}

/*
 * Checks if the vector contains the specified int.
 */
bool vectorContainsInt(vector<int> vectorToCheck, int integer) {
	bool vectorContainsInt = false;

	for (unsigned int i = 0; i < vectorToCheck.size(); i++) {
		if(vectorToCheck.at(i) == integer) {
			vectorContainsInt = true;
		}
	}

	return vectorContainsInt;
}

/*
 * Adds a new table with the name specified.
 * Check all the IDs in the file before doing so in order to create a unique id.
 */
void Table::add(string tableName) {
	fstream rt_file;
	string line;
	vector<int> existingRtIds;
	int currentId = 0;

	if (tableName.empty()) {
		throw runtime_error("The name of the table cannot be empty string!\n");
	}

	try {
		rt_file.open("/etc/iproute2/rt_tables");
		if (!rt_file.is_open()) {
			throw runtime_error("Wasn't able to open rt_tables file.\n");
		}

		//collect existing rt ids into vector
		while (getline(rt_file, line)) {
			istringstream iss(line);
			string rtId;
			string currentTableName;

			if (iss >> rtId) {
				//Check if the extracted word is the digit
				if(isNumber(rtId)) {
					existingRtIds.push_back(atoi(rtId.c_str()));
					//if the extracted word is a digit, then check the next word
					//to make sure we don't create duplicates
					if (iss >> currentTableName) {
						if (tableName == currentTableName) {
							throw runtime_error("The table with this name already exists.");
						}
					}
				}
			}
		}

		//create a unique rt id. Starts checking id numbers from 0.
		bool idFound = false;
		while (!idFound) {
			if (vectorContainsInt(existingRtIds, currentId)) {
				currentId++;
			}
			else {
				idFound = true;
			}
		}

		//write that id into the file
		rt_file.clear();
		rt_file << currentId << "	" << tableName << endl;

		rt_file.close();
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}
}

void Table::remove(string tableName) {
	fstream rt_file;
	fstream temp;
	string line;
	bool tableFound = false;

	if (tableName.empty()) {
		throw runtime_error("The name of the table cannot be empty string!\n");
	}

	try {
		rt_file.open("/etc/iproute2/rt_tables");
		if (!rt_file.is_open()) {
			throw runtime_error("Wasn't able to open rt_tables file.\n");
		}
		temp.open("/etc/iproute2/temp", fstream::out);
		if (!temp.is_open()) {
			throw runtime_error("Wasn't able to create a temp file.\n");
		}

		bool removeLine;
		while (getline(rt_file, line)) {
			istringstream iss(line);
			string wordInLine;
			removeLine = false;

			//check all the words in that line. If the table name was found,
			//then delete that line (by not putting it into the new file)
			while(iss >> wordInLine) {
				if(wordInLine == tableName) {
					removeLine = true;
					tableFound = true;
				}
			}

			if (!removeLine) {
				temp << line << endl;
			}
		}

		rt_file.close();
		temp.close();
		//remove file
		std::remove("/etc/iproute2/rt_tables");
		//rename file
		rename("/etc/iproute2/temp", "/etc/iproute2/rt_tables");

		if (!tableFound) {
			throw runtime_error("Table '" + tableName + "' was not found!\n");
		}
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}
}

Table::~Table() {
	delete cmdObj;
}

