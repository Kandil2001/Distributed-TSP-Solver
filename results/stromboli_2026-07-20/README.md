# Stromboli benchmark — 20 July 2026

This directory contains the repeated strong-scaling benchmark for the corrected TSPLIB-compatible Berlin52 solver.

## Protocol

- objective: TSPLIB `EUC_2D`
- reference optimum: `7542`
- MPI process counts: `1, 2, 4, 8, 12, 24`
- fixed seeds: `1001, 2002, 3003, 4004, 5005`
- repeats per process count: `5`
- total measured runs: `30`

Every measured run found the official optimum, so every recorded optimality gap is `0%`.

## Files

- `scaling_results_repeated_tsplib_euc2d.csv` — one row per measured run
- `scaling_summary_tsplib_euc2d.csv` — median, mean, standard deviation, range, speedup, efficiency, and route-quality summary by process count

The raw CSV is the canonical record. Speedup in the summary is calculated from the median one-process runtime.

## Main result

The median runtime decreased from `6.8956 s` with one MPI process to `0.6367 s` with 24 processes. This corresponds to a median speedup of `10.83×` and a parallel efficiency of `45.1%` at 24 processes.
