CC = gcc
CFLAGS = -Wall -std=c11 -g -I..
LDFLAGS = -lm

SRC = ../streaming_kmeans.c
CWRU_CSV = cwru/features.csv
VENV = cwru/.venv
PYTHON = $(VENV)/bin/python3
PIP = $(VENV)/bin/pip

.PHONY: all test test-cwru test-all clean clean-venv setup-cwru

all: test

# Build targets
test_kmeans: test_kmeans.c $(SRC)
	$(CC) $(CFLAGS) -o $@ test_kmeans.c $(SRC) $(LDFLAGS)

test_hitl: test_hitl.c $(SRC)
	$(CC) $(CFLAGS) -o $@ test_hitl.c $(SRC) $(LDFLAGS)

test_cwru: test_cwru_simulation.c $(SRC)
	$(CC) $(CFLAGS) -o $@ test_cwru_simulation.c $(SRC) $(LDFLAGS)

# Core tests (CI - fast, no external data)
test: test_kmeans test_hitl
	@echo "=== K-means tests ==="
	./test_kmeans
	@echo ""
	@echo "=== HITL tests ==="
	./test_hitl
	@echo ""
	@echo "=== Core tests passed ==="

# Create venv if missing
$(VENV):
	@echo "Creating Python venv..."
	python3 -m venv $(VENV)
	$(PIP) install --upgrade pip -q
	$(PIP) install -r cwru/requirements.txt -q
	@echo "✓ venv ready"

# Download and extract CWRU data
setup-cwru: $(VENV)
	@echo "=== Setting up CWRU dataset ==="
	$(PYTHON) cwru/download.py
	$(PYTHON) cwru/extract_features.py
	@echo "=== CWRU setup complete ==="

# Auto-setup if features.csv missing
$(CWRU_CSV): $(VENV)
	@echo "CWRU features not found. Running setup..."
	$(MAKE) setup-cwru

# CWRU benchmark (requires real data)
test-cwru: test_cwru $(CWRU_CSV)
	@echo "=== CWRU Benchmark ==="
	./test_cwru
	@echo ""

# Full suite
test-all: test test-cwru
	@echo "=== All tests passed ==="

clean:
	rm -f test_kmeans test_hitl test_cwru
	rm -f cwru/features.csv
	rm -rf cwru/cache/

clean-venv: clean
	rm -rf $(VENV)