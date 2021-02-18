###############
State of Charge
###############

State of charge calcuation occurs in `batteries.c`.

As part of the balance process, the cells state of charge (SoC) is checked.
The state of charge is the level of charge of a battery relative to its
capacity. The units of SoC are percentage points (0% = empty; 100% = full). In
depth information can be found in this `Wikipedia article on SoC`_. There are
a number of ways to estimate the state of charge, but for the 2020 car we use
the simple method of converting the voltage of the cell to the state of charge.

The following function is where our state of charge gets calculated.

.. doxygenfunction:: getSOCFromVoltage

A lookup table is created to convert from voltage to SoC. The range of safe
voltages is divided into evenly spaced intervals, and then the cell voltage is
used to index into the lookup table to get the SoC value.

.. doxygendefine:: NUM_SOC_LOOKUP_VALS
.. doxygenvariable:: voltageToSOCLookup

For example, with 10 lookup values, and safe voltage values from 3V to 3.9 V,
the indices of the lookup table correspond to 3.0, 3.1, 3.2, ..., 3.9 V, and
each lookup table value corresponds to the SoC at that index. This lookup
table method allows for modelling the nonlinear relationship between voltage
and SOC (the discharge curve).

For simplicity a linear model is assumed for the 2020 car. In our example
then, 3.0 V would be 0% SoC, and 3.4 V would be 50% SoC, and 3.9 V would be
100% SoC.

A possible area of future development would be to more accurately model the
SoC (the wikipedia page outlines a few other method), and a simple start would
be to create a non-linear lookup table.

**********
References
**********

.. target-notes::

.. _`Wikipedia article on SoC`: https://en.wikipedia.org/wiki/State_of_charge
