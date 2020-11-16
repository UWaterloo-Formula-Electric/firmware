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

## Can Monitor

Monitor incoming data off a specified CAN interface (default `can0`) and send it to a message queue to the display and a log file.  

## Installation

This project utilizes ZeroMQ for IPC. When installing, pyzmq will try to find libzmq/build it's own. This was taking a long time on my BeagleBone. Speed up the dependency installation process by running

`sudo apt-get install libzmq3-dev` 

prior to running

`pip install -r requirements.txt`

To ensure your setup is correct, I found it was useful to use can-utils and cantools to monitor the interface and decode messages: 

`candump can0 | cantools decode <example>.dbc`

## Running the script

In order to decode the messages, a DBC file is needed. The script by default requires that a DBC named "2018CAR.dbc" is located in `common-all/Data/2018CAR.dbc`, however an alternative path can be specified.

**TODO**:
* Write an install script to install this script as a service that automatically runs

