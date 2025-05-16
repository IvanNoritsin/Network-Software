#!/bin/bash

set -e

NS_CLIENT="client"
IF_SERVER="veth-server"
IF_CLIENT="veth-client"
IP_SERVER="192.168.1.1"
SUBNET_MASK="24"

echo "[+] Создание namespace '$NS_CLIENT'..."
sudo ip netns add $NS_CLIENT || true

echo "[+] Создание veth-пары '$IF_SERVER' <-> '$IF_CLIENT'..."
sudo ip link add $IF_SERVER type veth peer name $IF_CLIENT || true

echo "[+] Перемещение '$IF_CLIENT' в namespace '$NS_CLIENT'..."
sudo ip link set $IF_CLIENT netns $NS_CLIENT

echo "[+] Настройка интерфейса '$IF_SERVER' на стороне сервера..."
sudo ip addr add ${IP_SERVER}/${SUBNET_MASK} dev $IF_SERVER || true
sudo ip link set $IF_SERVER up

echo "[+] Настройка DHCP-клиента в namespace '$NS_CLIENT'..."
sudo ip netns exec $NS_CLIENT ip link set lo up
sudo ip netns exec $NS_CLIENT ip link set $IF_CLIENT up

sudo ip netns exec $NS_CLIENT ip route add 255.255.255.255 dev $IF_CLIENT