# Contributing

This is a small educational HPC project, but fixes and focused improvements are welcome.

Before opening a pull request:

1. Explain the problem or improvement clearly.
2. Keep changes limited to one topic.
3. Compile the solver with `make`.
4. Run at least one Berlin52 case and check that the output is sensible.
5. Run the local CI smoke test:
   - `mpirun --allow-run-as-root -np 2 bin/tsp_solver data/berlin52.tsp --replicas 4 --steps 10 --deterministic --swap-interval 2`
   - `python3 script/plot_results.py --solution solution.txt --route my_route.txt --tsp data/berlin52.tsp --output figures/route_comparison_python.png`
6. Mention whether CI passed (GitHub Actions workflow in `.github/workflows/ci.yml`).
7. Note the compiler, MPI implementation, and process count used for testing.

For C and MPI changes, use four-space indentation, free allocated memory, and add comments only where the communication or algorithmic logic is not obvious.

Please avoid unrelated formatting changes in the same pull request.
