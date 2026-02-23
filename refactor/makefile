CC = gcc
CFLAGS = -Wall -O2

dosews: main.o trigger.o filter.o output.o integrazione.o allarme.o
	$(CC) $(CFLAGS) -o dosews main.o trigger.o filter.o output.o integrazione.o allarme.o -lm

main.o: main.c trigger.h filter.h output.h integrazione.h allarme.h
	$(CC) $(CFLAGS) -c main.c

trigger.o: trigger.c trigger.h
	$(CC) $(CFLAGS) -c trigger.c

filter.o: filter.c filter.h
	$(CC) $(CFLAGS) -c filter.c

output.o: output.c output.h
	$(CC) $(CFLAGS) -c output.c

integrazione.o: integrazione.c integrazione.h
	$(CC) $(CFLAGS) -c integrazione.c

allarme.o: allarme.c allarme.h
	$(CC) $(CFLAGS) -c allarme.c

clean:
	rm -f *.o dosews

.PHONY: clean