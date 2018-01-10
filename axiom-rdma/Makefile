
APPS:=axiom-rdma

include ../simple.mk

CFLAGS+=$(call PKG-CFLAGS, axiom_allocator evi_lmm axiom_run_api)
LDFLAGS+=$(call PKG-LDFLAGS, axiom_allocator evi_lmm axiom_run_api)
LDLIBS+=$(call PKG-LDLIBS, axiom_allocator evi_lmm axiom_run_api)

axiom-rdma: $(OBJS)
