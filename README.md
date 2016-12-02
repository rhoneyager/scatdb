## Synopsis

SCATDB is a lookup table of snow particle scattering properties - scattering, absorption, extinction cross sections, asymmetry parameter and phase functions - for randomly oriented ice particles of various shapes. The properties are all computed using the Discrete Dipole Approximation (DDA).

## Improvements

This version improves upon the initial SCATDB release in several ways.

- In addition to bullet rosettes, sector and dendritic snowflakes and hexagonal columns, this release adds over a thousand aggregate snowflakes.
- More frequencies are considered.
- The library has been rewritten to improve accessibility. It has C and C++ interfaces. The snowflakes may be binned and distribution statistics may be easily obtained...

For users of previous versions, you might want to take a look at the [CHANGELOG](./changelog.md).

## Building and Installing

[See INSTALL.md](./INSTALL.md).

## Using

### Main example application

For most users, the scatdb_example_cpp program is a good place to start. This program reads the database and performs subsetting based on user-provided input. For example, you can filter and write out all data around 13.6 GHz at around 263 K for only aggregate snowflakes with the following command:
```
scatdb_example_cpp -y 20,21,22 -f 13/14 -T 259/265 -o filtered_1.csv
```

The scatdb_example_cpp application provides several options for reading and subsetting the database. A manual is [here](./apps/cpp/README.md).
You can subset by frequency, temperature, ice solid sphere-equivalent effective radius, maximum dimension, aspect ratio, and snowflake classification.
These options all read a string, and the string can be a comma-separated list (e.g. for flake types, *'-y 5,6,7,8'* to select small bullet rosettes only)
or can be expressed in a range notation (e.g. for frequencies between 13 and 14 GHz: *'-f 13/14'*).
**Because floating-point values are not exact, frequencies and temperatures should always be expressed in range notation to the nearest GHz or degree K.**

The selected data can be re-saved at another location, by specifying an output file. HDF5 files are detected by specifying a .hdf5 extension. This file format is recommended because it preserves the phase functions of the particles. Comma-separated-value files can also be written by specifying a .csv extension. These files can export either only 1) the overall single scattering properties or 2) the phase function for a single database record. To re-use the selected data, you can tell the program to load the saved hdf5 file with the -d option.

Once a selection is made, it is possible to calculate the statistics with the *'--stats'* option. By combining this option with filtering, it is possible to calculate binned quantites. An example application is binning over particle size bins produced by another instrument. This way, it is possible to efficiently calculate effective radar reflectivity for different PSDs and for different particle populations.

### Reading the results / importing other data into this library

[SEE HERE.](./dbformat.md)

### Other applications / library

Other example programs are in the [apps subdirectory](./apps/).

SCATDB is primarily written in C++, though it has an extensive C interface. Fortran programs may link to the library using the C interface.
C++ header files are denoted with the .hpp extension, and C-only headers use .h.


Papers
---------

[Liu 2004](http://dx.doi.org/10.1175/1520-0469(2004)061%3C2441:AOSSPO%3E2.0.CO;2)

[Liu 2008](http://journals.ametsoc.org/doi/abs/10.1175/2008BAMS2486.1)

[Nowell, Liu, Honeyager 2013](http://onlinelibrary.wiley.com/doi/10.1002/jgrd.50620/abstract)

[Honeyager, Liu, Nowell 2016](http://dx.doi.org/10.1016/j.jqsrt.2015.10.025)

## Work in progress / planned work

- Implementing phase function tables in the code
- Adding 220 GHz aggregate snowflake results
- More work on the C interface is underway. An improved Fortran code is also anticipated.
- Support for interpolation / extrapolation
- Support for sorting
- Support for estimating mean scattering behavior using the Locally-Weighted Scatterplot Smoothing (LOWESS) algorithm
- Debian / Ubuntu / Fedora / Windows binary installable packages
- Mac OS X and FreeBSD porting.

## License

The scattering database and the associated code are released under the [MIT License](https://opensource.org/licenses/MIT).

## Credits

An early implementation of the LOWESS algorithm is provided by Peter Glaus [see here](http://www.cs.man.ac.uk/~glausp/) and [here](https://github.com/BitSeq/BitSeq) (Artistic License 2.0).

The Chain Hull algorithm is implemented from http://geomalgorithms.com/a10-_hull-1.html

The MurmurHash3 algorithm was written by Austin Appleby, who placed it in the public domain. [site] (https://github.com/aappleby/smhasher).

The original scatdb and scatdb_ag databases are available [here](http://cirrus.met.fsu.edu/research/scatdb.html).


## Problems / Suggestions / Contributions

Contact Ryan Honeyager \(<rhoneyager@fsu.edu>\) or Guosheng Liu \(<gliu@fsu.edu>\).

