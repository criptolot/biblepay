#!/bin/bash
# use testnet settings,  if you need mainnet,  use ~/.estaterocore/estaterod.pid file instead
estatero_pid=$(<~/.estaterocore/testnet3/estaterod.pid)
sudo gdb -batch -ex "source debug.gdb" estaterod ${estatero_pid}
 