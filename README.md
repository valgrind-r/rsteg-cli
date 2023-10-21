# Rsteg

Rsteg is a CLI tool based on the Least Significant Bit (LSB) technique, which enables embedding of files and archives within image, audio and video containers.

## Features

- Currently supports encoding to lossless image ```png``` audio ```wav alac flac``` and video ```h264 h265 vp9 av1 (lossless) ffv1``` codecs.

- Compatible archives ```zip 7z tar tar.gz tar.xz tar.bz2 tar.zst dmg aar dar cfs rar```

- **Seed-Based Distribution**: The distribution of encoded data is determined using a seed value and encoded in random color channels. The generator implemented uses 64-bit Mersenne Twister (PRNG).

- **Layered AES-256**: Data is encrypted with an AES-256 key derived from SHA-2 and secure ECDH key-exchange.

## Dependencies

```
openssl
libpng (>=1.6.37)
ffmpeg
```

Note: MSYS2 distributions of the deps available for building on windows<br>

****https://packages.msys2.org/base/mingw-w64-openssl****<br>
****https://packages.msys2.org/base/mingw-w64-libpng****<br>
****https://packages.msys2.org/base/mingw-w64-cmake****<br>


**Build using cmake**
- clone the repo
- ```cd``` to repo root
- generate build files
```
mkdir build
cd build
cmake ..
``` 
  - on unix ```make```
  - on windows ```ninja``` 

## Usage:

- overview
```
./rsteg --help
```
- encode files
```
./rsteg enc -i [container] -m [file/archive] -rk [recipient public key] -pk [private key]
```
- decode containers
```
./rsteg dec -i [container] -rk [sender public key] -pk [private key]
```
