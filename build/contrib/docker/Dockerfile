FROM ubuntu:14.04

RUN \
    apt-get update && \
    apt-get -y upgrade && \
    apt-get install -y python-pip python-setuptools bridge-utils \
                       libglib2.0-dev libdbus-1-dev libudev-dev \
                       libical-dev libreadline-dev git-core && \
    pip install --user grpc google-cloud protobuf && \
    sudo cpan -i Text::Template && \
    mkdir weave

COPY . weave

RUN cd weave && \
    ./configure --disable-systemd && \
    make all && \
    make install

CMD ["/bin/bash"]
