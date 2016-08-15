MODNAME = ns

#KERNELDIR ?= /usr/src/kernels/linux-3.14.22
KERNELDIR := /work/wifi/bbb/ath_u11/linux/kernels/mips-linux-2.6.31

obj-m := $(MODNAME).o
$(MODNAME)-objs = do.o list.o

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	@echo "Cleanup device driver..."
	@rm -rf *.o *.ko *.mod.* .$(MODNAME).* .tmp* module* Module*
	@echo
