# 2020 Beaglebone App

## Repository Contents

```
.
├── README.md
├── app
├── dashboard
│   ├── dashboard.py
│   ├── database
│   │   ├── database.py
│   │   └── schema.py
│   ├── can_monitor.py
│   └── queue
│       ├── queue.py
│       └── packet.py
└── requirements.txt
```

## CAN Monitor

Monitor incoming data off a specified CAN interface (default `can0`) and send it to a message queue to the display and a log file.  

## Installation

This project utilizes ZeroMQ for IPC. When installing, pyzmq will try to find libzmq/build it's own. This was taking a long time on my BeagleBone. Speed up the dependency installation process by running

```
sudo apt-get install libzmq3-dev
```

prior to running

```
pip install -r requirements.txt
```

To ensure your setup is correct, I found it was useful to use can-utils and cantools to monitor the interface and decode messages: 

```
candump can0 | cantools decode <example>.dbc
```

## Running the script

In order to decode the messages, a DBC file is needed. The script by default requires that a DBC named "2018CAR.dbc" is located in `common-all/Data/2018CAR.dbc`, however an alternative path can be specified.

**TODO**:
* Write an install script to install this script as a service that automatically runs

## Simulating CAN Messages

For an extensive guide, refer to our [Confluence page](https://wiki.uwaterloo.ca/display/FESW/Simulation+of+CAN+Messages).

With a virtual bus, we can simulate CAN messages on the Beaglebone.

First, set up the virtual bus by running

```
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

For help on how to start the simulation, run `python3 can_simulator.py -h` or `python3 can_simulator.py --help`.

```
usage: can_simulator.py [-h] [-d DBC] [-j JSON] [-t TIME]

Run CAN Simulations with CAN monitor.

optional arguments:
  -h, --help            show this help message and exit
  -d DBC, --dbc DBC     DBC file (default: common-all/Data/2018CAR.dbc)
  -j JSON, --json JSON  JSON file (default: json/heartbeat.json)
  -t TIME, --time TIME  Duration of simulation in ms (default: 5000)
```

So if you want to use the default values, you can just run `python3 can_simulator.py`.
