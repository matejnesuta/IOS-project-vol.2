CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic
#LDLIBS = -pthread -lr
EXECUTABLE = proj2
INPUT = proj2.c
ZIP_FILE = xnesut00.zip

all:
#	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)
	$(CC) $(INPUT) $(CFLAGS) -o $(EXECUTABLE)

run:
	./$(EXECUTABLE) $(ARGS)

pack: $(ZIP_FILE)

$(ZIP_FILE): *.c Makefile 
	zip -j $@ $^

valgrind:
	valgrind ./$(EXECUTABLE) $(ARGS)

leaks: $(EXECUTABLE)
	valgrind --track-origins=yes --leak-check=full \
	--show-reachable=yes ./$(EXECUTABLE) $(ARGS)

clean:
	rm -r $(EXECUTABLE)
	rm vgcore*
	rm -r *zip