# Serenity [![Build Status](https://teamcity.mesosphere.io/guestAuth/app/rest/builds/buildType:(id:SerenityModules_CI)/statusIcon)](https://teamcity.mesosphere.io/viewType.html?buildTypeId=SerenityModules_CI&guest=1)

# Building the Modules

Intel and Mesosphere are working on creating cutting-edge oversubscription
technologies for Mesos. Follow the [Mesos Oversubscription Architecture](https://docs.google.com/document/d/1pUnElxHy1uWfHY_FOvvRC73QaOGgdXE0OXN-gbxdXA0/edit), it
is a very flexible solution which drives the internal semantics in Mesos but
leaves all actual estimation and controller logic to module implementors.

We consider oversubscription as a series of estimates i.e. how much can safely
be oversubscribed and decisions i.e. how to protect production workloads. The
different substages of estimates and decision-making should be able to
influence each other. For example, dramatic corrections may have to involve
limiting or stopping current estimates.

We aim for a very flexible solution where both estimation and corrections are
done in a pipelined approach with shared knowledge between each stage, referred
to as Filters with a shared bus.

![Serenity pipeline](https://github.com/mesosphere/serenity/blob/master/docs/images/serenity_pipeline.png)

For more documentation, please refer to [docs](https://github.com/mesosphere/serenity/blob/master/docs/README.md).

## Installing

### Quickstart: build-and-test with Docker

With the Serenity repository cloned locally:

```
cd serenity
docker build .
```

The `Dockerfile` located in the project root is based on the
`mesosphere/mesos-modules-dev` image.  This image has newest Mesos (from master)
pre-built with unbundled dependencies for convenience.
See the contents of that Dockerfile
[here](https://raw.githubusercontent.com/mesosphere/docker-containers/master/mesos-modules-dev/Dockerfile).

### Prerequisites

Building Mesos modules requires system-wide installation of google-protobuf,
glog, boost and picojson.

Currently it supports 0.24.x Mesos. 
(since Stout & libprocess changes appeared in newer versions)

### Build Mesos with some unbundled dependencies

#### Preparing Mesos source code

Start by pulling a recent version of [Apache Mesos](https://git-wip-us.apache.org/repos/asf/mesos.git):

```
git clone https://git-wip-us.apache.org/repos/asf/mesos.git ~/mesos
```

#### Building and Installing Mesos

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

### Building Serenity with Cmake

Once Mesos is built and installed, clone the Serenity package.

Build serenity with these commands:

```
cd build
cmake -DWITH_MESOS="/usr" ..
make
```

Run the tests:

```
make test
```

### Deploying Serenity Module

Create a JSON file that describes the shared library and its parameters to the Mesos slave process:

```
{
    "libraries": [
    {
        "file": "./build/libserenity.so",
        "modules": [
        {
            "name": "com_mesosphere_mesos_SerenityEstimator"
        },
        {
            "name": "com_mesosphere_mesos_SerenityController"
        }
      ]
    }
  ]
}

```

You can reuse sample `serenity.json.in`.
In order to use serenity, add these lines to your mesos-slave command line options:

```
--modules=file://serenity.json.in \
--resource_estimator="com_mesosphere_mesos_SerenityEstimator" \
--qos_controller="com_mesosphere_mesos_SerenityController"
```

### Deploying Serenity Module using Deployment Scripts

There is useful [Serenity-Formula project](https://github.com/Bplotka/serenity-formula) 
for the Mesos & Serenity deployment. It can be used  to prepare cluster 
for Serenity end-to-end tests.
 
You are welcome to use it & contribute in case of any bug or enhancement.

### Contributing

Send pull requests for code review before merging.
Make sure that commits describes the changes and can be applied atomically.

The code base follows the [Google C++ Style Guide](https://google-styleguide.googlecode.com/svn/trunk/cppguide.html) and is linted by [cpplint](https://google-styleguide.googlecode.com/svn/trunk/cpplint/cpplint.py).

Before submitting code, make sure to run:

```bash
$ # Run style checker
$ ./scripts/lint.sh
```

```bash
$ # Make sure newly added APIs are documented
$ doxygen
```

To install the style checker as a git pre-commit hook:

```bash
$ ln -s scripts/pre-commit .git/hooks/pre-commit
```

### Details about using Mesos Modules

See [Mesos Modules](http://mesos.apache.org/documentation/latest/modules/)


### Serenity Smoke Test Framework

Serenity includes Test Mesos Framework with convienient JSON API.

For more documentation, please refer to [docs](https://github.com/mesosphere/serenity/blob/master/docs/smoke_framework.md).
