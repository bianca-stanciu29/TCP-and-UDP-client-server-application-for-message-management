CFLAGS = -Wall -g

PORT = 29101
IP_SERVER = 127.0.0.1

all: server subscriber 

server: server.cpp

subscriber: subscriber.cpp

.PHONY: clean run_server run_client

run_server:
	./server ${PORT}

run_client:
	./subscriber ${ID_CLIENT} ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber
