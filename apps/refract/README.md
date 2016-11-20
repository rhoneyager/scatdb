scatdb-refract
===============

This program calculates the refractive index of a bulk medium, such as solid ice or liquid water.
Mixtures (and the effective medium approximation) will be handled in a separate application.

How to run
--------------

###Command-line arguments:

| Option | Required type | Description |
| ------ | ------------- | ----------- |
| --list-all | none | List all possible refractive index providers |
| --list-subst | string | List all refractive index providers for a given substance (viz. water, ice) |
| --subst | string | The substance of interest |
| --freq | double | Frequency |
| --freq-units | string | Units for the frequency (defaults to GHz) |
| --temp | double | Temperature |
| --temp-units | string | Units for the temperature (defaults to K) |

The arguments are also positional, with ordering of substance, frequency, frequency units, temperature,
temperature units. The temperature and associated units are optional.

**Note: when entering temperatures in degrees Celsius, negative temperatures cannot be specified using positional arguments.
In this case, you must specify temperature using ```--temp {temp}```

This works:

```scatdb-refract ice 30 GHz 263 K```

This also works:

```scatdb-refract -f 18.7 GHz --subst ice```

So does this:

```scatdb-refract --subst water -T 10 --temp-units C -f 20 --freq-units um```
