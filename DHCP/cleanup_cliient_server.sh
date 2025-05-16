#!/bin/bash

set -e

echo "[!] Удаление namespace 'client'..."
sudo ip netns delete client || true

echo "[!] Удаление интерфейса 'veth-server'..."
sudo ip link delete veth-server || true

echo "[✓] Всё очищено."