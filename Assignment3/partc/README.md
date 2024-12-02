# DOCS DB Project

This project includes the server-client setup with the storage engine, namespaces and benchmarking functionality. It provides mechanisms for building, setting up namespaces, running the server and client in isolated namespaces, and performing benchmarking without DPDK. We were unable to implement the DPDK-based portions due to installation issues.

## Project Structure

- `src/`: Contains the source code for the server and client.
  - `server.cpp`: Implementation of the server.
  - `client.cpp`: Implementation of the client.

- `lib/`: Contains the static library `libstorageengine.a` used by the server.
- `include/`: Header files for the project.
- `result/`: Directory to store benchmarking results.
- `data/`: Directory for storing data used by the project.
- `latex/`: Generated directory for Doxygen-based documentation (PDF).
- `Makefile`: The build system for the project.

## Prerequisites

- `g++` (C++ Compiler)
- `make`
- `redis-benchmark` for benchmarking
- `iproute2` (for managing network namespaces)

## Targets

### 1. **Build the Server and Client**

To build the project (server and client):

```bash
make build
```

This will compile the server and client executables

- The server is compiled with multi-threading support (`N_THREADS`).
- The client is compiled separately.

### 2. **Set up Namespaces**

To set up the required namespaces and virtual interfaces:

```bash
make setup
```

This will invoke a script (`start_namespace_bridge.sh`) to create the necessary network namespaces and bridges.

### 3. **Run the Server in Namespace NS1**

To run the server in the namespace `NS1`:

```bash
make run_server
```

This command uses ip netns to run the server inside the NS1 network namespace.

### 4. **Run the Client in Namespace NS3**

To run the client in the namespace `NS3`:

```bash
make run_client
```

This command uses ip netns to execute the client inside the NS3 network namespace.

### 5. **Benchmarking**

To run the Redis benchmarking tests without DPDK support:

```bash
make benchmark
```

This will run redis-benchmark in `NS3` for different configurations of requests and connections. The results will be saved in the `result/` directory.

### 6. **Clean the Build**

To clean the build:

```bash
make clean
```

This will remove the following:
- Network namespaces
- The server and client binaries
- Data directories
- Doxygen-generated files

## Clean Network Namespaces

To clean the network namespaces:

```bash
make clean_namespaces
```

## Clean Binaries

To clean the server and client binaries:

```bash
make clean_files
```

## Clean Data

To clean the data directories:

```bash
make clean_data
```

## Clean Doxygen Files

To clean the Doxygen-generated files:

```bash
make clean_doxygen
```

### 7. **Generate Doxygen Documentation**

To generate Doxygen documentation:

```bash
make doxygen
```

This will generate the Doxygen documentation and compile it into a PDF file using LaTeX. The generated documentation will be available in the `latex/` directory.
