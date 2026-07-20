# Distributed TSP Solver

<p align="center">
  <img src="https://img.shields.io/badge/Status-Completed-brightgreen.svg" alt="Completed">
  <img src="https://img.shields.io/badge/Benchmark%20refresh-Completed-brightgreen.svg" alt="Benchmark refresh completed">
  <img src="https://img.shields.io/badge/Language-C99-blue.svg" alt="C99">
  <img src="https://img.shields.io/badge/Parallelism-MPI-green.svg" alt="MPI">
  <a href="https://github.com/Kandil2001/Distributed-TSP-Solver/actions/workflows/ci.yml">
    <img src="https://github.com/Kandil2001/Distributed-TSP-Solver/actions/workflows/ci.yml/badge.svg" alt="MPI build and TSPLIB validation">
  </a>
  <img src="https://img.shields.io/badge/License-MIT-lightgrey.svg" alt="MIT License">
  <a href="https://kandil2001.github.io/projects/distributed-tsp.html">
    <img src="https://img.shields.io/badge/Portfolio-Case%20Study-2ea44f.svg" alt="Portfolio case study">
  </a>
</p>

A distributed-memory Traveling Salesman Problem solver written in C and parallelized with MPI.

The project was developed for a High Performance Computing course at Bergische Universität Wuppertal. It uses parallel tempering to study how a stochastic optimization method behaves when multiple temperature replicas are distributed across MPI processes.

## Corrected TSPLIB implementation

The solver follows the TSPLIB `EUC_2D` convention: each Euclidean edge length is rounded to the nearest integer before it contributes to the route weight.

The included official Berlin52 optimal tour is evaluated automatically and must produce:

```text
7542
```

The current repeated benchmark uses this corrected objective. Older course figures and the previously reported `20.7×` speedup were generated with raw floating-point Euclidean distances and are retained only as historical course artifacts.

## Main features

- distributed optimization with MPI
- parallel tempering with 24 global temperature replicas
- Metropolis acceptance logic
- two-opt route updates
- TSPLIB-compatible `EUC_2D` edge weights
- precomputed distance matrix for constant-time lookups
- deadlock-free communication between neighboring MPI ranks
- optional deterministic base seed for reproducible runs
- official Berlin52 optimal-tour regression check
- GitHub Actions production build and two-rank smoke execution
- repeated fixed-seed strong-scaling benchmark

## How the solver works

Each MPI process stores one or more route replicas. The replicas use different temperatures, allowing colder replicas to focus on local improvement while hotter replicas explore the search space more freely.

During the search:

1. a replica proposes a two-opt move
2. the route-weight change is calculated from four distance-matrix lookups
3. the move is accepted or rejected using the Metropolis criterion
4. neighboring replicas periodically attempt to exchange routes
5. the best route found across all processes is collected

The exchange step helps colder replicas leave local minima by receiving routes explored at higher temperatures.

## TSPLIB distance handling

The included instance declares:

```text
EDGE_WEIGHT_TYPE: EUC_2D
```

For two cities separated by `dx` and `dy`, the solver calculates:

```text
nint(sqrt(dx² + dy²))
```

where `nint` is the TSPLIB nearest-integer convention for non-negative distances. Unsupported edge-weight types are rejected instead of being interpreted silently.

City identifiers are validated when the instance is loaded, and the known optimal tour is stored in `data/berlin52.opt.tour` as a regression fixture.

## Build and run

On Ubuntu or Debian, install the main dependencies with:

```bash
sudo apt install build-essential openmpi-bin libopenmpi-dev octave
```

Build the production solver:

```bash
git clone https://github.com/Kandil2001/Distributed-TSP-Solver.git
cd Distributed-TSP-Solver
make
```

Run the Berlin52 case with a reproducible seed:

```bash
mpirun -np 4 ./tsp_solver data/berlin52.tsp --seed 12345
```

Without `--seed`, the solver uses the current time as the base seed.

Evaluate the official optimal tour without running the stochastic search:

```bash
mpirun -np 1 ./tsp_solver \
  data/berlin52.tsp \
  --evaluate-tour data/berlin52.opt.tour
```

Expected output:

```text
Tour weight (TSPLIB EUC_2D): 7542
```

## Reproducible Stromboli benchmark — 20 July 2026

The corrected solver was benchmarked with five fixed seeds (`1001`, `2002`, `3003`, `4004`, and `5005`) at `1`, `2`, `4`, `8`, `12`, and `24` MPI processes. This produced 30 measured runs.

Every run reached the official Berlin52 optimum of `7542`, giving a `0%` optimality gap at every process count.

| MPI processes | Median runtime [s] | Runtime std. dev. [s] | Median speedup | Parallel efficiency |
|---:|---:|---:|---:|---:|
| 1 | 6.8956 | 0.0310 | 1.000 | 100.0% |
| 2 | 3.5857 | 0.0097 | 1.923 | 96.2% |
| 4 | 1.9133 | 0.0047 | 3.604 | 90.1% |
| 8 | 1.0907 | 0.0009 | 6.322 | 79.0% |
| 12 | 0.8497 | 0.0020 | 8.116 | 67.6% |
| 24 | 0.6367 | 0.0080 | 10.830 | 45.1% |

The canonical raw and aggregated datasets are stored in [`results/stromboli_2026-07-20`](results/stromboli_2026-07-20). The table uses median runtime for speedup because repeated stochastic runs are better represented by a robust central value than by one isolated timing.

## Continuous integration

The GitHub Actions workflow performs three checks:

1. builds the unchanged production solver
2. evaluates the official Berlin52 optimal tour and requires a weight of `7542`
3. builds a temporary reduced-iteration binary and runs a deterministic two-rank MPI smoke case

The smoke run verifies that MPI communication completes, the global reduction returns a finite positive route weight, and the saved route is a valid permutation of all 52 cities. The temporary iteration reduction is used only to keep CI fast and does not modify the production solver.

## Historical course benchmark

The original course study reported a speedup of **20.7× on 24 cores** and approximately **86% parallel efficiency**. That study used the earlier raw floating-point distance objective, so its figures must not be compared directly with the corrected TSPLIB-compatible benchmark above.

<p align="center">
  <img src="figures/scaling_speedup.png" width="620" alt="Historical strong-scaling speedup">
</p>

<p align="center">
  <img src="figures/scaling_efficiency.png" width="48%" alt="Historical parallel efficiency">
  <img src="figures/scaling_breakdown_fixed.png" width="48%" alt="Historical runtime breakdown">
</p>

## Repository structure

```text
script/tsp_mpi.c                         MPI solver
script/scaling_test.sh                   cluster scaling study
script/plot_*.m                          Octave plotting scripts
data/berlin52.tsp                        included TSPLIB benchmark instance
data/berlin52.opt.tour                   official optimal-tour regression fixture
results/stromboli_2026-07-20/            corrected repeated benchmark data
figures/                                 historical course-study plots
.github/workflows/ci.yml                 build, validation, and MPI smoke test
Makefile                                 local build configuration
```

## Scope and limitations

This repository is an educational HPC project rather than a general-purpose TSP library. The current implementation is limited to TSPLIB `EUC_2D` coordinate instances, one MPI parallel-tempering method, 24 compile-time global replicas, and the Berlin52 benchmark.

The repeated benchmark improves confidence in the reported timing and route quality, but it still represents one problem instance and one cluster environment. Useful extensions include runtime configuration of the temperature schedule and replica count, additional TSPLIB datasets, hybrid MPI/OpenMP execution, and CUDA acceleration.

## Acknowledgments

This project was developed for the High Performance Computing course at Bergische Universität Wuppertal.

Thanks to **Dr. T. Korzec** and **Dr. J. Koponen** for their supervision and guidance.

## Author

Ahmed Kandil — [Portfolio](https://kandil2001.github.io/) · [LinkedIn](https://www.linkedin.com/in/ahmed-kandil03/) · [ORCID](https://orcid.org/0009-0007-2724-4565)

Released under the [MIT License](LICENSE).
