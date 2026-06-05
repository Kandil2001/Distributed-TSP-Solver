# Contributing

This is a small educational HPC project, but fixes and focused improvements are welcome.

Before opening a pull request:

1. Explain the problem or improvement clearly.
2. Keep changes limited to one topic.
3. Compile the solver with `make`.
4. Run at least one Berlin52 case and check that the output is sensible.
5. Note the compiler, MPI implementation, and process count used for testing.

For C and MPI changes, use four-space indentation, free allocated memory, and add comments only where the communication or algorithmic logic is not obvious.

Please avoid unrelated formatting changes in the same pull request.
