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

