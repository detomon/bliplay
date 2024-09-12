#!/bin/sh

REPO=ghcr.io/detomon/bliplay/bliplay-alpine
TAG=3.2.3

docker build --tag "$REPO:$TAG" .
docker build --tag "$REPO:latest" .
docker push "$REPO:$TAG"
docker push "$REPO:latest"
