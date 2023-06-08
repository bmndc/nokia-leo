# (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
# file or any portion thereof may not be reproduced or used in any manner
# whatsoever without the express written permission of KAI OS TECHNOLOGIES
# (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
# LIMITED or its affiliate company and may be registered in some jurisdictions.
# All other trademarks are the property of their respective owners.

FROM quay.io/kaici/gaia:latest
MAINTAINER Rudy Lu <rudy.lu@kaiostech.com>

ENV DEBIAN_FRONTEND noninteractive

USER root

RUN \
  dpkg --add-architecture i386 && \
  dpkg --add-architecture amd64

COPY llvm.org.list /etc/apt/sources.list.d/llvm.org.list
COPY tool-chain.list /etc/apt/sources.list.d/tool-chain.list

RUN \
  apt-key adv --recv-key --keyserver keyserver.ubuntu.com 15CF4D18AF4F7421
RUN \
  apt-key adv --recv-key --keyserver keyserver.ubuntu.com 1E9377A2BA9EF27F

RUN \
  apt-get update -qq && \
  apt-get install -qqy --no-install-recommends \
    ssh libxt6 bc autoconf2.13 bison bzip2 ccache curl flex gawk gcc g++ \
    g++-multilib git lib32ncurses5-dev lib32z1-dev libgconf2-dev zlib1g \
    zlib1g-dev libgl1-mesa-dev libx11-dev make zip lzop libxml2-utils \
    openjdk-7-jdk unzip python xz-utils rsync \
    clang-4.0 libclang-4.0-dev libssl1.0.0 libssl-dev libcurl4-gnutls-dev cmake \
    libwww-perl 	\
    libgtk2.0-dev libgtk-3-dev libdbus-glib-1-dev yasm libasound2-dev libpulse-dev libxt-dev && \
  apt-get clean && \
  rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Set variable normally configured at login, by the shells parent process, these
# are taken from GNU su manual
ENV           SHELL         /bin/bash
