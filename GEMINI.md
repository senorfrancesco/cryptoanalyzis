# GFN-16 Cryptographic Analysis

## Project Overview

This project implements a set of tools for the differential cryptanalysis of a custom lightweight 16-bit block cipher. The cipher operates on 16-bit blocks (split into four 4-bit nibbles) using a Generalized Feistel Network (Type-2) structure with S-boxes and bitwise rotations.

**Current Status:** The project is undergoing a major refactoring to switch from an 8-bit architecture to a 4-bit nibble architecture (Variant 5). See `task.md` for the active refactoring plan.

## Key Files

*   **`cipher_engine.h`** (New):
    *   **Core Header.** Contains the definition of the cipher logic, constants (S-Box, Keys), and data structures (`Block`).
    *   Implements the new Variant 5 architecture (4-bit nibbles).
    *   Acts as the single source of truth for the encryption algorithm to ensure consistency across all tools.

*   **`Makefile`** (New):
    *   Orchestrates the build process for all three tools.
    *   Includes a `run_test` target to execute the full analysis pipeline automatically.

*   **`generator_of_data.cpp`**:
    *   *Under Construction:* Needs update to use `cipher_engine.h`.
    *   Generates pairs of plaintexts with specific input differences.
    *   Outputs `pairs_data.txt`.

*   **`analysis_attack.cpp`**:
    *   *Under Construction:* Needs update to use `cipher_engine.h`.
    *   Analyzes the generated data to find high-probability differential characteristics for 5 rounds.
    *   Outputs results to `diff_round_5_top.txt`.

*   **`attack_last_round.cpp`**:
    *   *Under Construction:* Needs update to use `cipher_engine.h`.
    *   Performs the key recovery attack on the last round.

## Build and Run

The project uses standard C++ and requires a compiler with C++11 support (for threads and lambdas).

### Prerequisites
*   `g++` (GCC) or `clang`
*   `make`

### Compilation
To build all tools:
```bash
make
```
To clean up build artifacts:
```bash
make clean
```

### Execution Workflow

You can run the full pipeline (Generate -> Analyze -> Attack) with a single command:
```bash
make run_test
```

Or run steps individually:

1.  **Generate Data:**
    ```bash
    ./generator
    ```
2.  **Analyze Differentials:**
    ```bash
    ./analysis
    ```
3.  **Run Attack:**
    ```bash
    ./attack
    ```

## Configuration & Notes

*   **Multithreading:** The code uses 16 threads by default.
*   **Architecture:** The cipher is being updated to a 4-branch Generalized Feistel Network (Variant 5).