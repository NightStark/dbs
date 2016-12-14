MODNAME = tf

obj-m := $(MODNAME).o
$(MODNAME)-objs = tcp_filter.o

all:default

clean:
	rm -rf *.o .*.cmd .cmd *.ko *.mod.c .tmp_versions modules.order Module.symvers

