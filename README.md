<img align="right" src="https://i.imgur.com/PJsPNSG.png" height="200">
 
 [![Build Status](https://travis-ci.org/PoC-Consortium/engraver.svg?branch=master)](https://travis-ci.org/PoC-Consortium/engraver) [![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

# Engraver - PoC2 plotter in Rust

### Features
- windows, linux, unix & macOS
- x86 32&64bit 
- direct and async i/o
- SIMD support: sse2, avx, avx2, avx512f
- gpu support
- fastest plotter there is

### Requirements
- new version of rust [stable toolchain]

### Compile, test, ...

Binaries are in **target/debug** or **target/release** depending on optimization.

``` shell
# build debug und run directly
cargo run [--features=opencl]

# build debug (unoptimized)
cargo build [--features=opencl]

# build release (optimized)
cargo build --release [--features=opencl]
```

### Run

```shell
engraver --help
```

### Donate 
* JohnnyDeluxe: BURST-S338-R6VC-LTFA-2GC6G
  - shabal optimizations
  - windows support
* bold: BURST-8V9Y-58B4-RVWP-8HQAV
  - architecture
  - linux support

