
# Megabit

Megabit is a BIP44 HD wallet built on [libbitcoin](https://github.com/libbitcoin/) `version3`.

DO NOT USE FOR PRODUCTION USE ON MAINNET -- YOU WILL PROBABLY LOSE FUNDS

Provided as-is w/o warranty.  This project has not been actively maintained since 2017.


# Install libbitcoin-explorer
## macOs

First install [Homebrew](https://brew.sh).
```sh
$ ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```
Next install the [build system](http://wikipedia.org/wiki/GNU_build_system) and [wget](http://www.gnu.org/software/wget):
```sh
$ brew install autoconf automake libtool pkgconfig wget
```
Next install boost, icu4c, libpng, zmq, qrencode, and zlib:
```sh
$ brew install boost icu4c libpng zmq qrencode zlib 
```
Next download the [install script](https://github.com/libbitcoin/libbitcoin-explorer/blob/version3/install.sh) and enable execution:
```sh
$ wget https://raw.githubusercontent.com/libbitcoin/libbitcoin-explorer/version3/install.sh
$ chmod +x install.sh
```
Finally install BX with recommended [build options](#build-notes-for-linux--macos):
```sh
$ ./install.sh --prefix=/home/me/myprefix --with-icu --with-png --with-qrencode --enable-shared
```
Bitcoin Explorer is now installed in `/home/me/myprefix` and can be invoked as `$ bx`.

# Building

```
PKG_CONFIG_PATH=/path/to/libbitcoin/lib/pkgconfig qmake
make
```

# Running

```
LD_LIBRARY_PATH=/path/to/libbitcoin/lib ./megabit
```

