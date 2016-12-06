2016-12-05 - Version 2.0.2 - Beta release

- Builds and works on FreeBSD
- Added programs to calculate radar reflectivity, IWC, snowfall rates and terminal
  velocities from known profiles. Example input files are provided in share/.
- Added ability to read and write shape files in DDSCAT, ADDA and HDF5 formats.
- Improved documentation
- Fall speeds and snowfall rates may be calculated from Locatelli and Hobbs [1974]
  and Heymsfield and Westbrook [2010].

2016-11-20 - Version 2.0.1 - alpha release

- Added the rest of the scatdb interface functions in C.
  Fortran interface will come later.
- Fixed build system so that Clang gets the right compiler flags.
- Added note that OS X and BSD builds will not work due to
  missing /proc filesystem. Will be fixed later.
- Added refractive index code and application.
- Added unit conversion code and application.
- Added profile reading and reflectivity-calculating application.

2016-11-08 - Version 2.0.0 - alpha release. Commit 302de5f

- Initial release!
