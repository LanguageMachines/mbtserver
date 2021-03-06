[![GitHub build](https://github.com/LanguageMachines/mbtserver/actions/workflows/mbtserver.yml/badge.svg?branch=master)](https://github.com/LanguageMachines/mbtserver/actions/)
[![Language Machines Badge](http://applejack.science.ru.nl/lamabadge.php/mbt)](http://applejack.science.ru.nl/languagemachines/)

===================================
MbtServer: Tilburg Memory Based Tagger Server extension
===================================

    MbtServer 0.9 (c) CLST/ILK/CNTS 1998-2016

    Centre for Language and Speech Technology, Radboud University Nijmegen
    Induction of Linguistic Knowledge Research Group, Tilburg University and
    Centre for Dutch Language and Speech, University of Antwerp

**Website:** https://languagemachines.github.io/mbt

Comments and bug-reports are welcome on our issue tracker at
https://github.com/LanguageMachines/mbtserver/issues , or by mailing
lamasoftware (at) science.ru.nl .

MbtServer is distributed under the GNU Public Licence v3 (see the file COPYING)


This software has been tested on:
- Intel platform running several versions of Linux, including Ubuntu, Debian,
  Arch Linux, Fedora (both 32 and 64 bits)
- Apple platform running Mac OS X 10.10

Compilers:
- GCC. It is highly recommended to upgrade to at least GCC 4.8
- Clang

Contents of this distribution:
- sources
- Licensing information ( COPYING )
- Installation instructions ( INSTALL )
- Build system based on GNU Autotools
- example data files ( in the demos directory )
- documentation ( in the docs directory )

Dependecies:
To be able to succesfully build Timbl from the tarball, you need the
following pakages:
- pkg-config
- ticcutils (https://github.com/LanguageMachines/ticcutils)
- timbl (https://github.com/LanguageMachines/timbl)
- mbt (https://github.com/LanguageMachines/mbt)

To install MbtServer, first consult whether your distribution's package manager has an up-to-date package for TiMBL.
If not, for easy installation of Mbt, TiMBL, and all dependencies, it is included as part of our software
distribution LaMachine: https://proycon.github.io/LaMachine .

To compile and install manually from source instead, provided you have all the dependencies installed:

 $ bash bootstrap.sh
 $ ./configure
 $ make
 $ make install
