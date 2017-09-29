# bliplay

[![Build Status](https://api.travis-ci.org/detomon/bliplay.svg?branch=master)](https://travis-ci.org/detomon/bliplay)

**bliplay** is a plaintext-based chiptune sound player.
See the interactive player at <https://play.blipkit.audio> for examples and syntax description.
Some example files also are located in [examples](examples).

1. [Checkout](#1-checkout)
2. [Install SDL](#2-install-sdl-optional)
3. [Building](#3-building)
4. [Playing Files](#4-playing-files)
5. [Exporting Files](#5-exporting-files)
6. [Related Projects](#6-related-projects)
7. [License](#license)

## 1. Checkout

[BlipKit](https://github.com/detomon/BlipKit) is the core library which generates the sound.
It is added as a submodule.  To fetch its source, 
use the following command when cloning the **bliplay** repository:

```sh
git clone --recursive https://github.com/detomon/bliplay.git
```

or afterwards with:

```sh
git submodule update --init
```

## 2. Install SDL (optional)

For simplyfing the audio output on a wide range of systems the program uses SDL (<https://www.libsdl.org>).
If not already, you have to install it to be able to output the audio directly.

## 3. Building

First execute `autogen.sh` in the base directory to generate the build system:

```sh
./autogen.sh
```

Next execute `configure` in the base directory:

```sh
./configure
```

Or, use the `--without-sdl` option if you don't want to link against SDL. This will disable direct audio output, but it's still possible to export the audio.

```sh
./configure --without-sdl
```

Then execute `make` to build the program in the `bliplay` directory:

```sh
make
```

You can then run it from the bliplay directory, or install it on your system to run from anywhere.

```sh
sudo make install
```

## 4. Playing Files

Some example files are located in `examples`. Use the following command to play them:

```sh
bliplay/bliplay examples/hyperion-star-racer.blip
```

For Linux users:
If you associate bliplay with .blip files,
then the StopBlipAudio.sh script, that you'll find in the example directory,
will help you mute your blip tracks in a hurry.

## 5. Exporting Files

Use the `-o` option to render the audio to `WAV` or `RAW` files.

```sh
bliplay/bliplay -o killer-squid.wav examples/killer-squid.blip
```

## 6. Related Projects

### [blipSheet](https://github.com/mgarcia-org/blipSheet)

Script which uses a spreadsheet in order to render and play the files dynamically. By [@mgarcia-org](https://github.com/mgarcia-org/).

## License

This program is distributed under the MIT license. See `LICENSE`.
