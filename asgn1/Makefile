CC       = clang
CFLAGS   = -Wall -Wpedantic -Werror -Wextra
EXECBIN  = memory
SOURCES  = $(wildcard *.c)
OBJECTS  = $(SOURCES:%.c=%.o)

.PHONY: all clean

all: $(EXECBIN)

$(EXECBIN): $(OBJECTS)
	$(CC) -o $@ $^

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(EXECBIN) $(OBJECTS)
