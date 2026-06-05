#!/bin/bash
#SBATCH -J tsp_scaling
#SBATCH -p compute2011
#SBATCH --ntasks=24
#SBATCH --cpus-per-task=1
#SBATCH -t 00:45:00
#SBATCH -o scaling_output.txt

set -euo pipefail

module load mpi/openmpi/4.1.0-no_ucx

DATA_SOURCE=/work/korzec/LAB2/proj
PROCESS_COUNTS=(1 2 4 8 12 24)

if [ ! -f berlin52.tsp ]; then
    cp "$DATA_SOURCE/berlin52.tsp" .
fi

if [ -f "$DATA_SOURCE/berlin52.opt.tour" ] && [ ! -f berlin52.opt.tour ]; then
    cp "$DATA_SOURCE/berlin52.opt.tour" .
fi

make
printf "Cores,TotalTime,BestDist\n" > scaling_results.csv

for processes in "${PROCESS_COUNTS[@]}"; do
    log_file="run_P${processes}.log"
    rm -f solution.txt

    echo "Running Berlin52 with ${processes} MPI processes"
    mpirun -np "$processes" --map-by node ./tsp_solver berlin52.tsp | tee "$log_file"

    runtime=$(grep "Simulation Time" "$log_file" | awk '{print $3}')
    best_distance=$(cat solution.txt)
    printf "%s,%s,%s\n" "$processes" "$runtime" "$best_distance" >> scaling_results.csv
done

echo "Scaling results written to scaling_results.csv"
