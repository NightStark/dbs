TARGET=des

all:$(TARGET)

CFLAGS=-lcrypto
OBJS=des_ncbc.o base64.o

$(TARGET):$(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o $(TARGET)

clean:
	rm -rfv *.o
	rm -rfv $(TARGET)
