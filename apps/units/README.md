scatdb-units
===============

This is a program designed to perform basic unit interconversions.
For now, only basic unit conversions between common mass, length, density, temperature
and frequency units is supported. Eventually, a udunits-based interface is envisioned.

How to run
--------------

Execution is relatively simple. Just run the program. If no arguments are passed,
then follow the prompts specifying the input quantity, input units and output units.

The code has support for performing spectral conversions (i.e. frequency to wavelength).
You will be prompted if you wish to do this.

If the units are convertible, then a value is output. If the units cannot be converted,
then an error message is presented.

###Command-line arguments:

| Option | Required type | Description |
| ------ | ------------- | ----------- |
| --input (-i) | float | This is the input quantity (sans units) |
| --input-units (-u) | string | These are the input units |
| --output-units (-o) | string | The output units |
| --spec | \<none\> | Pass this option if a spectral conversion is desired. |

The input quantity, input units and output units options are also positional.
So, the command `scatdb-units 10 m um` will convert 10 meters to micrometers.
**Note: Negative quantities will mess up the positional parsing. In these cases, specify the input value with -i.**
