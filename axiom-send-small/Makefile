AXIOM_NIC_UL := ../../axiom-nic/axiom_user_library

APPS := axiom-send-small
CLEANFILES = $(APPS) *.o
CFLAGS += -Wall -g -DPDEBUG -I../../axiom-docs/include/


.PHONY: all clean

all: $(APPS)

axiom-send-small: axiom-send-small.o $(AXIOM_NIC_UL)/axiom_user_api.o

axiom-send-small.o: axiom-send-small.c

clean:
	rm -rf $(CLEANFILES)



