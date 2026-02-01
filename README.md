<p align="center">
  <a href="https://www.uni-wuppertal.de/en/">
    <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/b/b2/Bergische_Universit%C3%A4t_Wuppertal_Logo.svg/2560px-Bergische_Universit%C3%A4t_Wuppertal_Logo.svg.png" alt="Bergische Universität Wuppertal Logo" width="220" style="margin-right: 40px;">
  </a>
  <a href="https://www.open-mpi.org/">
    <img src="https://www.open-mpi.org/images/open-mpi-logo.png" alt="OpenMPI Logo" width="160" style="margin-right: 40px;">
  </a>
  <a href="https://en.cppreference.com/w/c">
    <img src="https://upload.wikimedia.org/wikipedia/commons/1/18/C_Programming_Language.svg" alt="C Language Logo" width="100">
  </a>
</p>

# **Distributed TSP Solver**
<p align="center">
  <a href="https://gcc.gnu.org/">
    <img src="https://img.shields.io/badge/Language-C99-blue.svg" alt="C Badge">
  </a>
  <a href="https://www.open-mpi.org/">
    <img src="https://img.shields.io/badge/MPI-OpenMPI_4.1-green.svg" alt="MPI Badge">
  </a>
  <a href="LICENSE">
    <img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License Badge">
  </a>
  <a href="#">
    <img src="https://img.shields.io/badge/Focus-HPC_Optimization-orange.svg" alt="HPC Badge">
  </a>
</p> 

## 🚨 High-performance Parallel Tempering solver with O(1) arithmetic & deadlock-free synchronization
<p align="center">
  <img src="figures/route_comparison.png" width="700" alt="TSP Route Optimization Result">
</p>

## 📑 Table of Contents
- [Overview](#overview)  
- [Features](#features)  
- [Quick Start](#quick-start)
- [Theoretical Background](#theoretical-background)  
- [Performance Metrics](#performance-metrics)  
- [Roadmap](#roadmap)  
- [Troubleshooting](#troubleshooting)
- [Acknowledgments](#acknowledgments)  
- [Contributing](#contributing)  
- [License](#license)

## Overview
This project delivers a **robust, scalable, and mathematically rigorous** framework for solving the Traveling Salesman Problem (TSP) on distributed clusters.  
It combines **Parallel Tempering (Replica Exchange)**, **Metropolis-Hastings MCMC**, and **low-level memory optimizations** to escape local minima in complex energy landscapes.  

Designed for:
- **HPC Engineers** analyzing strong scaling efficiency
- **Researchers** in combinatorial optimization
- **Systems Developers** interested in MPI synchronization patterns

## Features
🚀 **Massive Speedup** – Achieves **20.7x speedup** on 24 cores vs serial execution  
🔥 **Thermodynamic Tunneling** – Uses 24 temperature replicas to traverse non-convex landscapes  
🗺 **O(1) Delta Updates** – Precomputed distance matrices eliminate $O(N)$ math bottlenecks  
🛡 **Deadlock Free** – Implements a synchronized "Odd-Even" handshake protocol  
📊 **Automated Analytics** – Full Bash/Octave pipeline for generating scaling graphs  
📈 **Visual Outputs** – Real-time route maps, efficiency plots & swap rate analysis

<p align="center">
  <img src="figures/scaling_speedup.png" width="550" alt="Strong Scaling Speedup Graph">
</p>

## Quick Start

```bash
git clone [https://github.com/YOUR_USERNAME/Distributed-TSP-Solver.git](https://github.com/YOUR_USERNAME/Distributed-TSP-Solver.git)
cd Distributed-TSP-Solver
make

```

### Run a Single Instance

Solve the `berlin52` dataset using 4 processor cores:

```bash
mpirun -np 4 ./tsp_solver data/berlin52.tsp

```

### Run Full Scaling Benchmark

Execute the automated test suite (1-24 cores) and generate CSV logs:

```bash
./scripts/scaling_test.sh

```

## Theoretical Background

### 🔹 The Metropolis-Hastings Kernel

We model the TSP tour length as the energy of a physical system. The solver uses a **2-opt local search** move set. To satisfy detailed balance, moves are accepted with probability .

---

### 🔹 Parallel Tempering (Replica Exchange)

Standard Simulated Annealing often gets trapped in local minima. Parallel Tempering overcomes this by running multiple replicas at different temperatures and allowing them to **exchange states**.

This allows "cold" replicas to tunnel through high-energy barriers by swapping with "hot" replicas.

---

### 🔹 The Deadlock Problem

In distributed memory systems, a naive swap implementation leads to a **Circular Wait** (Deadlock) where every processor is waiting to send data.

We resolved this using a **Parity-Based Handshake Protocol**:

1. **Even Phase**: Ranks  communicate.
2. **Odd Phase**: Ranks  communicate.

---

### 🔹 Computational Complexity

By precomputing the distance matrix , we reduce the cost of calculating the energy difference  from **Linear ** to **Constant **.

## Performance Metrics

Tested on: **24-Core Cluster, OpenMPI 4.1.0, GCC 9.3**

### 1. Strong Scaling & Efficiency

The system maintains high parallel efficiency (86%) even at 24 cores. The slight dip is expected due to Amdahl's Law and network latency.

<p align="center">
<img src="figures/scaling_efficiency.png" width="48%" alt="Parallel Efficiency">
<img src="figures/scaling_breakdown_fixed.png" width="48%" alt="Runtime Breakdown">
</p>
<i>Left: Efficiency drops slightly at 24 cores. Right: Computation (Metropolis) dominates, but Communication (Swap) overhead grows marginally.</i>

---

### 2. Thermodynamic Health (Swap Rates)

A critical success factor for Parallel Tempering is the **Swap Acceptance Rate**. As shown below, increasing the number of replicas (and thus density in temperature space) improves the swap probability, facilitating better tunneling through energy barriers.

<p align="center">
<img src="figures/scaling_swap_rate.png" width="600" alt="Swap Acceptance Rate">
</p>

---

### 3. Solution Quality & Stability

A common risk in parallelization is "breaking" the algorithm logic. As shown below, our solver consistently finds the global optimum () across all core counts, proving the thermodynamic balance is preserved.

<p align="center">
<img src="figures/scaling_quality_bar.png" width="600" alt="Solution Quality Consistency">
</p>

## Roadmap

* [x] MPI Distributed Implementation
* [x] O(1) Matrix Optimization
* [x] Strong Scaling Analysis
* [ ] Hybrid OpenMP/MPI support
* [ ] GPU Acceleration (CUDA)

## Troubleshooting

**Q:** `mpirun` not found?

**A:** Ensure OpenMPI is installed: `sudo apt install openmpi-bin libopenmpi-dev`.

**Q:** Compiler errors with `sqrt`?

**A:** Ensure the linker flag `-lm` is present in the Makefile (included by default).

**Q:** Segmentation Fault?

**A:** Check that `data/berlin52.tsp` exists and is readable.

## Acknowledgments

This project was developed as a final thesis for the **High Performance Computing** course at the **Bergische Universität Wuppertal**. We extend our gratitude to:

* **Dr. T. Korzec** & **Dr. J. Koponen** for supervision and theoretical guidance.
* **The OpenMPI Project** for robust communication libraries.
* **The Scientific Computing Community** for foundational MCMC algorithms.

## Contributing

We welcome contributions of all kinds — bug fixes, performance improvements, documentation updates, and new features.

Please read our [CONTRIBUTING.md](https://www.google.com/search?q=CONTRIBUTING.md) for detailed contribution guidelines, coding standards, and workflow instructions before submitting a pull request.

## License

MIT License – see [LICENSE](https://www.google.com/search?q=LICENSE) for details.
