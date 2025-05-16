#!/bin/bash

set -e

NS_CLIENT="client"
IF_CLIENT="veth-client"
SCRIPT="dhcp.script"

sudo ip netns exec $NS_CLIENT busybox udhcpc -i $IF_CLIENT -v -s ./$SCRIPT
echo "--------------------------------------------------------------------"

sudo ip netns exec $NS_CLIENT ip addr show $IF_CLIENT 