

#KDIR := /home/langyanjun/work/openWRT_K/gen/openwrt/build_dir/target-i386_i486_glibc-2.21/linux-x86_generic/linux-3.18.36
KDIR := /work/wifi/openWRT3/openwrt/build_dir/target-i386_i486_glibc-2.21/linux-x86_generic/linux-3.18.36
#KDIR :=/home/langyanjun/work/MTK/wrt_kochab/kop/yop/op/trunk/target/linux/linux-3.10.14
PWD := $(shell pwd)


all:mod-up

# need add PATH at  ~/.bashrc file
# export PATH=$PATH:/home/langyanjun/work/openWRT_K/gen/openwrt/staging_dir/toolchain-i386_i486_gcc-4.8-linaro_glibc-2.21/bin
#

CC=i486-openwrt-linux-gcc
#CC=mipsel-openwrt-linux-gcc

mod-up:                                    
	@echo ======CC=$(CC)=====
	make -C $(KDIR) M=$(PWD)/mod_up modules
dns-intercept:                                    
	@echo ======CC=$(CC)=====
	make -C $(KDIR) M=$(PWD)/dns_intercept modules
skbb:                                    
	@echo ======CC=$(CC)=====
	make -C $(KDIR) M=$(PWD)/skb modules

clean:
	make -C $(PWD)/mod_up clean
	make -C $(PWD)/dns_intercept clean
	make -C $(PWD)/skb clean
