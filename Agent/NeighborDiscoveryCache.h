/*
 * NeighborDiscoveryCache.h
 *
 *  Created on: Nov 26, 2014
 *      Author: atlancer
 */

#ifndef NEIGHBORDISCOVERYCACHE_H_
#define NEIGHBORDISCOVERYCACHE_H_

#include <string>
#include "Command.h"

using namespace std;

class NeighborDiscoveryCache {
private:
	Command *cmdObj;
public:
	NeighborDiscoveryCache();
	virtual ~NeighborDiscoveryCache();
	string show();
	void add(string addr, string lladr, string device);
	void remove(string addr, string lladr, string device);
};

#endif /* NEIGHBORDISCOVERYCACHE_H_ */
