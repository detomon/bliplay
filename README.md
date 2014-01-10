bliplay
=======

This program was build to play more complex sound files to test the BlipKit
library. It reads a CSV like file format. Some examples are located in
`examples`.

Checkout BlipKit
----------------

BlipKit is the core library which generates the sound. It is added as a `git`
submodule. With this commands, the source is fetched:

	$ git submodule init
	$ git submodule update

Building and running
--------------------

First execute `autogen.sh` in the base directory to generate the build system:

	$ sh ./autogen.sh

Next execute `configure` in the base directory:

	$ ./configure

Then execute `make` to build the program in the `bliplay` directory:

	$ make

Options
-------

The play speed can be adjusted with the options `s`:

	bliplay$ ./bliplay -s 32 -p ../examples/hyperion-star-racer.blip

More TODO...

File format
-----------

TODO
