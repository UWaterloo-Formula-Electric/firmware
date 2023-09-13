# UWFE Firmware

This is the firmware monorepo for the UW Formula Electric team. This repository contains vehicle/HIL board firmware, cell tester firmware, and our firmware validation testbed.

# Onboarding Instructions

- [OpenProject Access](#open-project)
- [SSH Key](#ssh-key-set-up)
- [Vagrant (Windows Only)](#Vagrant-Environment-Set-Up-for-Windows-Users)
- [Mac OS](#mac-os-set-up)
- [Linux](#linux-set-up)
- [Other Resources](#other-resources)
- [FAQ](#FAQ)

# Open Project

We use Open Project for task management. Steps to get set up:

1. Visit http://owenbrake.com/pm
2. Create an account with your school email
3. Message your email in the new-members thread under #firmware on Slack, and a lead will add you to the project

# SSH Key Set Up

SSH key is used for access credentials (think of it as your username and password). Without it, you can't contribute to the repository.

1. Create a SSH key by following the instructions [here](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent)
2. Add the SSH key to your GitHub account by following this [page](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent)

Note: You can skip this step if you already have a SSH key configured on GitHub

# Vagrant Environment Set Up for Windows Users

### Overview

Vagrant is an open-source software product for building and maintaining portable virtual software development environments. We will be using it to create an Ubuntu VM for VirtualBox.
The repository we use is found [here](https://github.com/UWaterloo-Formula-Electric/vagrant).

### Prerequisites

- Install the latest version of [Vagrant](https://developer.hashicorp.com/vagrant/downloads)
- Install the latest version of [VirtualBox](https://www.virtualbox.org/wiki/Downloads)
- Install the matching [VirtualBox Extension Pack](https://www.virtualbox.org/wiki/Downloads) to get USB data
  After installing the above, verify the Vagrant installation worked by opening a new command prompt or console, and checking that **vagrant** is available.

```
$ vagrant
Usage: vagrant [options] <command> [<args>]

    -v, --version                    Print the version and exit.
    -h, --help                       Print this help.

# ...
```

### Getting Started

Make sure everything under **Prerequisites** has been installed, and that you have an SSH key added for your host computer to have GitHub access (check the link above).

Then, run the following commands:

```
git clone git@github.com:UWaterloo-Formula-Electric/vagrant.git
cd vagrant
vagrant up
vagrant reload
```

(may take a while to complete)
The commands above clone the vagrant repo which you only need to do this one time in your set up. The next time, you can just call `vagrant up`. When its done, you will see a login window pop up. Do not maximize the window when on the login page (for some reason, this causes it to freeze). Click on the user "vagrant" and login with the password: **vagrant**.

The **shared/** directory in the VM is shared between your host computer (your laptop) and the virtual environment. The location in your host computer is the path to where you cloned the vagrant repo.

To turn off the machine, run `vagrant halt` in the same terminal.

### Clone Firmware

Clone the firmware repository into the `vagrant/shared/` directory on your host computer. Navigate to where you cloned the **vagrant** repo.

```
cd shared
git clone git@github.com:UWaterloo-Formula-Electric/firmware.git

```

### Troubleshooting

This [document](http://208.68.36.87/projects/firmware/wiki/vagrant-set-up) contains more in-depth instructions in case you have any issues you need to troubleshoot. Also feel free to message in #firmware for asssistance!

# Mac OS Set Up

### Installation

1. Clone the repository from GitHub, run the following command in terminal

```
  git clone git@github.com:UWaterloo-Formula-Electric/firmware.git
```

2. Install home-brew http://brew.sh
3. Install openOCD, in terminal run the following commands (you may have to install missing packages after)

```
brew install open-ocd
brew info open-cd
```

4. install [the compiler toolchain](https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-9-2020-q2-update-mac.tar.bz2?revision=d0d318de-b746-489f-98b0-9d89648ce910&hash=DB1DA90A2BC0E5A0D3FA92D4E7D2E9A6F4A2118D)
5. Unzip the package you just installed and put it in the directory /Applications/ARM (you may need to create this directory)
6. in the file /etc/paths add the directory /Applications/ARM/arm-none-eabi-gcc/bin. ([see the original instructions](https://gist.github.com/disposedtrolley/06d37e1db82b80ccf8c5d801eaa29373))
7. Try to build the firmware, run `make all`

### Building the Firmware

When running `make` it looks for the python binary, which is pointed to Python 2.7 on the VM. However, you probably want to use Python 3.8. If Python 3.8 isn't installed on your machine, install it by running: `brew install python3.8`

Then, link the python binary to Python 3.8 by running: `sudo ln -s -f /usr/local/bin/python3.8 /usr/local/bin/python`

Make sure you have the right version of cantools. If you run into issues regarding this, you can try running the following commands

```
pip3 uninstall cantools
pip3 install cantools==35.0.0
```

Make sure you are in the same directory as the requirements.txt file (firmware/common) and run the command: `pip3 install -r requirements.txt`

If you are able to successfully run `make all` within the /firmware repository, you have been successful! Feel free to reach out on Slack if you need assistance.

# Linux Set Up

### Installation

1. Install OpenOCD through [this link](https://sourceforge.net/projects/openocd/files/openocd/0.10.0/) or try the following command `sudo apt-get install openocd`
2. Download the GNU ARM Embedded Toolchain (version:9-2019-q4-major) from [here](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads)
3. Once installed, follow the following commands to ensure you can access the toolchain

```
# Peep your current PATH
echo $PATH
# /usr/local/heroku/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:<BLA BLA BLA>
# Change your PATH by appending the "bin" folder of the folder
# NOTE: You should also add this line to ~/.bashrc or ~/.bash_profile (in your home directory)
# so you don't have to remember to do this every time you want to compile firmware.
export PATH="$PATH:/usr/local/gcc_arm/gcc-arm-none-eabi-7-2017-q4-major/bin/" # Check to ensure the gcc-arm firmw
arm-none-eabi-gcc --version
# arm-none-eabi-gcc (GNU Tools for Arm Embedded Processors 9-2019-q4-major) 9.2.1 20191025 (release) [ARM/arm-9-b
# Copyright (C) 2019 Free Software Foundation, Inc.
# This is free software; see the source for copying conditions. There is NO
# warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

5. Config your git with the following commands

```
git config --global core.autocrlf input
git config --global user.email "your_email"
git config --global user.name "your_name"
```

6. Clone the firmware repo using the following command: `git clone git@github.com:UWaterloo-Formula-Electric/firmware.git`
7. Make sure you are in the same directory as the requirements.txt (firmware/common), and run `pip install -r requirements.txt`
8. Run `make all` within the firmware repo to try and build the code :)

# Other Resources

1. Git tutorial: https://www.freecodecamp.org/news/what-is-git-learn-git-version-control/
2. CAN Bus tutorial: https://www.kvaser.com/can-protocol-tutorial/
3. Learning C, plenty of resources online
4. STM32CubeMX (may be useful) installation: https://www.st.com/en/development-tools/stm32cubemx.html

# FAQ

(will be updated over time, feel free to open a PR)
