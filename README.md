#ENGRAVER
=============

[![Build Status](https://travis-ci.org/jake-b/cg_obup.svg?branch=master)](https://travis-ci.org/jake-b/cg_obup)

This is the PoCC reference plotter for Burstcoin.

It generates PoC2 files and is intended to work on any UNIX system
with a sufficiently sane filesystem (able to pre-allocate space), but
for now only Linux and MacOS has been tested. 64bit only!

dcct -> mdcct -> omdcct -> cg_obup -> engraver

This version has some heritage and several people worked on
the code base until the result was what you see here.

    Markus Tervooren <info@bchain.info>           (BURST-R5LP-KEL9-UYLG-GFG6T)
    Cerr Janror <cerr.janror@gmail.com>           (BURST-LNVN-5M4L-S9KP-H5AAC)
    Niksa Franceschi <niksa.franceschi@gmail.com> (BURST-RQW7-3HNW-627D-3GAEV)
    a.k.a Mirkic7 <mirkic7@hotmail.com>
    Peter Kristolaitis <alter3d@alter3d.ca>       (BURST-WQ52-PUBY-N9WB-6J3DY)
    Brynjar Eide <brynjar@segfault.no>            (BURST-5WLT-TP7V-6B7S-CZRQP)
    Jake-B (MacOS Build)                          (BURST-ZGEK-VQ86-M9FV-7SDWY)

and finally

rico666 <bots@cryptoguru.org>                 (Don't donate)


### Installing
    git clone https://github.com/PoC-Consortium/engraver
    cd engraver
    make

### Usage:

```bash
./plot64 -k KEY [-x <core>] [-d <dir>] [-s <startnonce>] [-n <nonces>] [-m <staggersize>] [-t <threads>] [-a] [-D]
  -a
    Flag to use asynchronous writing mode. If this is set, the plotter can work
    even while data is being written to disk. It will give you more speed at the
    cost of more memory usage (will use double the memory!). Default is OFF.

  -b <maxmemory>
    Maximum amount of memory to use. Will automatically be halved when used in
    combination with -a.

  -R
    Resume from last position in an existing plot file.
    IMPORTANT: The plot file has to be created with the resume option to make resume work!
               Don't use the resume option if the plot file was created without the resume option!

  -d <directory>
    Which directory to use. You can give relative as well as absolute paths.
    If you omit this, plots are written into the 'plots' directory in the
    current path.

  -f <diskspace>
    When -n is not specified, leave this much disk space while calculating number
    of nonces to plot.

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

  -p <plotfilesize>
    Attempt to create a plot file of this size. May not be combined with -n, since
    the amount of nonces will be calculated from the file size.

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

  -D
    Use Direct I/O to avoid making the system very slow by flushing the buffer
    cache.

 ```

###### Notes

A word of warning for SMR drives: Plotting on these will seem
unbearable. You have been warned. Plot on PMR drives instead, then
copy the plot file to a SMR drive. If you do not know the difference
between SMR and PMR, don't plot until you do.

Calling the program with wrong or incomplete command line, will print a rudimentary
usage information.

The file name will have a '.plotting' suffix while the file is incomplete, and then
renamed to the standard format if plotting is successful.

AVX2/SSE4 usage: In order to achieve best performance, you must make sure that the
number of nonces to plot will match the number of threads like this:
* for SSE4: nonces is a multiple of threads * 4
* for AVX2: nonces is a multiple of threads * 8

If you do not match these numbers, the plotter will refuse to work for SSE4 and AVX2
cores, the default core will work on any arbitrary number of nonces.


For \<startnonce>, \<staggersize>, \<nonces>, \<maxmemory>, \<plotfilesize> and
\<diskspace> you can either define just a number or add the T/t, G/g, M/m or K/k suffix.
E.g. "-s 1234k"
* K/k = 1024
* M/m = 1024<sup>2</sup>
* G/g = 1024<sup>3</sup>
* T/t = 1024<sup>4</sup>
in which case the definitions for \<staggersize> and \<nonces> are not the number of
nonces, but the memory used.

### Tuning tipps for ext4 users:

If your drive only contains plot files then following tuning options are recommended.
Execute the following command on unmounted partitions.

1. Disable journal. Improves plot performance and is not needed when mining:

```tune2fs -O ^has_journal /dev/sdX```

2. Disable reserved blocks for root (gives you 5% more disk space):

```tune2fs -m 0 /dev/sdX```

3. To be sure the file system is clean:

```fsck /dev/sdX```

4. Adjust mount options for further tuning:

```UUID=<UUID> <mount-point> ext4 defaults,x-gvfs-show,noatime,nodiratime,nobarrier 0 2```

When mining it is recommended to add the option ```ro``` to avoid sudden damages of the file system.


### TODO:

* GPU support
* optimizations
* BFS support
