CC=gcc
CFLAGS=-Wall -Wextra -Wstrict-prototypes -o3 -pthread
SOURCES=../thpool.c benchmark.c
HEADERS=../thpool.h
SOURCES_OLD=thpool_old.c benchmark_old.c
HEADERS_OLD=thpool_old.h

.PHONY: clean

all: benchmark benchmark_old
	python3 run_benchmark.py

benchmark: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SOURCES)

benchmark_old: $(SOURCES_OLD) $(HEADERS_OLD)
	$(CC) $(CFLAGS) -o $@ $(SOURCES_OLD)

clean:
	rm -f benchmark benchmark_old benchmark*.csv benchmark*.png
