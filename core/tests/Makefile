CC = gcc
CFLAGS = -Wall -std=c11 -g -I..
LDFLAGS = -lm

SRC = ../streaming_kmeans.c

.PHONY: all test clean

all: test

test_kmeans: test_kmeans.c $(SRC)
	$(CC) $(CFLAGS) -o $@ test_kmeans.c $(SRC) $(LDFLAGS)

test_hitl: test_hitl.c $(SRC)
	$(CC) $(CFLAGS) -o $@ test_hitl.c $(SRC) $(LDFLAGS)

test_cwru: test_cwru_simulation.c $(SRC)
	$(CC) $(CFLAGS) -o $@ test_cwru_simulation.c $(SRC) $(LDFLAGS)

test: test_kmeans test_hitl
	@echo "=== Running k-means tests ==="
	./test_kmeans
	@echo ""
	@echo "=== Running HITL tests ==="
	./test_hitl
	@echo ""
	@echo "=== All tests passed ==="

clean:
	rm -f test_kmeans test_hitl test_cwru
