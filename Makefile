TARGET = sudoku

OBJECTS = src/main.o
LIBS =
CFLAGS = -Wreturn-type -Wunused-function

CC = cc
CFLAGS = -std=c11 -Wno-pointer-sign -pedantic-errors
LD = $(CC)

release: CFLAGS += -DNDEBUG -O2
debug: CFLAGS += -g
debug: LDFLAGS += -rdynamic

release debug: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -f $(TARGET) $(OBJECTS)
