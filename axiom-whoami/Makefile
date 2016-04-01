AXIOM_NIC_UL := ../../axiom-nic/axiom_user_library

APPS := axiom-whoami
CLEANFILES = $(APPS) *.o
CFLAGS += -Wall -g -DPDEBUG -I../../axiom-docs/include/


.PHONY: all clean

all: $(APPS)

axiom-whoami: axiom-whoami.o $(AXIOM_NIC_UL)/axiom_user_api.o

axiom-whoami.o: axiom-whoami.c

clean:
	rm -rf $(CLEANFILES)



