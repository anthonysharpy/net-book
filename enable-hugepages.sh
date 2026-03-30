#!/usr/bin/env bash

sudo sh -c 'sync; echo 3 > /proc/sys/vm/drop_caches'
sudo sh -c 'echo 1 > /proc/sys/vm/compact_memory'
sudo sysctl -w vm.nr_hugepages=128