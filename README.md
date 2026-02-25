# NetBook

This is a work in progress :-) (25/3/2025)

## Installation and Configuration

In order to run the program, you may need to follow all or some of the below steps. Note that these might not work on all versions of Linux.

### Install dependencies


```
sudo apt update
sudo apt install -y software-properties-common
sudo add-apt-repository -y universe
sudo apt update

sudo apt install -y dpdk libdpdk-dev dpdk-dev pkg-config
```

### Enable hugepages

Note that this will temporarily decrease the available memory on your machine by about 256MB. This gets reverted on restart, or you can probably revert it manually by running the same commands.

```
sudo sh -c 'sync; echo 3 > /proc/sys/vm/drop_caches'
sudo sh -c 'echo 1 > /proc/sys/vm/compact_memory'
sudo sysctl -w vm.nr_hugepages=128
grep -i HugePages_Total /proc/meminfo # Check that it applied the change. If not, try restarting computer to free memory.
```

## Usage

Note that if you don't use sudo, access to hugepages might get denied.

```
sudo ./nanofill
```

