FROM bplotka/mesos-modules-dev:0.24.1
MAINTAINER serenity

ADD . /serenity

WORKDIR /serenity

RUN apt-get update -q && apt-get -qy install \
  clang-3.5

# Check for style errors.
RUN ./scripts/lint.sh

# Install the picojson headers.
RUN wget https://raw.githubusercontent.com/kazuho/picojson/v1.3.0/picojson.h -O /usr/local/include/picojson.h

# Build serenity.
# We need libmesos which is located in /usr/local/lib.
RUN rm -rf build && \
    mkdir build && \
    cd build && \
    export LD_LIBRARY_PATH=LD_LIBRARY_PATH:/usr/local/lib && \
    cmake -DWITH_MESOS="/mesos" \
          -DWITH_SOURCE_MESOS="/mesos" \
          -DUSE_CLANG=ON ..  && \
    make -j 2 && \
    ./serenity-tests
