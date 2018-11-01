# Simple Daemon #

The simplest daemon which does nothing.

## How to make sure it works ##

```bash
make
./daemon
ps -axj | grep ./daemon
```

## Have to be implemented ##

* The other signals handling
* Support for command line arguments: stop and start.
  ```./daemon start``` should check if the daemon is running and start it if it's not.
  ```./daemon stop``` should check if the daemon is running and stop it if it is.

# Entropy Gathering Daemon  #

## Description ##

The daemon which gathers bytes of information from presumably random sources.

Every 5 seconds the daemon checks size of the special file ``` ~/random/buffer ```. If size is less than 5 mb (or file doesn't exist), the daemon fills it with random data.

## Possible sources of randomness ##

* Filesystem
* State of the CPU
* Time
* Keyboard and mouse
* ...and many others...

## Quality of randomness ##

The check of randomness of data is performed using FIPS 140-2 tests. The tool for this is called rngtest. You can find its description [here](https://linux.die.net/man/1/rngtest).

rngtest is included in the package **rng-tools**. You also can compile it by yourself: https://github.com/waitman/rngtest

Sample use case of rngtest:

```bash
rngtest < ~/random/buffer
```
