VERSION     = 0
SUBVERSION  = 1
DEVEVERSION = 1

TEST = OFF
NSDB_SYS = nsdb_sys

#动态库名
TARGET_SO_NAME 	  = nsdb
#动态库文件名
TARGET_SO_FILE    = lib$(TARGET_SO_NAME).so
#服务端执行文件名
TARGET_MAIN 	  = Server
#客户端执行文件名				
TARGET_NSDB_C  	  = Client

TARGET_MOD_UP     = mod-up
TARGET_MOD_UP_CLEAN     = mod-up-clean

ifeq ($(TEST), ON)
	#测试程序执行文件名				
	TARGET_NSDB_TEST  = Test
	#测试项动态库名				
	TARGET_SO_TEST_T  = TestT
endif

#测试项动态库文件名					
TARGET_SO_TEST_T_FILE = lib$(TARGET_SO_TEST_T).so
#WEB服务器执行文件
TARGET_WEB_SRV	      = WebServer
#sqlite3
SQLITE3 = sqlite3
SQLITE3_SO_FILE = lib$(SQLITE3).so

#测试代码简单独立层序
TARGET_CTEST = codetest

TARGET_APP_NS_LOG = nslog
TARGET_APPS += $(TARGET_APP_NS_LOG)

##############################################
#debug线程相关
#debug动态库名
TARGET_DEBUG_SO_NAME = debug
#debug动态库文件名
TARGET_DEBUG_SO_FILE = lib$(TARGET_DEBUG_SO_NAME).so
#debug程序名
TARGET_DEBUG_MAIN = debug
##############################################

#加入到目标集
TARGET  = $(TARGET_SO_FILE)
TARGET += $(TARGET_MAIN)
#TARGET += $(TARGET_NSDB_C)
ifeq ($(TEST), ON)
	TARGET += $(TARGET_NSDB_TEST)
	TARGET += $(TARGET_SO_TEST_T_FILE)
endif
TARGET += $(TARGET_WEB_SRV)
#TARGET += $(SQLITE3)

TARGET += $(TARGET_DEBUG_SO_FILE)
TARGET += $(TARGET_DEBUG_MAIN)
TARGET += $(TARGET_CTEST)
TARGET += $(TARGET_APPS)

ifeq ($(ARCH),arm) 
	CROSS=arm-linux-
endif
ifeq ($(ARCH),i686)
	CROSS=/opt/tools/bin/i686-pc-linux-gnu-
else 
    CROSS=
endif 
#CROSS=mips-linux-
 
CC=$(CROSS)gcc
LD=$(CROSS)ld

export CC
export LD

#-fpic : 位置无关码就是可以在进程的任意内存位置执行的目标码
#CFLAGS = -Wall -lpthread -I include/lib/ -lm -lrt -ggdb -Wall 
CFLAGS = -Wall -lpthread -I include/lib/ -lm -lrt -ggdb -Wall -DNS_EVENTFD -DNS_TIMERFD -lcrypto -lssl 
CFLAGS += -lmysqlclient -L/usr/lib64/mysql/
#OBJ_FLAGS =  -g -lpthread -I include/lib/ -fpic -lm -Wall 
OBJ_FLAGS =  -ggdb -lpthread -I include/lib/ -fpic -lm -Wall -DNS_EVENTFD -DNS_TIMERFD -lcrypto -lssl 

DEBUG = y


SRC_NSDB 	=
SRC_NSDB_C  =
SRC_MAIN 	= 

SRC_DEBUG_SO	=
SRC_DEBUG_MAIN  =
ifeq ($(TEST), ON)
	SRC_TEST = 
	SRC_TEST_T = #测试项源代码
endif

SRC_WEBS  =
SRC_CTEST =
#获取目标文件名
OBJ_NSDB 	= $(patsubst %c,%o,$(SRC_NSDB))
OBJ_NSDB_C  = $(patsubst %c,%o,$(SRC_NSDB_C))
OBJ_MAIN 	= $(patsubst %c,%o,$(SRC_MAIN))
ifeq ($(TEST), ON)
	OBJ_TEST	= $(patsubst %c,%o,$(SRC_TEST))
	OBJ_TEST_T 	= $(patsubst %c,%o,$(SRC_TEST_T))
endif

OBJ_WEBS        = $(patsubst %c,%o,$(SRC_WEBS))
OBJ_DEBUG_SO	= $(patsubst %c,%o,$(SRC_DEBUG_SO))
OBJ_DEBUG_MAIN	= $(patsubst %c,%o,$(SRC_DEBUG_MAIN))
OBJ_CTEST       = $(patsubst %c,%o,$(SRC_CTEST))
#######################################################
#OBJ_APP
OBJ_APP_NS_LOG  = $(patsubst %c,%o,$(SRC_APP_NSLOG))
#######################################################


#DIR_TREE = $(shell pwd)
DIR_TREE = .

include main/makefile.mk
include lib/makefile.mk
include web/makefile.mk
include app/makefile
include ctest/ctest.mk

NSDB_SYS:$(TARGET)

#-l$(TARGET_SO_NAME) 选项实际为 -lnsdb ,-L. (注意"."的存在，指定动态库的位置)
$(TARGET_MAIN): $(OBJ_MAIN)
	$(CC)  $(OBJ_MAIN) -l$(TARGET_SO_NAME) -L. -o $(TARGET_MAIN) $(CFLAGS)

ifeq ($(TEST), ON)
	#测试
	$(TARGET_NSDB_TEST): $(OBJ_TEST)
		$(CC)  $(OBJ_TEST) -l$(TARGET_SO_NAME) -L. -o $(TARGET_NSDB_TEST) $(CFLAGS) -ldl
	$(TARGET_SO_TEST_T_FILE): $(OBJ_TEST_T)
		$(CC) -shared $(OBJ_TEST_T) -o $(TARGET_SO_TEST_T_FILE) $(CFLAGS)
endif

NS_LIB_PATH     = /usr/lib/ns
NS_LIB_PATH_CFG = /etc/ld.so.conf.d/ns-lib.conf
#吧 OBJ_NSDB 指向的所有.obj 文件都变异到动态库 TARGET_SO_FILE  (libnsdb.so)
$(TARGET_SO_FILE):$(OBJ_NSDB)
	$(CC) -shared $(OBJ_NSDB) -o $(TARGET_SO_FILE) $(CFLAGS)
	echo $(TARGET_SO_FILE) create success ....
	mkdir $(NS_LIB_PATH) -p
	cp ./$(TARGET_SO_FILE) $(NS_LIB_PATH)/
	touch  $(NS_LIB_PATH_CFG)
	echo $(NS_LIB_PATH) > $(NS_LIB_PATH_CFG)
	
$(TARGET_NSDB_C):$(OBJ_NSDB_C)
	$(CC)  $(OBJ_NSDB_C) -o $(TARGET_NSDB_C) $(CFLAGS)
#	cp $(TARGET_NSDB_C) /mnt/hgfs/SFDIR/

$(TARGET_WEB_SRV):$(OBJ_WEBS) $(OBJ_NSDB_C) 
	$(CC)  $(OBJ_WEBS) $(OBJ_NSDB_C) -l$(TARGET_SO_NAME) -L. -o $(TARGET_WEB_SRV) $(CFLAGS) -ldl	
#	cp $(TARGET_SO_FILE) /opt/nfsroot/    
#	cp $(TARGET_WEB_SRV) /opt/nfsroot/

$(SQLITE3):
	(cd lib/sqlite3;$(MAKE)) ;\
	cp lib/sqlite3/$(SQLITE3_SO_FILE) .

$(TARGET_CTEST):$(OBJ_CTEST) $(OBJ_NSDB_C)
	$(CC) $(OBJ_CTEST) -l$(TARGET_SO_NAME) -L. -o $(TARGET_CTEST) $(CFLAGS)
#debug#########################################
$(TARGET_DEBUG_SO_FILE):$(OBJ_DEBUG_SO)
	$(CC) -shared $(OBJ_DEBUG_SO) -o $(TARGET_DEBUG_SO_FILE) $(CFLAGS) 
	
$(TARGET_DEBUG_MAIN):$(OBJ_DEBUG_MAIN) $(TARGET_DEBUG_SO_FILE) $(TARGET_SO_FILE)
	$(CC) $(OBJ_DEBUG_MAIN) -l$(TARGET_SO_NAME) -L. -o $(TARGET_DEBUG_MAIN) $(CFLAGS) 
#################################################
$(TARGET_APP_NS_LOG):$(TARGET_SO_FILE) $(OBJ_APP_NS_LOG)
	@echo --$(TARGET_SO_NAME) --$(OBJ_APP_NS_LOG) --$(SRC_APP_NSLOG)
	$(CC) $(OBJ_APP_NS_LOG) -l$(TARGET_SO_NAME) -L. -o $(TARGET_APP_NS_LOG) $(CFLAGS)
#################################################
$(TARGET_MOD_UP):
	make -C kernel
	cp -rfv kernel/mod_up/mod-up.ko .
	[ -f kernel/mod_up/mod-up.ko ] && sz mod-up.ko
$(TARGET_MOD_UP_CLEAN):
	make -C kernel clean
#################################################
	
%.o:%.c 
	$(CC) -c $< -o $@ $(OBJ_FLAGS) 

#生成的程序名不要和目录名相同，否则目录会被散掉
clean:
	rm $(OBJ_NSDB) 		-rvf;	\
	rm $(OBJ_NSDB_C) 	-rvf; 	\
	rm $(OBJ_MAIN) 		-rvf; 	\
	rm $(OBJ_TEST) 		-rvf; 	\
	rm $(OBJ_TEST_T) 	-rvf; 	\
	rm $(TARGET) 		-rvf; 	\
	rm $(OBJ_WEBS) 		-rvf;   \
	rm $(OBJ_CTEST) 		-rvf;   \
	rm core.*;				\
	find . -name "*.o"   | xargs rm -rfv;	\
	find . -name "*.so"  | xargs rm -rfv;	\
	rm ./log/*				\

cleanweb:
	rm $(OBJ_WEBS) -rvf



