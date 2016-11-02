server: server.o client.o
		gcc -o3 -o server -g server.o  -lm -lpthread
		gcc -o3 -o client -g client.o  -lm -lpthread
server.o: client.c  server.c
		gcc  -g -c server.c
		gcc  -g -c client.c


clean:
		rm -f *.o server
		rm -f *.o client
