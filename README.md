# bliplay

[![Build Status](https://github.com/detomon/bliplay/actions/workflows/c.yml/badge.svg?branch=master)](https://github.com/detomon/bliplay/actions/workflows/c.yml)

**bliplay** is a plaintext-based chiptunes sound player.

Check out the [syntax description](SYNTAX.md). Some example files are located in [examples](examples).

Alternatively, you can use the [web editor](https://play.blipkit.audio) to run your code.

```blip
% set speed
st:18

% set waveform
w:square

% set duty cycle of square wave
dc:8

% set note C on octave 5
a:c5

% wait 1 step
s:1

% set note C on octave 6
a:c6

% enable volume slide effect
e:vs:9/1; v:0

% wait 9 steps
s:9

% release note
r
```

1. [Checkout](#1-checkout)
2. [Install SDL](#2-install-sdl-optional)
3. [Building](#3-building)
4. [Playing Files](#4-playing-files)
5. [Exporting Files](#5-exporting-files)
6. [Related Projects](#6-related-projects)
7. [Using with Docker](#7-using-with-docker)
8. [License](#license)

## 1. Checkout

[BlipKit](https://github.com/detomon/BlipKit) is the core library which generates the sound.
It is added as a submodule.  To fetch its source, 
use the following command when cloning the **bliplay** repository:

```sh
git clone --recursive git@github.com:detomon/bliplay.git
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

### [bliplay-wasm](https://github.com/detomon/bliplay-wasm)

A WebAssembly implementation of the player.

## 7. Using with Docker

A precompiled version can be used with Docker. There is no support for direct audio output, though.

Load/run container and mount local directory `/absolute/path/to/bliplay/examples` at container path `/examples`:

```sh
docker run -it -v /absolute/path/to/bliplay/examples:/examples detomon/bliplay-alpine
```

Render file `examples/killer-squid.blip` to `killer-squid.wav` on local directory.

```sh
/ # bliplay -o examples/killer-squid.wav examples/killer-squid.blip
```

## License

This program is distributed under the MIT license. See `LICENSE`.
