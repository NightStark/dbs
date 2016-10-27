all:pt tr 

CC=mips-linux-gcc
pt: pt.c a2l.c bucomm.c filemode.c libbfd.a
	$(CC) pt.c a2l.c bucomm.c filemode.c  -lbfd -L. -liberty -I /home/langyanjun/work/t/binutils-2.19.1/bfd -I /home/langyanjun/work/t/binutils-2.19.1/include/ -o pt -Wall
	#$(CC) pt.c -o pt -Wall
	mips-linux-strip pt
tr: tl.c track.c
	$(CC) tl.c -fPIC -shared -o libtl.so -rdynamic -g
	$(CC) track.c -L. -ltl -o tr -rdynamic 
	#$(CC) track.c tl.c -o tr
#mips-linux-gcc bfd_t.c -lbfd -L. -liberty
a2l: a2l.c bucomm.c filemode.c libbfd.a
	mips-linux-gcc a2l.c bucomm.c filemode.c  -lbfd -L. -liberty -I /home/langyanjun/work/t/binutils-2.19.1/bfd -I /home/langyanjun/work/t/binutils-2.19.1/include/ -o a2l
	mips-linux-strip a2l
relf: readelf.c bucomm.c filemode.c libbfd.a
	mips-linux-gcc readelf.c bucomm.c filemode.c  -lbfd -L. -liberty -I /home/langyanjun/work/t/binutils-2.19.1/bfd -I /home/langyanjun/work/t/binutils-2.19.1/include/ -I /home/langyanjun/work/t/binutils-2.19.1/elfcpp/ -o relf
clean:
	rm *.o pt tr *.so
