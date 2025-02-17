FROM ubuntu:jammy

RUN apt-get update -y
RUN apt-get install -y \
        git \
        build-essential \
        pkg-config \
        binutils-arm-none-eabi \
        libpng-dev
RUN git clone https://github.com/pret/agbcc && cd agbcc && ./build.sh
RUN hash -r

