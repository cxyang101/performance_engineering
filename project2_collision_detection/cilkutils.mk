ifeq ($(CILKSAN),1)
CFLAGS += -fsanitize=cilk
LDFLAGS += -fsanitize=cilk
else ifeq ($(CILKSCALE),1)
CFLAGS += -fcilktool=cilkscale
LDFLAGS += -fcilktool=cilkscale
endif
