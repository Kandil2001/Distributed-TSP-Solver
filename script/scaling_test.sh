#!/bin/bash
#SBATCH -J tsp_scaling
#SBATCH -p compute2011
#SBATCH --ntasks=24
#SBATCH --cpus-per-task=1
#SBATCH -t 00:45:00
#SBATCH -o scaling_output.txt

set -euo pipefail

module load mpi/openmpi/4.1.0-no_ucx

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$ROOT_DIR"

PROCESS_COUNTS=(1 2 4 8 12 24)
INSTANCE="data/berlin52.tsp"
BASE_SEED=${BASE_SEED:-20260720}
OUTPUT="scaling_results_tsplib_euc2d.csv"

if [ ! -f "$INSTANCE" ]; then
    echo "Missing benchmark instance: $INSTANCE" >&2
    exit 1
fi

make
printf "Cores,BaseSeed,TotalTime,BestDist\n" > "$OUTPUT"

for processes in "${PROCESS_COUNTS[@]}"; do
    log_file="run_tsplib_P${processes}.log"
    rm -f solution.txt my_route.txt

    echo "Running Berlin52 with ${processes} MPI processes and base seed ${BASE_SEED}"
    mpirun -np "$processes" --map-by node \
        ./tsp_solver "$INSTANCE" --seed "$BASE_SEED" | tee "$log_file"

    runtime=$(grep "Simulation Time" "$log_file" | awk '{print $3}')
    best_distance=$(cat solution.txt)
    printf "%s,%s,%s,%s\n" \
        "$processes" "$BASE_SEED" "$runtime" "$best_distance" >> "$OUTPUT"
done

echo "Corrected TSPLIB-compatible scaling results written to $OUTPUT"
