FROM alpine:latest

RUN apk --no-cache add --virtual .build-dependencies build-base git automake autoconf libtool \
	&& git clone --recursive https://github.com/detomon/bliplay.git bliplay \
	&& (cd bliplay && ./autogen.sh && ./configure --without-sdl && make -j 4 install) \
	&& rm -rf bliplay \
	&& apk del .build-dependencies
