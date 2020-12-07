# Custom UDP protocol (school assignment)

UDP communicator between client/server. Client can send messages and files, server will display or save them.
Simulation of errors during communication is implemented as well as keep alive packets.  

## How to run
* compile, then open .exe two times
* in one window choose server and in the second one choose clinet
* on clinet side choose `127.0.0.1` as IP of target, the target port should be higher than 1024
* follow instructions in terminal and you're good to go

## Wireshark
* you can see how packets are sent/revieved in wireshark
* start capturing packets from loopback trafic and set a filter `udp.port == ****`
  

Your source for r/badcode!
