
libsqlite3.so:sqlite3.o
	$(CC) -shared sqlite3.o -o libsqlite3.so -fPIC	\

sqlite3.o: sqlite3.c
	$(CC) -c sqlite3.c -lpthread -ldl -Wall -g -o sqlite3.o -O -fPIC

clean:
	rm -rvf sqlite3.so\
			sqlite3.o
	

