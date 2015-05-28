# Serenity

# Installing

## Prerequisites

Building Mesos modules requires system-wide installation of google-protobuf,
glog, boost and picojson.

## Build Mesos with some unbundled dependencies

### Preparing Mesos source code

Start by pulling a recent version of [Apache Mesos](https://git-wip-us.apache.org/repos/asf/mesos.git):

```
git clone https://git-wip-us.apache.org/repos/asf/mesos.git ~/mesos
```

### Building and Installing Mesos

Due to the fact that modules will need to have access to a couple of libprocess
dependencies, Mesos itself should get built with unbundled dependencies to
reduce chances of problems introduced by varying versions (libmesos vs. module
library).

We recommend using the following configure options:

```
cd ~/mesos
mkdir build && cd build
../configure --with-glog=/usr/local --with-protobuf=/usr/local --with-boost=/usr/local
make
make install
```

## Build Serenity Modules

Once Mesos is built and installed, clone the Serenity package.

The configuration phase needs to know some details about your Mesos build and installation
location:

```
./bootstrap
mkdir build && cd build
../configure --with-mesos-root=~/mesos --with-mesos-build-dir=~/mesos/build
make
```

At this point, the Module libraries are ready in `build/.libs`.

## Using Mesos Modules

See [Mesos Modules](http://mesos.apache.org/documentation/latest/modules/).
