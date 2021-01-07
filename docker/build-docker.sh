#!/usr/bin/env bash
 
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

DOCKER_IMAGE=${DOCKER_IMAGE:-estatero/estaterod-develop}
DOCKER_TAG=${DOCKER_TAG:-latest}

BUILD_DIR=${BUILD_DIR:-.}

rm docker/bin/*
mkdir docker/bin
cp $BUILD_DIR/src/estaterod docker/bin/
cp $BUILD_DIR/src/estatero-cli docker/bin/
cp $BUILD_DIR/src/estatero-tx docker/bin/
strip docker/bin/estaterod
strip docker/bin/estatero-cli
strip docker/bin/estatero-tx

docker build --pull -t $DOCKER_IMAGE:$DOCKER_TAG -f docker/Dockerfile docker
