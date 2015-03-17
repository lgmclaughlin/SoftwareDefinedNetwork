Synopsis
========
This project implements a Software Defined Network. An SDN effectively allows a Controller to sit on a network and give Hosts commands to modify routes, firewall rules, NAT settings, network discovery caches, etc. while the network is still operating. The ability to change the network on the fly is extremely practical, and is commonly used in LANs, WANs, and Data Centers. For more details on the project implementation, view the DESIGN.md file.

Motivation
==========
The SDN project was assigned as a seven week project during a Advanced Networks course at my college. 

Creation
========
The project was completed on December 12, 2014 by myself and my project partner, Batyrlan Nurbekov. We chose to use object-oriented design techniques with C++ in order to keep things organized.

We tested the system on a network of 6 VMs all running Ubuntu Linux. These VMs included a DNS Server (using BIND 9), a Controller, and four Hosts. 

Installation
============
As this was a school project, it was not meant to be distributed and used. Nonetheless, it's certainly possible to recreate the environment we used. As long as the Hosts can reach the Controller and vice versa, building the project is just a matter of running make on each machine and spinning up the program. 

