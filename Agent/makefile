all: agent

agent: Agent.cpp Agent.h IPv6.cpp IPv6.h NAT.cpp NAT.h Command.cpp Command.h ProtocolMessage.cpp ProtocolMessage.h
	g++ ProtocolMessage.cpp Agent.cpp IPv6.cpp NAT.cpp Command.cpp Firewall.cpp NeighborDiscoveryCache.cpp Route.cpp Rule.cpp Table.cpp -o Agent -lpthread

clean:
	rm Agent
