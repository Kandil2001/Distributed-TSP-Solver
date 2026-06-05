# Distributed TSP Solver

<p align="center">
  <img src="https://img.shields.io/badge/Language-C99-blue.svg" alt="C99">
  <img src="https://img.shields.io/badge/Parallelism-MPI-green.svg" alt="MPI">
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="MIT License">
  <a href="https://kandil2001.github.io/">
    <img src="https://img.shields.io/badge/Portfolio-kandil2001.github.io-2ea44f.svg" alt="Portfolio">
  </a>
</p>

This repository contains my distributed-memory solver for the Traveling Salesman Problem, written in C and parallelized with MPI. I built it as part of a High Performance Computing course at Bergische Universität Wuppertal.

The project uses parallel tempering to study how a stochastic optimization method behaves when the search is divided across several MPI processes. Each process explores the problem at a different temperature, and neighboring processes periodically exchange routes.

<p align="center">
  <img src="figures/route_comparison.png" width="760" alt="Comparison between the optimized and reference TSP routes">
</p>

## Main features

- Distributed optimization using MPI
- Parallel tempering with multiple temperature replicas
- Metropolis-Hastings acceptance logic
- Two-opt route updates
- Precomputed distance matrix for constant-time distance lookups
- Deadlock-free communication between neighboring MPI ranks
- Strong-scaling measurements across several process counts
- Octave scripts for plotting runtime, efficiency, route quality, and swap behaviour

## How the solver works

Each MPI process stores one or more route replicas. The replicas are assigned different temperatures so that some focus on local improvement while others explore the search space more freely.

During the search:

1. A replica proposes a two-opt move.
2. The change in route length is calculated.
3. The move is accepted or rejected using the Metropolis criterion.
4. Neighboring replicas periodically attempt to exchange routes.
5. The best route found across all processes is collected at the end.

The exchange step helps colder replicas leave local minima by receiving routes that were explored at higher temperatures.

## Implementation details

### Constant-time route updates

A two-opt move only replaces two edges, so the full route length does not need to be recalculated after every proposal. The change in distance is evaluated using four lookups from a precomputed distance matrix.

```c
static inline double get_dist(int i, int j) {
    return dist_matrix[i * N_CITIES + j];
}

double delta =
    (get_dist(cA, cB) + get_dist(cA_n, cB_n))
    - (get_dist(cA, cA_n) + get_dist(cB, cB_n));
```

### Replica exchange

Communication alternates between two neighboring-rank patterns:

- Even phase: ranks 0–1, 2–3, 4–5, ...
- Odd phase: ranks 1–2, 3–4, 5–6, ...

This avoids circular waits and keeps the replica-exchange step predictable.

## Results from the current study

The benchmark was run on a 24-core cluster using OpenMPI 4.1.0 and GCC 9.3.

For the tested Berlin52 configuration, the parallel version reached a reported speedup of **20.7× on 24 cores**, corresponding to approximately **86% parallel efficiency**. The measurements also show the expected increase in communication overhead as more MPI processes are added.

<p align="center">
  <img src="figures/scaling_speedup.png" width="620" alt="Strong-scaling speedup">
</p>

<p align="center">
  <img src="figures/scaling_efficiency.png" width="48%" alt="Parallel efficiency">
  <img src="figures/scaling_breakdown_fixed.png" width="48%" alt="Runtime breakdown">
</p>

The swap-rate and route-quality plots are included to check whether the replicas exchange routes often enough and whether scaling the run changes the quality of the final solution.

<p align="center">
  <img src="figures/scaling_swap_rate.png" width="48%" alt="Replica swap acceptance rate">
  <img src="figures/scaling_quality_bar.png" width="48%" alt="Solution quality across process counts">
</p>

## Running the code

The project requires a C compiler, MPI, and the math library. Octave is used for the result plots.

On Ubuntu or Debian, the main dependencies can be installed with:

```bash
sudo apt install build-essential openmpi-bin libopenmpi-dev octave
```

Clone the repository and follow the included run script or compile the C source with `mpicc`:

```bash
git clone https://github.com/Kandil2001/Distributed-TSP-Solver.git
cd Distributed-TSP-Solver
mpicc -O3 tsp_mpi.c -o tsp_solver -lm
mpirun -np 4 ./tsp_solver data/berlin52.tsp
```

The included cluster script can be used to repeat the strong-scaling study and generate the CSV data used by the plotting scripts.

## Notes and limitations

This is an educational HPC project rather than a general-purpose TSP library. The current version focuses on one MPI-based parallel-tempering approach and was mainly evaluated using the Berlin52 case.

The reported timings are specific to the tested cluster and solver settings. Because the method is stochastic, a wider statistical study with repeated runs would be needed before drawing broader conclusions about solution quality or scalability.

Useful next steps include hybrid MPI and OpenMP parallelism, CUDA acceleration, additional TSPLIB datasets, and runtime configuration of the temperature schedule and solver parameters.

## Acknowledgments

This project was developed for the High Performance Computing course at Bergische Universität Wuppertal.

Thanks to **Dr. T. Korzec** and **Dr. J. Koponen** for their supervision and guidance during the course.

## Author

Ahmed Kandil — [Portfolio](https://kandil2001.github.io/) · [GitHub](https://github.com/Kandil2001) · [LinkedIn](https://www.linkedin.com/in/ahmed-kandil03/)

Released under the [MIT License](LICENSE).
