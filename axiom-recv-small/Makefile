AXIOM_NIC_UL := ../../axiom-nic/axiom_user_library

APPS := axiom_recv_small
CLEANFILES = $(APPS) *.o
CFLAGS += -Wall -g -DPDEBUG -I../../axiom-docs/include/


.PHONY: all clean

all: $(APPS)

axiom_recv_small: axiom_recv_small.o $(AXIOM_NIC_UL)/axiom_user_api.o

axiom_recv_small.o: axiom_recv_small.c

clean:
	rm -rf $(CLEANFILES)



