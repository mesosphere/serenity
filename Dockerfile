FROM mesosphere/mesos-modules-dev:latest
MAINTAINER Mesosphere <support@mesosphere.io>

ADD . /serenity

WORKDIR /serenity

# Install the picojson headers
RUN wget https://raw.githubusercontent.com/kazuho/picojson/v1.3.0/picojson.h -O /usr/local/include/picojson.h

RUN ./setup.sh

RUN rm -rf build && \
    mkdir build && \
    cd build && \
    cmake -DWITH_MESOS="/mesos" .. && \
    make -j 2 && \
    make test
