#!/bin/bash
#SBATCH -J TSP_Master_Run
#SBATCH -p compute2011
#SBATCH --ntasks=24
#SBATCH --cpus-per-task=1
#SBATCH -t 00:45:00
#SBATCH -o scaling_output.txt

module load mpi/openmpi/4.1.0-no_ucx

# Colors for readability
GREEN='\033[0;32m'
NC='\033[0m'

echo -e "${GREEN}===================================================${NC}"
echo -e "${GREEN}   BERLIN52: FULL PROJECT RUN (Scaling + Route)    ${NC}"
echo -e "${GREEN}===================================================${NC}"

# --- 1. SETUP FILES ---
# Try copying from the official cluster folder first [cite: 58]
echo "Setting up files..."
cp /work/korzec/LAB2/proj/berlin52.tsp . 2>/dev/null
cp /work/korzec/LAB2/proj/berlin52.opt.tour . 2>/dev/null

# Fallback: Check if copy succeeded, if not, warn user (or create if you prefer)
if [ ! -f berlin52.tsp ]; then
    echo "❌ Error: berlin52.tsp not found. Please ensure you are on the cluster."
    # (Optional: Insert the 'cat' creation block here if you want it to be 100% fail-safe)
    exit 1
fi

# --- 2. COMPILE ---
echo "Compiling C Code..."
mpicc -O3 tsp_mpi.c -o tsp_solver -lm
if [ $? -ne 0 ]; then
    echo "❌ Compilation Failed!"
    exit 1
fi

# --- 3. STRONG SCALING LOOP ---
echo "Cores,TotalTime,BestDist" > scaling_results.csv

for P in 1 2 4 8 12 24; do
    echo "---------------------------------------------------"
    echo -e "Running with ${GREEN}$P processors...${NC}"
    
    # Clean previous output to ensure we get fresh data
    rm -f solution.txt
    
    # Run MPI Job (The C code automatically saves 'my_route.txt' every time)
    mpirun -np $P --map-by node ./tsp_solver berlin52.tsp | tee run_P${P}.log
    
    # Extract Data from logs
    time=$(grep "Simulation Time" run_P${P}.log | awk '{print $3}')
    
    if [ -f solution.txt ]; then
        dist=$(cat solution.txt)
    else
        dist="ERR"
    fi
    
    echo "   -> Runtime: ${time}s"
    echo "   -> Quality: ${dist}"
    
    # Save to CSV for the graphs
    echo "$P,$time,$dist" >> scaling_results.csv
done

echo -e "${GREEN}===================================================${NC}"
echo -e "${GREEN}✅ ALL DONE.${NC}"
echo "1. Scaling data saved to: scaling_results.csv"
echo "2. Best Route saved to:   my_route.txt (from the last run)"
echo "3. Logs saved to:         run_P*.log"
echo -e "${GREEN}===================================================${NC}"
