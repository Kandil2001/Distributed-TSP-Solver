CC = mpicc
CFLAGS = -O3 -std=c99 -Wall -Wextra
LDLIBS = -lm
TARGET = tsp_solver
SOURCE = script/tsp_mpi.c

.PHONY: all verify clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET) $(LDLIBS)

verify: $(TARGET)
	mpirun -np 1 ./$(TARGET) data/berlin52.tsp --evaluate-tour data/berlin52.opt.tour

clean:
	rm -f $(TARGET) *.o solution.txt my_route.txt
