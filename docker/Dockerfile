FROM debian:stretch
LABEL maintainer="Estatero Developers <dev@estatero.org>"
LABEL description="Dockerised EstateroCore, built from Travis"

RUN apt-get update && apt-get -y upgrade && apt-get clean && rm -fr /var/cache/apt/*

COPY bin/* /usr/bin/
