#CG_OBUP
=============

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

and finally

rico666 <bots@cryptoguru.org>                 (Don't donate)


### Installing
    git clone https://github.com/rico666/cg_obup
    cd cg_obup
    make

### Usage
###### Notes
    The file name will have a '.plotting' suffix while the file is incomplete, and then renamed to the
    standard format if plotting is successful.

    Usage:
```bash
./plot64 -k KEY [ -x CORE ] [-d DIRECTORY] [-s STARTNONCE] [-n NONCES] [-m STAGGERSIZE] [-t THREADS] [-a]
```
      CORE:
        0 - default core
        1 - SSE4 core
        2 - AVX2 core
       -a = ASYNC writer mode (will use 2x memory!)
 
###### Not specifying -x option will use default core


TODO:

thorough test suite
optimizations
