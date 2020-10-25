CC = cc -pedantic -std=c11
PTCC = cc -pedantic -pthread -g
PTRUN = srun -c
LINKS = -lm

all: server client

server: Programming2_Server.c DieWithError.c ProcessThread.c StringList.c Programming2.h StringList.h
	$(PTCC) -o server Programming2_Server.c ProcessThread.c DieWithError.c StringList.c $(LINKS)

client: Programming2_Client.c DieWithError.c ProcessThread.c StringList.c Programming2.h StringList.h
	$(PTCC) -o client Programming2_Client.c ProcessThread.c DieWithError.c StringList.c $(LINKS)

clean :
	rm -f server client download.txt
