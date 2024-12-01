.PHONY: all
all: server client

server: queue_adt.o server.o
	gcc -o $@ $^

client: read_csv.o client.o
	gcc -o $@ $^

.o: %.c
	gcc -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o server client