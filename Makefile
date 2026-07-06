CC = mpicc
CFLAGS = -O3 -std=c99 -Wall -Wextra
LDLIBS = -lm
BIN_DIR := bin
TARGET = $(BIN_DIR)/tsp_solver
SOURCE = script/tsp_mpi.c

.PHONY: all clean run-example

all: $(TARGET)

$(TARGET): $(SOURCE)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET) $(LDLIBS)

run-example: $(TARGET)
	@echo "Example:"
	@echo "  mpirun --allow-run-as-root -np 2 $(TARGET) data/berlin52.tsp --replicas 4 --steps 10 --deterministic --swap-interval 2"

clean:
	rm -f $(TARGET) *.o
