# NetBook

This is a work in progress :-) (30/3/2025)

NetBook writes and reads market-data networking packets using DPDK, bypassing the kernel.

The program, which is optimised for raw throughput, writes 1,000,000 network packets and then quits, displaying the time taken.

## Prerequisites

In order to run the program, you may need to follow the steps below. Note that these might not work on all versions of Linux.

### Install dependencies

```
sudo apt update
sudo apt install -y software-properties-common
sudo add-apt-repository -y universe
sudo apt update

sudo apt install -y dpdk libdpdk-dev dpdk-dev pkg-config
```

### Enable hugepages

You'll need to have hugepages enabled for the software to work.

A simple way to do this is via running the provided script. Note that this will temporarily decrease the available memory on your machine by about 256MB. This gets reverted on restart, or you can probably revert it manually by running the same commands.

```
./enable-hugepages
```

You can then check it was successful by doing:

```
grep -i HugePages_Total /proc/meminfo
```

A computer restart might be necessary.

If you run into permissions issues and it's safe to do so you can try temporarily relaxing hugepages read/write restrictions:

```
sudo chmod 777 /dev/hugepages
```

## Building

**make release**: Build for release.

## Usage

```
./netbook
```

## Recent changes

### 31 Mar 2026
- Send packets in small batches instead of individually.
- Increase mempool size by 8x for ~59% speed boost.
- Use 8 cores instead of 6 for ~23% speed boost.
- Double the number of packets we process to offset the fact that the program has gotten a lot faster.

### 30 Mar 2026
- Half read polling buffer size for a ~11% speed increase.
- Half DPDK queue size for a ~1.36% speed increase.
