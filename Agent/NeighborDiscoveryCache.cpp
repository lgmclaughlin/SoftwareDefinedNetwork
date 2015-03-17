/*
 * NeighborDiscoveryCache.cpp
 *
 *  Created on: Nov 26, 2014
 *      Author: atlancer
 */

#include "NeighborDiscoveryCache.h"

NeighborDiscoveryCache::NeighborDiscoveryCache() {
	cmdObj = new Command();
}

/*
 * Returns Neighbor Discovery Cache table entries.
 */
string NeighborDiscoveryCache::show() {
	string result;

	try {
		result = cmdObj->execute("ip -6 neigh show");
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}

	return result;
}

/*
 * Allows to add an NDC table entry.
 */
void NeighborDiscoveryCache::add(string addr, string lladr, string device) {
	try {
		string result = cmdObj->execute("ip -6 neigh add " + addr + " lladdr " + lladr + " dev " + device);
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}
}

/*
 * Allows to delete a NDC table entry.
 */
void NeighborDiscoveryCache::remove(string addr, string lladr, string device) {
	try {
		string result = cmdObj->execute("ip -6 neigh del " + addr + " lladdr " + lladr + " dev " + device);
	}
	catch(exception& e) {
		throw; //re-throw the exception
	}
}

NeighborDiscoveryCache::~NeighborDiscoveryCache() {
	delete cmdObj;
}

