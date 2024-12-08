.. README.rst

third_party
===========

This directory contains external source code used by this project.

Unless indicated, none of the external code has been modified at all.

Contents
--------

prand
~~~~~

A C99 library for pseudorandom number generation with either the 32-bit
Mersenne Twister or the MRG32k3a_. The source used has been checked out of the
prand_ repo at commit ``37c5bba`` and has not been modified.

For ease of integration into the project, a simple CMake configuration has been
added that builds libprand as a static library for ingestion by downstream
targets in this project. On ELF systems, if ``BUILD_SHARED_LIBS`` is ``ON``,
then the object code emitted is position-independent (no effect for COFF/PE32)
to allow linking into shared libraries.

.. _prand: https://github.com/cheng-zhao/prand/

.. _MRG32k3a: https://www.intel.com/content/www/us/en/docs/onemkl/developer-
   reference-vector-statistics-notes/2021-1/mrg32k3a.html
