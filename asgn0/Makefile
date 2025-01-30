EXECBIN  = hello
SOURCES  = $(wildcard *.c)
OBJECTS  = $(SOURCES:%.c=%.o)
FORMATS  = $(SOURCES:%.c=%.fmt)

CC       = clang
FORMAT   = clang-formatm
CFLAGS   = -Wall -Wpedantic -Werror -Wextra

.PHONY: all clean format hello

all: $(EXECBIN)

#hello: $(EXECBIN)

$(EXECBIN): $(OBJECTS)
	$(CC) -o $@ $^

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(EXECBIN) $(OBJECTS) $(FORMATS)

format: $(FORMATS)

%.fmt: %.c
	$(FORMAT) -i $<
	touch $@

