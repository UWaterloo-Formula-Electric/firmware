# 2020 Beaglebone App

## Repository Contents

```
.
├── README.md
├── requirements.txt
├── setup.py
├── tests/
└── wfe
    ├── dashboard.py
    ├── database
    │   ├── database.py
    │   └── schema.py
    ├── can_monitor.py
    ├── can_simulator.py
    └── queue
        ├── queue.py
        └── packet.py
```

## CAN Monitor

Monitor incoming data off the default CAN interface (default `can1`) and send
it to a message queue to the display and a log database.

## Installation

### ZeroMQ

This project utilizes ZeroMQ for Interprocess Communication (IPC).
**Important**: When installing, pyzmq will try to find libzmq/build it's own.
This was taking a long time on my BeagleBone. Speed up the dependency
installation process by running

```
sudo apt-get install libzmq3-dev
```

prior to installing the package. This will download a pre-exsting ZeroMQ
binary so you don't have to build your own.

### WFE Package

To install this repository as a package, run

```
pip install .
```

in this directory.

### CAN Interfacing

This package relies heavily on having a SocketCAN interface properly set up
to interface with. To ensure your CAN setup is correct, I found it was useful
to use can-utils and cantools to monitor the interface and decode messages:

```
candump can0 | cantools decode <example>.dbc
```

If you need a DBC file to test, installing this package installs a version of
the DBC to your `site-packages` in your default Python install location.

## Simulating CAN Messages

For an extensive guide, refer to our [Confluence page](https://wiki.uwaterloo.ca/display/FESW/Simulation+of+CAN+Messages).

With a virtual bus, we can simulate CAN messages on the Beaglebone.

First, set up the virtual bus by running

```
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

For help on how to start the simulation, run `wfe-simulator -h` or
`wfe-simulator --help`.

```
usage: wfe-simulator [-h] [-d DBC] [-j JSON] [-t TIME]

Run CAN Simulations with CAN monitor.

optional arguments:
  -h, --help            show this help message and exit
  -d DBC, --dbc DBC     DBC file (default: ../../../../common/Data/2024CAR.dbc)
  -j JSON, --json JSON  JSON file (default: json/heartbeat.json)
  -t TIME, --time TIME  Duration of simulation in ms (default: 5000)
```

So if you want to use the default values, you can just run `wfe-simulator`.

## Development

To develop for this package, it's useful to set up a virtual environment:

```
python3 -m venv venv
```

This has now set up a fresh Python installation with no other Python packages
installed. Any packages you install while in the virtual environment
(indicated by the `(venv)` next to your shell prompt) will be unique to your
virtual environment. This is useful for development because your project's
package dependencies are now buffered from changes to any other pre-existing
system-wide packages that may also happen to be dependencies of this project.
It's also useful to ensure that our package will install cleanly on a brand
new system.

Read more about the virtual environment package [here](https://docs.python.org/3/tutorial/venv.html)

Enter the virtual environment with

```
source venv/bin/activate
```

Install the package as a development package:

```
python setup.py develop
```

It's now technically installed as a package to your virtual environment.
However, it's linked to the development directory so you can still update the
code in the repository here without reinstalling the package to your system.

I'm pretty sure you'll need to re-run this command if you change anything like
an `entry_point` or `package_data` since these are only configuring during
package installation but for simply editing a Python files, you should be
good.

### Running tests

In the root project files (contains `tests/`):

```
python -m unittest
```

If you want to execute a specific test:

```
python -m unittest tests/<test-name>.py
```
