AXIOM_NIC_UL := ../../axiom-nic/axiom_user_library

APPS := axiom-recv-small
CLEANFILES = $(APPS) *.o
CFLAGS += -Wall -g -DPDEBUG -I../../axiom-docs/include/


.PHONY: all clean

all: $(APPS)

axiom-recv-small: axiom-recv-small.o $(AXIOM_NIC_UL)/axiom_user_api.o

axiom-recv-small.o: axiom-recv-small.c

clean:
	rm -rf $(CLEANFILES)



