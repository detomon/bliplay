FROM alpine:latest

RUN apk --no-cache add --virtual .build-dependencies build-base git automake autoconf libtool openssh-client \
	&& git clone https://github.com/detomon/bliplay.git bliplay \
	&& (cd bliplay && sed -i 's/git@github.com:detomon\/BlipKit.git/https:\/\/github.com\/detomon\/BlipKit.git/g' .gitmodules) \
	&& (cd bliplay && git submodule update --init) \
	&& (cd bliplay && ./autogen.sh && ./configure --without-sdl && make -j 4 install) \
	&& rm -rf bliplay \
	&& apk del .build-dependencies
