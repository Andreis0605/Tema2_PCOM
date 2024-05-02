```
usage: udp_client.py [-h] [--input_file FILE] [--source-address SOURCE_ADDRESS] [--source-port 1024-65535] [--mode {all_once,manual,random}] [--count COUNT] [--delay DELAY] server_ip server_port

UDP Client for Communication Protocols (2023-2024) Homework #2

optional arguments:
  -h, --help            show this help message and exit

Input:
  --input_file FILE     JSON file to read payloads from (default: sample_payloads.json)

Server Address & Port (required):
  server_ip             Server IP
  server_port           Server Port

Source Address:
  --source-address SOURCE_ADDRESS
                        IP Address to be bind by UDP client (default: unspecified)
  --source-port (1024-65535)
                        UDP port to be used as source for this client (default: random port)

Workload changing parameters:
  --mode {all_once,manual,random}
                        Specifies the mode used for the load generator as following:
                        * all_once - send each payload in the list once
                        * manual - let you choose which message to send next
                        * random - continuously send random payloads from the list

Load characteristics:
  --count COUNT         Number of packets to be send (only used for when mode is random, default: infinity)
  --delay DELAY         Wait time (in ms) between two messages (default: 0)
```
