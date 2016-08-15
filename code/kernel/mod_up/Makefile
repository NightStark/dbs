MODNAME = mod-up

obj-m := $(MODNAME).o
$(MODNAME)-objs = up.o

#ARCH := mips                           
#CC=mips-linux-gcc
#CXX   :=mips-linux-c++
#STRIP :=mips-linux-strip
#AR    :=mips-linux-ar
#LD    :=mips-linux-ld
#CROSS_COMPILE :=mips-linux-
#CROSS :=mips-linux-

#KERNELDIR := /work/wifi/bbb/ath_u11/linux/kernels/mips-linux-2.6.31
#KERNELDIR := /usr/src/kernels/3.10.0-327.13.1.el7.x86_64

#mod-up:                                                      
#	make -C $(KERNELDIR) M=$(PWD) modules

#all: mod-up

all:default

clean:
	rm -rf *.o .*.cmd .cmd *.ko *.mod.c .tmp_versions modules.order Module.symvers

############################################
#in fils:ath_u11/linux/modules
#mod-up:                                    
#	make -C $(KDIR) M=$(PWD)/mod_up modules
