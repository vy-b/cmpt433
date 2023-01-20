#!/bin/sh
sudo iptables --table nat --append POSTROUTING --out-interface ens33 -j MASQUERADE
sudo iptables --append FORWARD --in-interface enx30e283df4a9c -j ACCEPT
echo 1 | sudo tee /proc/sys/net/ipv4/ip_forward
