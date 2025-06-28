CC = gcc
CFLAGS = -Wall -pedantic -std=c11
LDFLAGS = -lrt
OBJECTS_SERVER = server.o file_parser/parse_rescuers.o file_parser/parse_emergency_types.o file_parser/parse_env.o file_thread/barrier.o file_thread/handler_emergency.o write_log.o
OBJECTS_CLIENT = client.o file_parser/parse_request.o file_thread/thread_client.o write_log.o

.PHONY: clean all run

run: server
	./server

all:server client

client: $(OBJECTS_CLIENT)
	$(CC) $(CFLAGS) $(OBJECTS_CLIENT) -o client $(LDFLAGS)

server: $(OBJECTS_SERVER)
	$(CC) $(CFLAGS) $(OBJECTS_SERVER) -o server $(LDFLAGS)

server.o: server.c file_header/parse_rescuers.h file_header/parse_emergency_types.h file_header/parse_env.h write_log.o
	$(CC) $(CFLAGS) -c server.c -o server.o

parse_env.o: file_parser/parse_env.c file_header/parse_rescuers.h write_log.o
	$(CC) $(CFLAGS) -c file_parser/parse_env.c -o file_parser/parse_env.o

parse_rescuers.o: file_parser/parse_rescuers.c file_header/parse_rescuers.h write_log.o
	$(CC) $(CFLAGS) -c file_parser/parse_rescuers.c -o file_parser/parse_rescuers.o

parse_emergency_types.o: file_parser/parse_emergency_types.c file_header/parse_emergency_types.h write_log.o
	$(CC) $(CFLAGS) -c file_parser/parse_emergency_types.c -o file_parser/parse_emergency_types.o

parse_request.o: file_parser/parse_request.c file_header/parse_request.h write_log.o
	$(CC) $(CFLAGS) -c file_parser/parse_request.c -o file_parser/parse_request.o

thread_client.o: file_thread/thread_client.c file_header/thread_client.h write_log.o
	$(CC) $(CFLAGS) -c file_thread/thread_client.c -o file_thread/thread_client.o 

barrier.o: file_thread/barrier.c file_header/barrier.h write_log.o 
	$(CC) $(CFLAGS) -c file_thread/barrier.c -o file_thread/thread_client.o 

handler_emergency.o: file_thread/handler_emergency.c file_header/thread_server.h
	$(CC) $(CFLAGS) -c file_thread/handler_emergency.c -o file_thread/handler_emergency.o

write_log.o: write_log.c file_header/write_log.h 
	$(CC) $(CFLAGS) -c write_log.c -o write_log.o


clean:
	rm -f $(OBJECTS_SERVER) $(OBJECTS_CLIENT) server client

