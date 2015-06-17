FROM mesosphere/mesos-modules-dev:latest
MAINTAINER Mesosphere <support@mesosphere.io>

ADD . /serenity

WORKDIR /serenity

# Install the picojson headers.
RUN wget https://raw.githubusercontent.com/kazuho/picojson/v1.3.0/picojson.h -O /usr/local/include/picojson.h

RUN ./setup.sh

# Build serenity.
# We need libmesos which is located in /usr/local/lib.
RUN rm -rf build && \
    mkdir build && \
    cd build && \
    export LD_LIBRARY_PATH=LD_LIBRARY_PATH:/usr/local/lib && \
    cmake -DWITH_MESOS="/mesos" .. && \
    make -j 2 && \
    ./serenity-tests
