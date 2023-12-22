CC=gcc

all: proxy.out

proxy.out: picohttpparser.o proxy.c data_storage.c thread_sync_pipe.o
	$(CC) -Werror -Wpedantic -Wall picohttpparser.o thread_sync_pipe.o data_storage.c proxy.c -lpthread -o proxy.out

picohttpparser.o: picohttpparser-master/picohttpparser.c
	$(CC) -Werror -Wpedantic -Wall picohttpparser-master/picohttpparser.c -c

thread_sync_pipe.o: thread_sync_pipe
	$(CC) -Werror -Wpedantic -Wall thread_sync_pipe/thread_sync_pipe.c -c
