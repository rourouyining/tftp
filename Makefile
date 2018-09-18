CC=gcc

obj = tftp_client.o

CFLAGS=-Wall -g 
LIB=
INCLUDE=

target= tftp_client

all:$(obj)
	$(CC) $^ -o $(target) $(LIB)
%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE)

clean:
	rm -rf *.o $(target)
