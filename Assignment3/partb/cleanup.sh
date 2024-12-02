#!/bin/bash
set euxo -pipefail

if [[ $EUID -ne 0 ]]; then
    echo "This script must be run with sudo"
    exit 1
fi

ip netns delete NS1 || true
ip netns delete NS3 || true
ip link set dev br0 down || true
brctl delbr br0 || true