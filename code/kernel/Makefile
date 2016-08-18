
KDIR := /home/langyanjun/work/openWRT_K/gen/openwrt/build_dir/target-i386_i486_glibc-2.21/linux-x86_generic/linux-3.18.36
PWD := $(shell pwd)

all:mod-up

mod-up:                                    
	make -C $(KDIR) M=$(PWD)/mod_up modules

clean:
	make -C $(PWD)/mod_up clean
