# Makefile for DOCS DB Project

NAMESPACES = NS1 NS3
N_THREADS = 4

# Default target
.PHONY: all
all: build setup

# Build the server and client
.PHONY: build
build: server client

server: src/server.cpp lib/libstorageengine.a
	g++ -O3 -D N_THREADS=$(N_THREADS) -o server src/server.cpp -Iinclude -Llib -lstorageengine

client: src/client.cpp
	g++ -O3 -o client -Iinclude src/client.cpp

# Set up namespaces and virtual interfaces
.PHONY: setup
setup:
	sudo ./start_namespace_bridge.sh

# Run the server in NS1
.PHONY: run_server
run_server: server
	sudo ip netns exec NS1 ./server

# Run the client in NS3
.PHONY: run_client
run_client: client
	sudo ip netns exec NS3 ./client

# Benchmarking
.PHONY: benchmark
benchmark:
	@echo "Running redis-benchmark for various non-DPDK configurations in NS3..."
	@mkdir -p result  # Create result directory if it doesn't exist
	@for requests in 10000 100000 1000000; do \
	    for connections in 10 100 1000; do \
	        filename=result/result_$${requests}_$${connections}_0.txt; \
	        echo "Running redis-benchmark for requests=$${requests}, connections=$${connections}..."; \
	        sudo ip netns exec NS3 redis-benchmark -t get,set -h 192.168.20.1 -p 6379 -c $${connections} -n $${requests} | stdbuf -oL tee $${filename}; \
	    done; \
	done
	@echo "Non-DPDK benchmarks completed and saved to the result/ directory."

.PHONY: clean
clean: clean_namespaces clean_files clean_data clean_doxygen

# Clean up namespaces
.PHONY: clean_namespaces
clean_namespaces:
	@for ns in $(NAMESPACES); do \
		sudo ip netns delete $$ns || true; \
	done
	sudo ip link delete br0 || true

# Clean up binaries
.PHONY: clean_files
clean_files:
	rm -f server client

# Clean up data
.PHONY: clean_data
clean_data:
	sudo rm -rf data

.PHONY: clean_doxygen
clean_doxygen:
	rm -rf latex

doxygen:
	doxygen Doxyfile
	cd latex && make