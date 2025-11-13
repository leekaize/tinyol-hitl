CC = gcc
CFLAGS = -Wall -Werror -std=c11 -g
LDFLAGS = -lm

SRC_DIR = ..
SRC = $(SRC_DIR)/streaming_kmeans.c

.PHONY: all test clean

all: test

test: test_kmeans test_hitl
	@echo "Running k-means tests..."
	./test_kmeans
	@echo ""
	@echo "Running HITL tests..."
	./test_hitl
	@echo ""
	@echo "All tests passed!"

test_kmeans: test_kmeans.c $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_hitl: test_hitl.c $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f test_kmeans test_hitl

watch:
	@echo "Watching for changes..."
	@while true; do \
		make test; \
		inotifywait -qe modify $(SRC) *.c; \
	done