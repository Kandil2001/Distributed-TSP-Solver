CC = mpicc
CFLAGS = -O3 -std=c99 -Wall -Wextra
LDLIBS = -lm
TARGET = tsp_solver
SOURCE = script/tsp_mpi.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET) $(LDLIBS)

clean:
	rm -f $(TARGET) *.o
