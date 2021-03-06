# AXIOM NIC applications

----

**NOTE: This repository is a submodule of axiom-evi project. Please clone the
master project from https://github.com/evidence/axiom-evi**

This repository contains the following user space application:

 * axiom-init
    + axiom-init is the main daemon running on all nodes of the cluster.
    It runs the discovery algorithm, sets the routing table and handles control messages.
    It will be run automatically by the system or through axiom-startup.sh script.
```
        example:

        # in n-1 nodes
        axiom-init &

        # in the master node
        axiom-init -m &
```
 * axiom-ethtap
    + axiom-ethtap is another deamon that can run on each nodes in the cluster
    to map the ethernet frames over AXIOM messages.
    It creates a virtual ethernet device on top of AXIOM device.
    It will be run automatically by the system or through axiom-startup.sh script.
```
        example:
        # start axiom-ethtap with 4 threads
        axiom-ethtap -n 4
```
 * axiom-info
    + print informations (node-id, interfaces, routing, etc.) about AXIOM NIC
```
        example:
        # print all information
        axiom-info
```
```
        # print nodeid
        axiom-info -n
```
```
        # print routing table
        axiom-info -r
```
 * axiom-whoami
     + print the node-id set after the discovery phase
```
        example:
        axiom-whoami
```
 * axiom-ping
    + estimate the round trip time (RTT) between two nodes
```
        example:
        # Estimate RTT with target node 2. Send a ping message every 0.5 seconds.
        axiom-ping -d 2 -i 0.5
```
```
        # Estimante RTT with target node 2. Send 10 ping message every 0.2 seconds.
        axiom-ping -d 2 -c 10 -i 0.2
```
 * axiom-netperf
    + estimate the throughput between two nodes
```
        example:
        # start server on one node with 8 threads
        axiom-netperf -s -n 8

        # Estimate the throughput with targat node 3, sending 512 KBytes of data
        # using RAW messages.
        axiom-netperf -d 3 -l 512K -t raw
```
```
        # Estimate the throughput with targat node 3, sending 2 MBytes of data.
        # using LONG messages and 2 sending threads.
        axiom-netperf -d 3 -l 2M -t long -n 2
```
 * axiom-traceroute
    + print the hops needed to reach a specified target node
```
        example:
        # Print the hops needed to reach the node 1
        axiom-traceroute -d 1
```
 * axiom-[send|recv]
     + send/receive Axiom RAW/LONG data to/from a node
```
        examples:
        # receive RAW message
        axiom-recv -t raw
```
```
        # receive LONG message
        axiom-recv -t long
```
```
        # send RAW data to node (node_id=4) with a specified payload
        axiom-send -t raw -d 4   112 43 0 57 33
```
```
        # send RAW data to a neighbour directly connected on a local
        # interface (if_id=1) with a specified payload
        axiom-send -t raw -d 1 --neighbour   67 52 1 0 1 0
```
```
        # send LONG data to node (node_id=0) with a specified payload
        axiom-send -t long -d 0   112 43 0 57 33
```
 * axiom-rdma
     + use RDMA features (remote write, remote read) of Axiom NIC
```
        examples:
        # dump local RDMA zone (starting from offset 0 to 1023)
        axiom-rdma -m d -s 1k
```
```
        # store in the local RDMA zone (starting from offset 1024 to 2043)
        # the bytes ( 1, 2, 3, 4) repeated many times to fill the size
        # specified
        axiom-rdma -m s -o 1k -s 1k 1 2 3 4
```
```
        # write 3072 bytes in the RDMA zone of node 3 (offset 2048) reading
        # from the local RDMA zone (offset 1024)
        axiom-rdma -m w -n 3 -o 1k -O 2k -s 3k
```
```
        # read 4096 bytes in the RDMA zone of node 2 (offset 7168) reading
        # from the local RDMA zone (offset 0)
        axiom-rdma -m r -n 2 -O 7k -s 4k
```
 * axiom-run
    + spawn application on multiple nodes
```
        # run ls on remote node 1 (-n) and redirect (-r) the stdin/stdout/stderr
        axiom-run -n 1 -r ls
```
```
        # run ls on all nodes, redirect (-r) the stdin/stdout/stderr and print
        # the node id in each line (-i)
        axiom-run -r -i ls
```
