CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lm

SRCS = main.c dosews.c filter.c trigger.c integrazione.c allarme.c output.c
OBJS = $(SRCS:.c=.o)
TARGET = dosews

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

main.o: main.c dosews.h output.h
	$(CC) $(CFLAGS) -c main.c

dosews.o: dosews.c dosews.h filter.h trigger.h integrazione.h allarme.h output.h
	$(CC) $(CFLAGS) -c dosews.c

filter.o: filter.c filter.h
	$(CC) $(CFLAGS) -c filter.c

trigger.o: trigger.c trigger.h
	$(CC) $(CFLAGS) -c trigger.c

integrazione.o: integrazione.c integrazione.h
	$(CC) $(CFLAGS) -c integrazione.c

allarme.o: allarme.c allarme.h
	$(CC) $(CFLAGS) -c allarme.c

output.o: output.c output.h
	$(CC) $(CFLAGS) -c output.c

clean:
	rm -f $(OBJS) $(TARGET) allarme_report.txt

.PHONY: clean
