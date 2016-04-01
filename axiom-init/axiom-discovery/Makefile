AXIOM_INCLUDE := ../../../axiom-docs/include
AXIOM_SIM := axiom_simulator

APPS := axiom_discovery_protocol_test axsw_discovery_protocol_test
CLEANFILES = $(APPS) *.o $(AXIOM_SIM)/*.o
CFLAGS += -Wall -g -DPDEBUG -I$(AXIOM_INCLUDE) -I./axiom_simulator/
LDFLAGS += -pthread

PWD := $(shell pwd)
#MKFILE_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
BUILDROOT := ${PWD}/../../../axiom-buildroot
DESTDIR := ${BUILDROOT}/output/target
CCPREFIX := ${BUILDROOT}/output/host/usr/bin/arm-linux-
CC := ${CCPREFIX}gcc


.PHONY: all clean

all: $(APPS)

axsw_discovery_protocol_test: axiom_discovery_protocol_test.o $(AXIOM_SIM)/axiom_simulator.o $(AXIOM_SIM)/axiom_nic_simulator.o axiom_node_code.o axiom_discovery_protocol.o axiom_route_compute.o axiom_route_delivery.o axiom_route_set.o $(AXIOM_SIM)/axiom_net_switch.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

axiom_discovery_protocol_test: axiom_discovery_protocol_test.o $(AXIOM_SIM)/axiom_simulator.o $(AXIOM_SIM)/axiom_nic_simulator.o axiom_node_code.o  axiom_discovery_protocol.o axiom_route_compute.o axiom_route_delivery.o axiom_route_set.o $(AXIOM_SIM)/axiom_net_socketpair.o

axiom_discovery_protocol_test.o: axiom_discovery_protocol_test.c $(AXIOM_SIM)/axiom_simulator.h axiom_discovery_protocol.h

axiom_simulator.o: $(AXIOM_SIM)/axiom_simulator.c $(AXIOM_SIM)/axiom_simulator.h axiom_route_compute.h

axiom_net_socketpair.o: $(AXIOM_SIM)/axiom_net_socketpair.c $(AXIOM_SIM)/axiom_net.h

axiom_net_switch.o: $(AXIOM_SIM)/axiom_net_switch.c $(AXIOM_SIM)/axiom_net.h

axiom_node_code.o: axiom_node_code.c axiom_node_code.h axiom_route_compute.h axiom_route_delivery.h axiom_route_set.h axiom_discovery_protocol.h

axiom_discovery_protocol.o: axiom_discovery_protocol.c axiom_discovery_protocol.h

axiom_nic_simulator.o: $(AXIOM_SIM)/axiom_nic_simulator.c $(AXIOM_SIM)/axiom_simulator.h

axiom_route_compute.o: axiom_route_compute.c axiom_discovery_protocol.h axiom_route_compute.h

axiom_route_delivery.o: axiom_route_delivery.c axiom_route_delivery.h axiom_discovery_protocol.h axiom_route_compute.h

axiom_route_set.o: axiom_route_set.c axiom_discovery_protocol.h axiom_route_compute.h axiom_route_set.h

clean:
	rm -rf $(CLEANFILES)



