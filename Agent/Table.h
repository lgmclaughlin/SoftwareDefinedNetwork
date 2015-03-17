/*
 * Table.h
 *
 *  Created on: Nov 26, 2014
 *      Author: atlancer
 */

#ifndef TABLE_H_
#define TABLE_H_

#include <string>
#include "Command.h"

using namespace std;

class Table {
private:
	Command *cmdObj;
public:
	Table();
	virtual ~Table();
	string show();
	void add(string tableName);
	void remove(string tableName);
};

#endif /* TABLE_H_ */
