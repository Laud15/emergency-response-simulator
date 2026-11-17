CC = gcc
CFLAGS = -Wall -pedantic -std=c11
LDFLAGS = -lrt
OBJECTS_SERVER = server.o parsers/parse_rescuers.o parsers/parse_emergency_types.o parsers/parse_env.o threads/barrier.o threads/handler_emergency.o write_log.o
OBJECTS_CLIENT = client.o parsers/parse_request.o parsers/parse_env.o write_log.o

.PHONY: clean all run

run: server
	./server

all:server client

client: $(OBJECTS_CLIENT)
	$(CC) $(CFLAGS) $(OBJECTS_CLIENT) -o client $(LDFLAGS)

server: $(OBJECTS_SERVER)
	$(CC) $(CFLAGS) $(OBJECTS_SERVER) -o server $(LDFLAGS)

client.o: client.c include/parse_env.h write_log.o
	$(CC) $(CFLAGS) -c client.c -o client.o

server.o: server.c include/parse_rescuers.h include/parse_emergency_types.h include/parse_env.h write_log.o
	$(CC) $(CFLAGS) -c server.c -o server.o

parse_env.o: parsers/parse_env.c include/parse_rescuers.h write_log.o
	$(CC) $(CFLAGS) -c parsers/parse_env.c -o parsers/parse_env.o

parse_rescuers.o: parsers/parse_rescuers.c include/parse_rescuers.h write_log.o
	$(CC) $(CFLAGS) -c parsers/parse_rescuers.c -o parsers/parse_rescuers.o

parse_emergency_types.o: parsers/parse_emergency_types.c include/parse_emergency_types.h write_log.o
	$(CC) $(CFLAGS) -c parsers/parse_emergency_types.c -o parsers/parse_emergency_types.o

parse_request.o: parsers/parse_request.c include/parse_request.h write_log.o
	$(CC) $(CFLAGS) -c parsers/parse_request.c -o parsers/parse_request.o

barrier.o: threads/barrier.c include/barrier.h write_log.o
	$(CC) $(CFLAGS) -c threads/barrier.c -o threads/barrier.o

handler_emergency.o: threads/handler_emergency.c include/handler_emergency.h
	$(CC) $(CFLAGS) -c threads/handler_emergency.c -o threads/handler_emergency.o

write_log.o: write_log.c include/write_log.h
	$(CC) $(CFLAGS) -c write_log.c -o write_log.o


clean:
	rm -f $(OBJECTS_SERVER) $(OBJECTS_CLIENT) server client

