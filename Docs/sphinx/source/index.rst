======================================
Welcome to the 2018 BMU documentation!
======================================

The Battery Management Unit (BMU) is responsible for all things related to the
car's high voltage battery.

The intended use of this documentation is to document the specific
functionalities of the board alongside revelant functions and configuration
in order to provide a complete understanding of the code base.

Know where you need to get to? Then follow one of the following links here.

.. toctree::
   :maxdepth: 1
   :caption: Contents:

   files/state_machine
   files/tasks
   files/cell_monitoring
   files/charging
   files/pcdc
   files/soc
   files/cli
   files/style_guide

Main Functionality
------------------

The BMU performs a few main functions:
  * Monitoring the cell voltages and temperatures of the battery pack
  * Controlling the contactors to connect the motor controllers (or charger)
    to the battery pack in a safe manner (a.k.a. precharge/discharge)
  * Monitoring various safety systems and interlock loops 
  * Charging the battery pack

.. note:: Before proceeding with work on the BMU, **it is critical to read the
          Electrical Systems Form (ESF) documenting the car's electrical
          system, in particular Section 11 which details the system that
          manages the battery.**

Click on the "Next" button to read more about the BMU.
