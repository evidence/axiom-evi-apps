AXIOM_INCLUDE := ../../../../axiom-evi-nic/include
AXIOM_DISCOVERY := ../
AXIOM_SIM := axiom_simulator

APPS := axiom_discovery_protocol_test axsw_discovery_protocol_test
HEADERS := $(AXIOM_INCLUDE)/*.h
CLEANFILES = $(APPS) *.o $(AXIOM_SIM)/*.o $(AXIOM_DISCOVERY)/*.o

DFLAGS := -g -DPDEBUG
CFLAGS += -Wall $(DFLAGS) -I$(AXIOM_INCLUDE) -I./$(AXIOM_SIM) -I$(AXIOM_DISCOVERY)
LDLIBS += -pthread -lm

PWD := $(shell pwd)
#MKFILE_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
#CCARCH := arm
CCARCH := aarch64
BUILDROOT := ${PWD}/../../../../axiom-evi-buildroot
DESTDIR := ${BUILDROOT}/output/target
CCPREFIX := ${BUILDROOT}/output/host/usr/bin/$(CCARCH)-linux-
CC := ${CCPREFIX}gcc


.PHONY: all clean

all: $(APPS)

axsw_discovery_protocol_test: axiom_discovery_protocol_test.o $(AXIOM_SIM)/axiom_simulator.o $(AXIOM_SIM)/axiom_nic_simulator.o $(AXIOM_DISCOVERY)/axiom_discovery_node.o $(AXIOM_DISCOVERY)/axiom_discovery_protocol.o $(AXIOM_DISCOVERY)/axiom_route_compute.o $(AXIOM_DISCOVERY)/axiom_route_delivery.o $(AXIOM_DISCOVERY)/axiom_route_set.o $(AXIOM_SIM)/axiom_net_switch.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDLIBS)

axiom_discovery_protocol_test: axiom_discovery_protocol_test.o $(AXIOM_SIM)/axiom_simulator.o $(AXIOM_SIM)/axiom_nic_simulator.o $(AXIOM_DISCOVERY)/axiom_discovery_node.o  $(AXIOM_DISCOVERY)/axiom_discovery_protocol.o $(AXIOM_DISCOVERY)/axiom_route_compute.o $(AXIOM_DISCOVERY)/axiom_route_delivery.o $(AXIOM_DISCOVERY)/axiom_route_set.o $(AXIOM_SIM)/axiom_net_socketpair.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDLIBS)


axiom_discovery_protocol_test.o: axiom_discovery_protocol_test.c $(AXIOM_SIM)/axiom_simulator.h $(AXIOM_DISCOVERY)/axiom_discovery_protocol.h $(HEADERS)


$(AXIOM_SIM)/axiom_simulator.o: $(AXIOM_SIM)/axiom_simulator.c $(AXIOM_SIM)/axiom_simulator.h $(AXIOM_DISCOVERY)/axiom_route_compute.h $(HEADERS)

$(AXIOM_SIM)/axiom_net_socketpair.o: $(AXIOM_SIM)/axiom_net_socketpair.c $(AXIOM_SIM)/axiom_net.h $(HEADERS)

$(AXIOM_SIM)/axiom_net_switch.o: $(AXIOM_SIM)/axiom_net_switch.c $(AXIOM_SIM)/axiom_net.h $(HEADERS)

$(AXIOM_SIM)/axiom_nic_simulator.o: $(AXIOM_SIM)/axiom_nic_simulator.c $(AXIOM_SIM)/axiom_simulator.h $(HEADERS)


clean:
	rm -rf $(CLEANFILES)



