# DOCS DB Project

We were unable to complete the DPDK portion due to having issues with the DPDK setup. We have included some of our code for the DPDK portion in the `dpdk` directory. Aside from that, we have completed the rest of the project.

### 1. **Build the Server and Client**

To build the project (server and client), run the following command:

```bash
make build
```

This will compile the server and client executables.

### 2. **Set up Namespaces**

To set up the required namespaces and virtual interfaces, run the following command:

```bash
./start_namespace_bridge.sh
```

This will invoke a script (`start_namespace_bridge.sh`) to create the necessary network namespaces and bridges.

### Performance Metrics

For 10,000 packets:

- **Avg Latency:** 117.91 ms
- **Bandwidth:** 0.01 Mbps
- **Avg CPU Usage:** 3.21%
- **Avg Memory Usage:** 4198 KB

For 100,000 packets:

- **Avg Latency:** 117.13 ms
- **Bandwidth:** 0.01 Mbps
- **Avg CPU Usage:** 0.42%
- **Avg Memory Usage:** 4355 KB