CFLAGS+=-ggdb -DTHPOOL_DEBUG
LDFLAGS+=-lpthread
example: example.o thpool.o
