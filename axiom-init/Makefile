AXIOM_INCLUDE := ../../axiom-docs/include
AXIOM_DISC := ../../axiom-discovery
AXIOM_SIM := $(AXIOM_DISC)/axiom_simulator

APPS := axiom_init
CLEANFILES = $(APPS) *.o
CFLAGS += -Wall -g -I../../axiom-docs/include/ -I../../axiom-discovery

.PHONY: all clean

all: $(APPS)

axiom_init: axiom_init.o  $(AXIOM_DISC)/axiom_node_code.o $(AXIOM_DISC)/axiom_discovery_protocol.o $(AXIOM_DISC)/axiom_route_compute.o $(AXIOM_DISC)/axiom_route_delivery.o $(AXIOM_DISC)/axiom_route_set.o

axiom_init.o: axiom_init.c $(AXIOM_INCLUDE)/axiom_nic_types.h $(AXIOM_INCLUDE)/axiom_sim_topology.h $(AXIOM_DISC)/axiom_node_code.h

clean:
	rm -rf $(CLEANFILES)



