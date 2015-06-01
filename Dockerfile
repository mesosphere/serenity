FROM mesosphere/mesos-modules-dev:latest
MAINTAINER Mesosphere <support@mesosphere.io>

ADD . /serenity

WORKDIR /serenity

# Install the picojson headers
RUN wget https://raw.githubusercontent.com/kazuho/picojson/v1.3.0/picojson.h -O /usr/local/include/picojson.h

RUN ./bootstrap

RUN mkdir build && \
    cd build && \
    ../configure --with-mesos-root=/mesos --with-mesos-build-dir=/mesos/build && \
    make -j 2
