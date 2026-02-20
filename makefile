all: client

client: client.c monitor.c packetBuffer.c packetQueue.c socketConnection.c
	gcc -pthread -std=c99 -D_GNU_SOURCE client.c monitor.c packetBuffer.c packetQueue.c socketConnection.c -o client

clean:
	rm -f client
