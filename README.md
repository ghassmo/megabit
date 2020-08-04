
# Megabit

Megabit is a BIP44 HD wallet built on [libbitcoin](https://github.com/libbitcoin/) `version3`.

DO NOT USE FOR PRODUCTION USE ON MAINNET -- YOU WILL PROBABLY LOSE FUNDS

Provided as-is w/o warranty.  This project has not been actively maintained since 2017.

# Building

```
PKG_CONFIG_PATH=/path/to/libbitcoin/lib/pkgconfig qmake
make
```

# Running

```
LD_LIBRARY_PATH=/path/to/libbitcoin/lib ./megabit
```
