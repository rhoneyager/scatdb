The bulk quantity apps:
===============================

The three applications here allow you to calculate bulk quantities for a known
or assumed particle size distribution.

- The scatdb_profile_parse application reads
the PSD from an input file and converts it into a format that the other applications
can understand. It is also possible to use this program to model the PSDs for
a well-described mathematical distribution \(e.g. gamma or log-normal\) with varied
bin sizes.

- The scatdb_profile_evaluate application uses the 
PSD information and determines the effective backscatter, scattering, absorption 
and extinction crosss sections, as well as the effective asymmetry parameter and 
effective reflectivity. It subsets the scattering database according to user-provided
frequency and temperature ranges. If multiple frequency ranges are provided, it will
also calculate the dual frequency ratios.

- The scatdb_profile_shape application calculates
PSD-dependent bulk quantities that do not depend on frequency or temperature. These
include expected fall velocities and ice water contents. This application reads raw
shape files (in hdf5 format) which have been preprocessed using scatdb_shape.

- The scatdb_profile_shape_from_scatdb program calculates the same quantities, but
using the data already available in the main scatdb database. Fall speeds and snowfall
rates are calculated from Locatelli and Hobbs (1974)'s fall speed equations for unrimed
and rimed aggregate snowflakes.

