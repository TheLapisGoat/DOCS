#!/bin/bash
set euxo -pipefail

if [[ $EUID -ne 0 ]]; then
    echo "This script must be run with sudo"
    exit 1
fi

ip netns add NS1
ip netns add NS3
#show namespace
ip netns show
ip link add br0 type bridge
ip link set dev br0 up
ip link add veth-NS1 type veth peer name veth-NS1-br
ip link add veth-NS3 type veth peer name veth-NS3-br
ip link set veth-NS1 netns NS1
ip link set veth-NS1-br master br0
ip link set veth-NS3 netns NS3
ip link set veth-NS3-br master br0
ip -n NS1 addr add 192.168.20.1/24 dev veth-NS1
ip -n NS3 addr add 192.168.20.2/24 dev veth-NS3
ip -n NS1 link set veth-NS1 up
ip -n NS3 link set veth-NS3 up
ip link set veth-NS1-br up
ip link set veth-NS3-br up

# Introduce delay and packet loss on NS3
ip netns exec NS3 tc qdisc add dev veth-NS3 root netem delay 100ms loss 5%