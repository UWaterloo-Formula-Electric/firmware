===============
Cell Monitoring
===============

An important part of managing the high voltage battery on the car is
monitoring the individual cell voltage and temperatures of the high voltage
battery pack.

The relevant source files for cell monitoring include: `Src/batteries.c`,
`Src/F7_Src/ltc6811.c`, `Inc/batteries.h`, `Src/F7_Inc/batteries.h`.

*********************
Battery / Accumulator 
*********************

Battery Structure
=================

The battery is composed of a number of individual cells connected in parallel
and series to achieve the voltage and capacity requirements of the battery.

For more information on the cells, see this page in the accumulator space, or
the accumulator space as a whole.

Battery Safety
==============

In our battery, the cells are lithium ion (li-ion for short) cells, which
have a number of safety hazards associated with them that require careful
management.

From the BMU's perspective the key risks are in managing the voltage and
temperature of the cells. Too high or too low a voltage or temperature can
cause significant hazards, so these conditions must be avoided.

Check out `this document regarding Li-Ion safety`_ to learn more about the
risks of Li-Ion cells and how to manage them.

Cell Configuration Values
=========================

Relevant configuration related to the cells.

  .. doxygengroup:: CellConfig

******************************
Accumulator Measurement System
******************************

In order to monitor the cell temperature and voltages, the BMU communicates
with the Accumulators Measurement System (AMS) boards in the battery pack over
isoSPI (an electrically isolated version of the SPI interface, see the
`LTC6812 datasheet`_ for more info).

The main feature of an AMS board is a battery stack monitor chip. We've used
different part numbers over the years, but they're all from a similar series
of chips created by Analog Devices. Each AMS board is connected to a number of
cells, and it's battery stack monitor chip can be commanded to measure their
temperature and voltage.

The 2019-2020 AMS boards use the LTC6804/LTC6811 battery monitoring chips (the
6811 is just a newer version of the chip with identical interface). The 2021
AMS boards use the newer LTC6812 chips that also have an identical interface
but can measure up to 15 cells. 

Since the number of cells each LTC chip can monitor is less than the total
number of cells in the battery pack, multiple AMS boards are used each with
their own LTC chip. In order to communicate with all the AMS boards, the BMU
is connected to them over isoSPI in a daisy chain configuration. This means
the BMU is connected to the first AMS board (the AMS board's LTC chip) over
isoSPI, then the first AMS board is connected to the second AMS board over
isoSPI, and then each additional AMS board is connected to the previous AMS
board over isoSPI.

LTC681x Driver
==============

The driver for the LTC681x chips is implemented in the ltc6811.c and
ltc6811.h files.

Packet Error Code
-----------------

The Packet Error Code (PEC) is a 15-bit cyclic redundancy check (CRC) value.
The LTC681x calculates the PEC for any command or data received and compares
it with the PEC appended to the command or data. **Data is only regarded as
valid if the PEC matches.**

These functions in the LTC681x driver are responsible for generating and
checking the PEC for each isoSPI message.

.. doxygenfunction:: batt_gen_pec
.. doxygenfunction:: checkPEC

IsoSPI Communication
--------------------

Two functions are provided for communicating with the AMS boards.

.. doxygenfunction:: spi_tx_rx
.. doxygenfunction:: batt_spi_tx

Cell Voltages/Temperatures
--------------------------

Cell voltages are measured by internal ADCs on the LTC681x. Taking the
measurement takes a non-zero amount of time due to the ADC conversion time.
Therefore, we send the command to start measuring and read back the results
`VOLTAGE_MEASURE_DELAY_MS` milliseconds later.

Cell temperatures are measured by measuring the voltage drop through a voltage
divider of a device called a thermistor. Each thermistor is individually
selected by an external multiplexer chip through the GPIOs and and then
measured with the internal ADC through the GPIO5 pin.

.. doxygenfunction:: batt_read_cell_voltages 
.. doxygenfunction:: batt_read_cell_temps
.. doxygenfunction:: batt_readBackCellVoltage

To convert the measured voltage into a temperature, we estimate using a line
of best fit of the thermistor temperature/resistance curve.

.. doxygenfunction:: batt_convert_voltage_to_temp

Open Wire Test
--------------

The Open Wire Test checks if any of the cells have become disconnected from
the battery monitor chip. The Open Wire Test is run periodically by the BMU
to ensure that everything is being monitored correctly.

.. doxygenfunction:: checkForOpenCircuit
.. doxygenfunction:: performOpenCircuitTestReading
.. doxygenfunction:: sendADOWCMD

Balancing
---------

The LTC chip is responsible for balancing the cells it monitors. It
accomplishes this by switching MOSFETs to allow individual cells to discharge
through a discharge resistor.

This process does generate heat and the LTC will automatically shut down if
the LTC die temperature exceeds 150 degrees Celcius.

See the page at :doc:`charging` for more info on balancing.

.. doxygenfunction:: batt_balance_cell(int cell)
.. doxygenfunction:: batt_stop_balance_cell(int cell)
.. doxygenfunction:: batt_is_cell_balancing(int cell)
.. doxygenfunction:: batt_unset_balancing_all_cells
.. doxygenfunction:: batt_write_balancing_config
.. doxygenfunction:: batt_write_config

AMS Configuration
=================

Below are some defines that tell the measurement system (specifically the
LTC681x driver) the length of the isoSPI bus and number of cells per AMS board.

.. doxygengroup:: AccumulatorConfig

The LTC driver also needs to know what a particular board looks like:

.. doxygengroup:: AmsArchConfig

**********
References
**********

.. target-notes::

.. _`this document regarding Li-Ion Safety`: https://wiki.uwaterloo.ca/display/FESW/Managing+Li-Ion+Batteries+Safely?preview=/194645839/194645840/Li-ion-safety-July-9-2013-Recharge-.pdf

.. _`LTC6811 datasheet`: https://www.analog.com/media/en/technical-documentation/data-sheets/LTC6811-1-6811-2.pdf
.. _`LTC6812 datasheet`: https://www.analog.com/media/en/technical-documentation/data-sheets/ltc6812-1.pdf 
