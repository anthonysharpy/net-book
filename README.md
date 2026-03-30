# NetBook

This is a work in progress :-) (30/3/2025)

NetBook writes and reads market-data networking packets using DPDK, bypassing the kernel.

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

### Args

The below are optional.

| <div style="width:30vw">Command</div> | Explanation |
| --- | --- |
| --runtime=N | The program stops after N seconds. N must be an integer |

