########
Charging
########

The source code for charging is in `batteries.c`.

The BMU is also responsible for managing the charging of the cells of the high
voltage battery. It does this with the help of the AMS boards described in
`cell_monitoring.rst`, and the high voltage charger. The charge process
involves coordinating a few tasks.

.. note::

  For 2021, we are no longer using the charger board. Charging
  should be coordinated with a program to send CAN messages and a computer
  with a CAN adapter. Optionally, charging can be controlled directly from the
  BMU CLI.

**************
Charge Process
**************

Connection to Charger
=====================

In order to charge the battery needs to be connected to the charger.
This requires mechanically connecting the battery to the charger, but also
going through the precharge procedure to close the contactors and electrically
connect the battery to the charger.

As shown in the :doc:`state_machine`, there is a minor change in the precharge
tests when connected to the charger, as the capacitance of the charger is much
lower than that of the motor controllers.

Charge Voltage and Current
==========================

The BMU must communicate to the charger over a CAN bus in order to
set up the charging voltage and current. For more information on the CAN
communication, see the `charger CAN specification document`_.

.. note:: 
  For the 2021 car, the charger required a lower speed CAN bus than that of
  the rest of the car, so is on its own CAN bus connected to the BMU separate
  from that of the rest of the car.

There are two phases to charging, the constant current and constant voltage phase.

To achieve constant voltage, the charger voltage is set to the desired fully
charged voltage of the battery, which is equal to the fully charged voltage of
each cell multiplied by the number of series connected cells.

.. doxygenvariable:: maxChargeVoltage

To achieve constant current, the current limit is set to the desired value for
the constant current phase.

.. doxygenvariable:: maxChargeCurrent

The BMU has no control over the charge phases, that is completely up to the
charger to determine. The BMU just sets the limits and tells the charger when
to start or stop.

Cell Voltage Balancing
======================

The BMU must balance the individual cells in the battery pack. Balancing means
ensuring that the cells have the same voltage, as otherwise during the
charging process it is possible that some cells increase in voltage more than
others, and therefore have a different voltage.

Balancing is important! If the cells have different voltage, some cells
could be charged/discharged past their safe voltage limit, while others are
within a safe limit. To balance the cells, the BMU uses the AMS boards and
the LTC681x chips on them to discharge cells that have a higher voltage.

This is done through the use of discharge resistors present on the AMS board
which can be connected via a transistor switch across each of the cells on the
AMS board.

Balance Configuration
---------------------

.. doxygendefine:: BALANCE_START_VOLTAGE
.. doxygendefine:: BALANCE_MIN_SOC_DELTA
.. doxygendefine:: CELL_RELAXATION_TIME_MS
.. doxygendefine:: CHARGE_STOP_SOC
.. doxygendefine:: BALANCE_RECHECK_PERIOD_MS

Balance Charge Algorithm
========================

The balance algorithm for use during charging is as follows:

1. Start charging with voltage and current limits set as described above
2. Pause ongoing balancing and read cell voltages and temperatures (pausing
   balancing is necessary as otherwise the balancing affects the voltage
   reading of cells that are discharging)
3. Resume paused balancing of cells and verify that cell voltages and
   temperatures are within their safe limits
4. If the minimum cell voltage is above the minimum voltage to start
   balancing, start checking if cells should be balanced (the minimum balance
   voltage is done to speed up charging at low cell voltages and to avoid
   over-discharging cells below their safe voltage limit)
5. Get the state of charge (SoC) value of the cell with the minimum state of
   charge (SOC is explained below)
6. For each cell, if the cell SoC is more than `BALANCE_MIN_SOC_DELTA` %
   above the minimum SOC, balance the cell, otherwise don't balance the cell
7. If the minimum SOC value is above `CHARGE_STOP_SOC` %, and no cells
   are balancing, charging is finished. Otherwise, return to Step 2

.. doxygenfunction:: balanceCharge
.. doxygenfunction:: pauseBalance
.. doxygenfunction:: resumeBalance
.. doxygenvariable:: isCellBalancing

State of Charge
===============

As part of the balance process, the cells state of charge (SoC) is checked.
The state of charge is the level of charge of a battery relative to its
capacity. The units of SoC are percentage points (0% = empty; 100% = full).

Check out our page on :doc:`soc`.

******************
Charging Procedure
******************

#. Power up the charge cart. *Ensure HV contactors on charge cart are closed!*
#. Plug BMU into charger
#. Attempt to close the contactors with an HV Toggle Event.

   - This could come from the DCU or manually with the :doc:`cli`.

#. Initiate charging

   - If using CAN, connect a computer to the vehicle CAN bus and initiate
     charging using the provided program. **This is TODO! This program needs
     to be implemented**
   - If using CLI, run the following commands:
     
   .. code-block:: console

       canStartCharger
       maxChargeCurrent 1 # change this to desired current (in Amps)
       startCharge

#. Battery should begin charging, you should hear the charger fans spin up.


**********
References
**********

.. target-notes::

.. _`charger CAN specification document`: https://wiki.uwaterloo.ca/x/lISzCw
