# Contributing to Distributed TSP Solver

First off, thank you for considering contributing to this project! It's people like you that make the open-source community such an amazing place to learn, inspire, and create.

## 🤝 How Can I Contribute?

### 1. Reporting Bugs
This section guides you through submitting a bug report.
* **Check the Issues:** Ensure the bug hasn't already been reported.
* **Open a New Issue:** comprehensive bug reports are the key to fixing problems.
* **Include Details:**
    * Your OS (e.g., Ubuntu 20.04, macOS Big Sur).
    * MPI Version (e.g., OpenMPI 4.1.0).
    * Compiler Version (e.g., GCC 9.3).
    * Steps to reproduce the bug.

### 2. Suggesting Enhancements
This section guides you through submitting an enhancement suggestion.
* **Open a New Issue:** Describe the enhancement and why it is useful.
* **Provide Context:** Explain how this enhancement fits into the current architecture (e.g., "Adding OpenMP support to the inner loop").

### 3. Pull Requests
The process for submitting a Pull Request (PR) is straightforward:

1.  **Fork the Repo** and create your branch from `main`.
2.  **Clone the Repo** to your local machine.
3.  **Create a Branch**:
    ```bash
    git checkout -b feature/AmazingFeature
    ```
4.  **Make your changes**.
5.  **Test your changes**: Ensure the code compiles and runs the scaling test without errors.
    ```bash
    make clean && make
    ./scripts/scaling_test.sh
    ```
6.  **Commit your changes** (Write clear, concise commit messages).
7.  **Push to the branch**:
    ```bash
    git push origin feature/AmazingFeature
    ```
8.  **Open a Pull Request**.

## 💻 Coding Guidelines

To maintain the quality of this HPC project, please adhere to the following guidelines:

### C / MPI Standards
* **Standard:** Use **C99** or later.
* **MPI:** Use standard MPI calls (e.g., `MPI_Send`, `MPI_Recv`). Avoid proprietary extensions.
* **Memory Safety:** Always free allocated memory (`malloc` / `free`). Use `valgrind` if possible to check for leaks.
* **Error Handling:** Check return values for file I/O and MPI calls.

### Style Guide
* **Indentation:** Use **4 spaces** for indentation. No tabs.
* **Naming:**
    * Variables: `snake_case` (e.g., `best_distance`, `city_count`)
    * Functions: `snake_case` (e.g., `calculate_energy`, `swap_replicas`)
    * Constants/Macros: `UPPER_CASE` (e.g., `MAX_REPLICAS`)
* **Comments:** Comment complex logic, especially the MPI synchronization blocks.

### Performance
* **Avoid:** Excessive I/O in the main simulation loop.
* **Prefer:** `static inline` for small helper functions like distance calculations to encourage inlining.

## 📜 License
By contributing, you agree that your contributions will be licensed under its MIT License.
