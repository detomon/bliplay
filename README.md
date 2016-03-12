bliplay
=======

This program was build to play sound files to test the BlipKit library.
Some example files are located in `bliplay/examples`.

1. Install SDL
--------------

For simplyfing the audio output on a wide range of systems the program uses SDL
(<http://www.libsdl.org>). If not already, you have to install it to be able to
run the program.

2. Checkout BlipKit
-------------------

BlipKit is the core library which generates the sound. It is added as a `git`
submodule. The source is fetched with this commands:

```Shell
$ git submodule init
$ git submodule update
```

3. Building
-----------

First execute `autogen.sh` in the base directory to generate the build system:

```Shell
$ sh ./autogen.sh
```

Next execute `configure` in the base directory:

```Shell
$ ./configure
```

Then execute `make` to build the program in the `bliplay` directory:

```Shell
$ make
```

4. Playing files
----------------

Some example files are located in `examples`. Use the following command to play them:

```Shell
$ bliplay/bliplay examples/hyperion-star-racer.blip
```
