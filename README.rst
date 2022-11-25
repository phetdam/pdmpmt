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
   results in an estimation of pi. However, this estimate tends to be very
   rough.

Implementations
---------------

The following table summarizes the available language/hardware implementations
for each example from `Examples`_. Here "serial" refers to a standard
single-core, single-thread implementation.

+----------+----------+----------+---------------------+
| Example  | Language | Hardware | Implementations     |
+==========+==========+==========+=====================+
| ``mcpi`` | C        | CPU      | serial              |
|          |          |          +---------------------+
|          |          |          | OpenMP              |
+          +----------+----------+---------------------+
|          | C++      | CPU      | serial              |
|          |          |          +---------------------+
|          |          |          | ``std::async`` [#]_ |
|          |          |          +---------------------+
|          |          |          | OpenMP              |
+----------+----------+----------+---------------------+

.. [#] ``std::async`` from ``<future>`` was used for asynchronous
   multithreading, in contrast to synchronous multithreading through OpenMP_
   via ``#pragma omp`` directives.

.. _OpenMP: https://www.openmp.org/

Building from source
--------------------

TBA. The ``build_cpp.(sh|bat)`` scripts work to build the C/C++ code, which
requires `CMake`_. The Python code (for now) has no compiled components and can
be installed with ``pip install .``, which requires `setuptools`_.

.. _CMake: https://cmake.org/cmake/help/latest/

.. _setuptools: https://setuptools.pypa.io/en/latest/

Building GSL from source
------------------------

Windows
~~~~~~~

Working with DLLs on Windows is a huge pain, especially because any DLL must
have its directory present in the ``PATH`` variable in order to be found at
runtime, unlike on Linux, where ``ldconfig`` can search its cache to find the
desired shared library and where it is located. Although GSL *is* GNU software,
and therefore originally intended for use on \*nix systems, it can be built
with CMake for Windows, either as a static or shared library.

Following the comments in the top-level ``CMakeLists.txt``, one can configure
the native Visual Studio generator to build x86 DLLs for GSL with the following
CMake configuration command line

.. code:: batch

   cmake -S . -B build_windows_x86_2.7_shared -A Win32 ^
       -DGSL_INSTALL_MULTI_CONFIG=1 -DBUILD_SHARED_LIBS=1 -DNO_AMPL_BINDINGS=1

To build x64 DLLs, just replace ``-A Win32`` with ``-A x64``, and omit the
``-DBUILD_SHARED_LIBS=1`` to build static libraries instead. Note that I have
separated source and build directories, and at time of writing was using GSL
2.7. Then, one can build the respective Debug and Release configurations with

.. code:: batch

   cmake --build build_windows_x86_2.7_shared --config Debug -j

.. code:: batch

   cmake --build build_windows_x86_2.7_shared --config Release -j

If you would like to use fewer parallel jobs, e.g. 4, you can use ``-j 4``
instead. Then, the resulting Debug and Release build artifacts can be installed
to your directory of choice with the lines

.. code:: batch

   cmake --install build_windows_x86_2.7_shared ^
       --prefix install_windows_x86_2.7_shared --config Debug

.. code:: batch

   cmake --install build_windows_x86_2.7_shared ^
       --prefix install_windows_x86_2.7_shared --config Release

If ``--prefix`` is not specified, then ``CMAKE_INSTALL_PREFIX`` will be used,
which by default will attempt something like ``C:\Program Files\gsl``, which
will require you to run the Developer Command Prompt as administrator.
