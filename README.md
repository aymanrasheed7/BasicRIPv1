# BasicRIPv1

##  Implemented Features
1. Routers are simulated as processes where the processes are started by running multiple instances of one C program
2. The program has two threads, one works for broadcasting RIP packets and the other works for listening to UDP port for RIP packets and for applying changes to the routing tables
3. The program should work on both Windows and Linux
4. Includes randomized periodic timer (25-35s) for each router
5. Includes expiration timer (180s) for each entry in the routing table
6. Includes garbage collection timer (120s) for each entry in the routing table
7. The packets are exchanged between routers in a multicast group in RIPv1 packet format

##  Limitations
1. Attempted to use classless addresing instead of classful addresing
2. In wireshark capture the packets do not contain real ipv4 info in the ip header since the assigned ip addresses to the routers are not real, however the assigned ip addresses are used in the RIP packet
3. The timer triggers are implemented manually, so they can activate some seconds later than the intended time
4. There is no triggered update, as a result response packets are sent only based on the periodic timer
5. Split Horizon, Poison Reverse etc. techniques are not implemented, as a result instability problem is possible, that is adjacent routers exchanging expired routing entries for a long time

## Usage
After Compilation, multiple instances of the program can be run with the help of config files. The config files for a router contains the ip addresses of the interfaces of the router and its neighboring routers.

Compilation in Windows: <pre> gcc RIP.c -lwsock32 </pre>
Execution in Windows: <pre> \a.exe <config file> </pre>
Compilation in Linux: <pre> gcc RIP.c </pre>
Execution in Linux: <pre> \a.out <config file> </pre>
