# Database format

## Finding the Database

The applications all should automatically detect the location of the scattering database.
By default, it is located in *'scatdb.hdf5'*, and contains tables for cross sections and phase functions.
If the database is not found, then it can be specified by setting the *'scatdb_db'* environment variable, or by specifying the *'-d {dbfile}'* option at the command prompt.


## Data Table Structure

CSV input and output files look like this (the HDF5 files internally have the same structure; this allows you to
easily add data):
```
flaketype,frequencyghz,temperaturek,aeffum,max_dimension_mm,cabs,cbk,cext,csca,g,ar
0,3.000000,273.149994,25.000000,0.120900,8.708887e-16,1.059305e-20,8.708959e-16,7.220166e-21,1.782000e-04,-1.000000
0,3.000000,273.149994,50.000000,0.241700,6.967110e-15,6.779529e-19,6.967572e-15,4.620890e-19,1.794500e-04,-1.000000
```

The columns are:
- flaketype - an integer that describes the type on snowflake (see below for a listing)
- frequencyghz - The frequency, in GHz.
- temperaturek - The temperature, in Kelvin
- aeffum - The radius of an equal-mass solid sphere of ice, expressed in micrometers.
- max_dimension_mm - The furthest distance in three dimensions between any two points on the ice particle. Units are millimeters.
- cabs - Absorption cross section (m^2)
- cbk - Backscatter cross section (m^2)
- cext - Extinction cross section (m^2)
- csca - Scattering cross section (m^2)
- g - Asymmetry parameter (dimensionless)
- ar - Aspect ratio (definition is a work in progress)

Flake Category Listing
-------

| Id | Source | Description |
| --- | ------ | -------- |
| 0 | Liu (2004) |  Long hexagonal column l/d=4 |
| 1 | Liu (2004) | Short hexagonal column l/d=2 |
| 2 | Liu (2004) | Block hexagonal column l/d=1 |
| 3 | Liu (2004) | Thick hexagonal plate l/d=0.2 |
| 4 | Liu (2004) | Thin hexagonal plate l/d=0.05 |
| 5 | Liu (2008) | 3-bullet rosette |
| 6 | Liu (2008) | 4-bullet rosette |
| 7 | Liu (2008) | 5-bullet rosette |
| 8 | Liu (2008) | 6-bullet rosette |
| 9 | Liu (2008) | sector-like snowflake |
| 10 | Liu (2008) | dendrite snowflake |
| 20 | Nowell, Liu and Honeyager (2013) | Rounded aggregates with aspect ratio near 0.9 |
| 21 | Honeyager, Liu and Nowell (2016) | Oblate aggregates with ar near 0.6 |
| 22 | Honeyager, Liu and Nowell (2016) | Prolate aggregates with ar near 0.6 |

## State of the database

The package supersedes Guosheng Liu's original scatdb release, as well as Holly Nowell's scatdb_ag database.

Flake categories are listed above.

- Ids 0-10 are for the pristine snowflakes of Liu (2004) and Liu (2008). There are data for 233, 243, 253, 263 and 273 K. Numerous frequencies have been included (3, 5, 9, 10, 13.4, 15, 19, 24.1, 35.6, 50, 60, 70, 80, 85.5, 90, 94, 118, 150, 166, 183, 220 and 340 GHz).
- Ids 20-22 are for bullet rosette aggregates, described in Nowell, Liu and Honeyager (2013) and Honeyager, Liu and Nowell (2016).
Over one thousand aggregates are included. We currently have results only for 263 K, but extrapolation is possible by examining the behavior of the Liu (2004,2008) particles over the appropriate ranges.
Ten frequencies are currently available (10.65, 13.6, 18.7, 23.8, 35.6, 36.5, 89, 94, 165.5 and 183.31 GHz).

All calculations were performed using [DDSCAT](http://www.ddscat.org/) versions 7.0-7.3. All dielectric indices are calculated using [Mätzler \(2006\)](http://www.atmos.washington.edu/ice_optical_constants/Matzler.pdf). Random particle orientations are presented in this database. This is achieved by averaging over thousands of possible orientations. The per-orientation and polarimetric data are retained for radar frequencies. This data may be obtained by contacting the authors directly.
