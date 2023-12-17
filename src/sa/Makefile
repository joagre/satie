CC := gcc
CFLAGS := -Wall -Wextra -I../clib -Wfatal-errors -std=c17 -g -D_GNU_SOURCE -Wno-unused-value -DDEBUG -pg #-O3 # -pg # -DDEBUG #-DUNITTEST

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)
EXEC := ../../bin/sa

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

-include $(DEPS)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -f $(OBJS) $(DEPS) $(EXEC)

mrproper: clean
	find . \( -name "*~" -or -name '#*' -or -name '.#*' \) -exec rm {} \;;

.PHONY: all clean