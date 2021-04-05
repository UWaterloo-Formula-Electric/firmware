=====================
Precharge / Discharge
=====================

Contactor control via the precharge / discharge (PCDC for short) algorithm is
another important part of managing the vehicle safely.

Relevant files for precharge / discharge control are
`Src/prechargeDischarge.c`, `Inc/prechargeDischarge.h`.

.. note:: The Precharge & Discharge Section of the Electrical Systems Form
          (ESF) details this functionality of the vehicle. It also contains
          some diagrams we don't maintain in the code here that are very
          helpful in understanding how this part of the system works.

*****************
Contactor Control
*****************

In order to safely connect the high voltage battery to the motor controllers,
the electrical system uses contactors. The contactors electrically connect
the battery positive and negative terminal to the motor controllers positive
and negative terminal. Similar to a relay, they are electrically controllable,
allowing the connection to be opened and closed. As well, they are normally
open, meaning that they require current flow through their control terminals
to remain closed.

The following functions are used by the PCDC algorithm to control the
contactors:

.. doxygenfunction:: setNegContactor
.. doxygenfunction:: setPosContactor
.. doxygenfunction:: setPrechargeContactor
.. doxygenfunction:: openAllContactors

.. doxygenenum:: ContactorState_t

Opening the Contactors
======================

Opening the contactors will disconnect the motor controllers from the high
voltage battery pack and therefore isolate the high voltage of the pack from
the rest of the car. Clearly, this is part of a key safety system on the car!

The contactors will open when:
    1. We are disabling the high voltage on the car via an HV Toggle event.
    2. A fault happens. This is enforced at the firmware and the electrical
       system level.

Before opening, the BMU will broadcast a DTC to the rest of the car warning
that the contactors are about to close.

Opening the contactors is controlled by the `Discharge` procedure.

Closing the Contactors
======================

Closing the contactors will connect the motor controllers to the high voltage
battery pack. This is done in a controlled manner, as the large capacitance
of the motor controllers would lead to a very large current flowing through
the contactors if they were both closed at the same time.

The contactors will only close when we are enabling the high voltage on the
car via an HV Toggle event.

Closing the contactors is controlled by the `Precharge` procedure.

Precharge Discharge (PCDC) Contactor
====================================

This is an additional relay (often referred to as a contactor, although it is
technically a relay) called the precharge discharge relay (PCDC relay), as
well as a power resistor. This is the key to the whole PCDC procedure that
allows us to charge / discharge the high voltage bus in a controlled manner.

*******************************
Precharge / Discharge Algorithm
*******************************

In summary, the algorithm does a series of tests to ensure the contactors are
operating correctly while also charging the motor controllers in a controlled
manner.

A common failure mode of contactors is that they become welded closed due to
the high currents that can occur when they are incorrectly operated and fail
closed. In other cases, they can fail open. Therefore, as part of the
precharge procedure, a series of tests are performed to test whether either of
the contactors or the PCDC relay have failed (either open or closed). If a
failure is detected, the precharge procedure is aborted and an error raised.

Use this define to enable contactor control:

.. doxygendefine:: ENABLE_PRECHARGE_DISCHARGE

Precharge
=========

The precharge procedure is used to slowly charge up the capacitors in the
motor controllers, limiting the current flow. This is done with the PCDC
contactor and a power resistor. In order to precharge the motor controller,
one of the contactors is closed, and the precharge relay connects the
precharge resistor across the other contactor, thus allowing current to flow
into the motor controllers through the resistor, where the resistor limits the
current flow. Once the motor controller is at the same (or almost the same)
voltage as the battery, then the other contactor is closed.

.. doxygenfunction:: precharge

Connected to Charger
--------------------

There is a minor change in the precharge algorithm when connected to the
charger, as the capacitance of the charger is much lower than that of the
motor controllers.

.. doxygenenum:: Precharge_Type_t

Discharge
=========

Related to the precharge procedure is the discharge procedure. This uses the
PCDC relay and resistor to discharge the motor controllers once the contactors
are open. If this was not done, then the capacitance of the motor controllers
would mean that even once the contactors are open and the battery
disconnected, the motor controllers would retain a high voltage across their
terminals.

.. doxygenfunction:: discharge

Return Types
============

.. doxygenenum:: Precharge_Discharge_Return_t
