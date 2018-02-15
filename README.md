#CG_OBUP
=============

[![Build Status](https://travis-ci.org/jake-b/cg_obup.svg?branch=master)](https://travis-ci.org/jake-b/cg_obup)

This is the CryptoGuru Optimized BURSTcoin Plotter -
a BURST coin plotter that generates optimized plot files
without the need to run an optimizer after plotting.

It is intended to work on any UNIX system with a sufficiently sane
filesystem (able to pre-allocate space), but for now only Linux
has been tested. 64bit only!

dcct -> mdcct -> omdcct -> cg_obup

This version has some heritage and several people worked on
the code base until the result was what you see here.

    Markus Tervooren <info@bchain.info>           (BURST-R5LP-KEL9-UYLG-GFG6T)
    Cerr Janror <cerr.janror@gmail.com>           (BURST-LNVN-5M4L-S9KP-H5AAC)
    Niksa Franceschi <niksa.franceschi@gmail.com> (BURST-RQW7-3HNW-627D-3GAEV)
    a.k.a Mirkic7 <mirkic7@hotmail.com>
    Peter Kristolaitis <alter3d@alter3d.ca>       (BURST-WQ52-PUBY-N9WB-6J3DY)
    Jake-B (MacOS Build)                          (BURST-ZGEK-VQ86-M9FV-7SDWY)

and finally

rico666 <bots@cryptoguru.org>                 (Don't donate)


### Installing
    git clone https://github.com/rico666/cg_obup
    cd cg_obup
    make

### Usage:

```bash
./plot64 -k KEY [-x <core>] [-d <dir>] [-s <startnonce>] [-n <nonces>] [-m <staggersize>] [-t <threads>] [-a]
  -a
    Flag to use asynchronous writing mode. If this is set, the plotter can work
    even while data is being written to disk. It will give you more speed at the
    cost of more memory usage (will use double the memory!). Dafult is OFF.

  -R
    Resume from last position in an existing plot file.
    IMPORTANT: The plot file has to be created with the resume option to make resume work!
               Don't use the resume option if the plot file was created without the resume option!

  -d <directory>
    Which directory to use. You can give relative as well as absolute paths.
    If you omit this, plots are written into the 'plots' directory in the
    current path.
    
  -m <staggersize>
    In this version, you can think of this as the memory cache used by the program
    before a write to the disk is necessary. Obviously the more you give here, the
    less I/O is necessary. If not given, the program tries to use 80% of the free
    memory of the machine. Please be aware that in combination with the -a parameter
    the memory requirement is doubled!
    
  -n <nonces|spacedef>
    The number of nonces to plot. Each nonce is 256KB in size. If you do not
    give the number of nonces, the program will try to plot the maximum number
    of nonces that are possible according to the free disk space where
    <directory> resides.
    
  -s <startnonce>
    The offset from which to start plotting nonces. If not given, the program will
    simply choose an offset randomly.
    
  -t <threads>
    Number of threads to use when plotting. There is no "more is better".
    Depending on the number of physical cores of your CPU, and the core
    (see below) used, there will be an optimum. Probably the number of physical
    cores your CPU has.

  -v
    Verbose mode.
    
  -x <core>
    Define which SHABAL256 hashing core to use. Possible values are:
      0 - default core (*)
      1 - SSE4 core
      2 - AVX2 core
    Of course, SSE4 and AVX2 will work only on CPU architectures supporting
    these instruction sets. Otherwise the program will throw an "illegal instruction"
    error. You can assume a roughly 2x speed increase default->SSE4->AVX2 with
    AVX2 being roughly 4x faster than default. See also "Notes" below!

 ```
 
###### Notes

Calling the programm with wrong or incomplete command line, will print a rudimentary
usage information.

The file name will have a '.plotting' suffix while the file is incomplete, and then
renamed to the standard format if plotting is successful. 

AVX2/SSE4 usage: In order to achieve best performance, you must make sure that the
number of nonces to plot will match the number of threads like this:
* for SSE4: nonces is a multiple of threads * 4
* for AVX2: nonces is a multiple of threads * 8

If you do not match these numbers, the plotter will refuse to work for SSE4 and AVX2
cores, the default core will work on any arbitrary number of nonces.


For \<startnonce>, \<staggersize> and \<nonces> you can either define just a number or
add the T/t, G/g, M/m or K/k suffix. E.g. "-s 1234k"
* K/k = 1024
* M/m = 1024<sup>2</sup>
* G/g = 1024<sup>3</sup>
* T/t = 1024<sup>4</sup>
in which case the definition is not the number of nonces, but the memory used.

### TODO:

* thorough test suite
* optimizations
