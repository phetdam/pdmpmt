.. README.rst

pdmpmt
======

Exploring mixed-hardware parallel computing through practice.

TBA: CPU, GPU, multiprocess, multithread examples in Python, C++, C.

Examples
--------

``mcpi``
   Monte Carlo estimation of pi by repeated sampling points in [-1, 1] x [-1, 1]
   and then counting the points that fall in the unit circle. Multiplying the
   ratio of number of points in the unit circle to the total sample count by 4
   results in an estimation of pi.

Implementations
---------------

The following table summarizes the available language/hardware implementations
for each example from `Examples`_. Here "serial" refers to a standard
single-core, single-thread implementation.

+----------+----------+----------+---------------------+
| Example  | Language | Hardware | Implementations     |
+==========+==========+==========+=====================+
| ``mcpi`` | C        | CPU      | serial              |
+          +----------+----------+---------------------+
|          | C++      | CPU      | serial              |
|          |          |          +---------------------+
|          |          |          | ``std::async`` [#]_ |
|          |          |          +---------------------+
|          |          |          | `OpenMP`_           |
+----------+----------+----------+---------------------+

.. _OpenMP: https://www.openmp.org/

.. [#] ``std::async`` from ``<future>`` was used for asynchronous
   multithreading, in contrast to multithreading through OpenMP_.

Building from source
--------------------

TBA. The ``build_cpp.(sh|bat)`` scripts work to build the C/C++ code, which
requires `CMake`_. The Python code (for now) has no compiled components and can
be installed with ``pip install .``, which requires `setuptools`_.

.. _CMake: https://cmake.org/cmake/help/latest/

.. _setuptools: https://setuptools.pypa.io/en/latest/
