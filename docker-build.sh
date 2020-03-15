#!/bin/sh

docker build -t detomon/bliplay-alpine:latest .
docker push detomon/bliplay-alpine:latest
