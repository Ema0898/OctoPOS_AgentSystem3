#!/bin/bash
sudo apt-get update && sudo apt-get -y install \
libncurses5 \
libtie-ixhash-perl \
libfile-find-rule-perl \
libxml-simple-perl \
nasm \
doxygen \
git \
gcc-8-plugin-dev \
build-essential \
python3 \
perl \
graphviz \
libc6-dev \
libc6-dev-amd64-cross \
libc6-dev-i386-cross \
libc6-dev-i386 \
wget curl \
capnproto \
&& sudo apt-get -y install --install-suggests autoconf 