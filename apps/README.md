scatdb apps listing
========================

The example apps:
------------------------
These applications exist to show you how to programatically use the library.
There are examples for [C](./c), [C++](./cpp) and [Fortran](./fortran).

The C++ example application \(scatdb_example_cpp\) is capable of loading and
subsetting data. Either HDF5 or CSV files may be used as inputs or outputs. An
end user can even use this program to add new data into the scattering database.
This application is recommended for casual use of scatdb as a lookup table -
it is relatively easy to use this program to subset, bin and interpolate the raw data.
Its manual is [here](./cpp/README.md).

The bulk quantity apps:
------------------------
The three applications here allow you to calculate bulk quantities for a known
or assumed particle size distribution.

- The [scatdb_profile_parse](./profile_effective/README.md) application reads
the PSD from an input file and converts it into a format that the other applications
can understand. It is also possible to use this program to model the PSDs for
a well-described mathematical distribution \(e.g. gamma or log-normal\) with varied
bin sizes.

- The [scatdb_profile_evaluate](./profile_effective/README.md) application uses the 
PSD information and determines the effective backscatter, scattering, absorption 
and extinction crosss sections, as well as the effective asymmetry parameter and 
effective reflectivity. It subsets the scattering database according to user-provided
frequency and temperature ranges. If multiple frequency ranges are provided, it will
also calculate the dual frequency ratios.

- The [scatdb_profile_shapes](./profile_effective/README.md) application calculates
PSD-dependent bulk quantities that do not depend on frequency or temperature. These
include expected fall velocities and ice water contents.



Utility apps:
------------------------
These applications explore functions that supplement the library.

- The [scatdb_refract](./refract) application provides refractive index calculations
for a variety of substances. Dielectric formulas may depend on different combinations
of frequency, temperature and salinity; this program is designed to provide the
best guess of the refractive index to match the known parameters. Over fifteen
dielectric formulas are currently implemented.

- The [scatdb_units](./units) application provides an interface for unit
conversions. scatdb can convert between different temperature, length, mass
and density units. It can also perform spectral conversions \(i.e. frequency \[GHz\]
to wavelength \[millimeters\]\).


