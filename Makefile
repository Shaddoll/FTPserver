server: server.c mySocket.h myConst.h myStdlib.h myExecute.h
	gcc -Wall -o server server.c -lpthread
clean:
	rm server
