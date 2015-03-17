Build/Clean
===========
In order to build the controller or the agent "make" command should be executed in the corresponding folder.

In order to clean the controller or the agent "make clean" command should be executed in the corresponding folder.

Run
===
In order to run the controller the executable should be invoked in the following format: ./Controller {FileWithPreSharedSecrets}.

In order to run the agent the executable should be invoked in the following format: ./Agent {controllerFqdn} {controllerPort} {hostFqdn} {preSharedSecret}.

Design of the Solution
======================

Basic Functionality
-------------------
Upon running the controller, it sets up a database of expected Hosts that should be able to connect. To do so, it uses an external file with each hostname and a corresponding pre-shared secret (a naive way of authenticating the connecting host). It then begins listening on the specified port for requests from hosts on the network.

With each connecting host, the controller creates a new thread and populates a struct to keep track of information about that host. Once the connection is set up, the Controller can begin sending commands to the host.

Dealing with Elevation
----------------------
A major consideration was how to handle the case where a host does not have a route to the destination it would like to send to. In this situation, the host must elevate a packet to the controller and ask for a route. Below is the solution to this that we came up with.

1) We created code that adds a tunnel automatically to the host machine. This tunnel receives any traffic for which the host agent does not have a destination in its routing tables. This is accomplished by adding a default route for the agent pointing to the tunnel.  

2) The host agent listens for elevation requests coming out of the tunnel. Anything that comes out of the tunnel is sent to the controller for handling. Only the first 1200 bytes of each packet is sent. 

3) The controller listens for elevated packets from the host agent. When it receives these, it checks our protocol header to see if the packet is a true elevation from an agent. It then parses out the source and destination IP addresses.

4) The controller attempts to find the entry in the configuration database (based on the hostname and IP addresses mentioned above). When an appropriate match is found, the controller initiates sending all the commands to that host contained in the configuration database. 

5) The host agent receives a slew of commands from the controller and is none the wiser that they have anything to do with the elevation request.

6) Once the controller is finished, it sends a response to the elevation request. When the host agent receives the elevation response, it checks the acknowledgment number to see if it matches the sequence number of the elevation request. If it does, it attempts to resend/drop the packet. In other other words, the acknowledgment number acts as an elevation ID because it is unique. 