# Storage Engine Project

This project implements a storage engine. It includes a REPL (Read-Eval-Print Loop) for interacting with the engine, along with testing capabilities and Doxygen-based documentation.

## Prerequisites

- `g++` (C++ Compiler)
- `make`
- `Google Test` (for unit testing)
- `Doxygen` (for generating documentation)

## Project Structure

- `src/`: Contains source code files for the storage engine.
  - `persistent_hashmap.cpp`: Implementation of the persistent hashmap.
  - `lru_cache.cpp`: Implementation of the LRU cache.
  - `storage_engine.cpp`: The core storage engine implementation.
  - `REPL.cpp`: The REPL interface to interact with the engine.
  
- `include/`: Header files for the storage engine.
  
- `obj/`: Directory for object files generated during the build process.

- `tests/`: Contains the test files (unit tests for the storage engine).

- `data/`: Stores persistent data and test data files.

- `latex/`: Generated documentation directory (Doxygen output).

- `Makefile`: The build system for the project.

## Build and Usage

### 1. **Build the Project**

To build the project, run:

```bash
make
```

This will build the REPL binary and the storage engine static library (libstorageengine.a). You can run the REPL using:

```bash
make repl
```

Note, due to the persistent nature of the storage engine, the data will be stored in the `data/` directory. The data will persist across REPL sessions unless deleted.

Also, as the REPL is directly linked to the storage engine library, when exiting, it may take ~30 seconds to stop due the GC (garbage collection) process running in the background.

### 2. **Run Tests**

To run the unit tests:

```bash
make test
```

This will compile the tests and execute them using Google Test. Ensure that you have gtest and gtest_main installed on your system.

### 3. **Clean the Build**

To clean the generated files (object files, REPL, test binaries, etc.):

```bash
make clean
```

This will remove all the build artifacts including object files, the static library, and the REPL binary.

To also remove generated data:

```bash
make clean_data
```

This will delete the data and test data directories.

### 4. **Generate Doxygen Documentation**

To generate the Doxygen documentation:

```bash
make doxygen
```

This This will generate the documentation in the `latex` directory and then compile it into a PDF using LaTeX.