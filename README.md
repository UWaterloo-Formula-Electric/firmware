# UWFE Firmware

This is the firmware monorepo for the UW Formula Electric team. This repository contains vehicle/HIL board firmware, cell tester firmware, and our firmware validation testbed.

# Onboarding Instructions

- [Notion Access](#notion)
- [SSH Key](#ssh-key-set-up)
- [Git Configuration](#git-configuration)
- [Windows](#windows-setup)
- [Mac OS](#mac-os-set-up)
- [Linux](#linux-set-up)
- [Other Resources](#other-resources)
- [FAQ](#FAQ)

# Notion

We use Notion for task management and documentation. Steps to get set up:

1. Create a Notion account with your school email
2. Fill out this [form](https://forms.gle/Mz7rk66djoDZY9Fv9) and we will invite you to the team's Notion workspace

# SSH Key Set Up

SSH key is used for access credentials (think of it as your username and password). Without it, you can't contribute to the repository.

1. Create a SSH key by following the instructions [here](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent)
2. Add the SSH key to your GitHub account by following this [page](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent)

Note: You can skip this step if you already have a SSH key configured on GitHub

# Git Configuration
Regardless of what operating system you use, please run the following commands in a terminal (remember to replace the arguments with your name + email). If you are using a Windows machine, you need to install [Git first](#windows-setup)
```
git config --global user.name "Your Name"
git config --global user.email "your_email@example.com"
git config --global push.autoSetupRemote true
```

# Windows Setup
## Installing Git and Cloning the Firmware Repository
__For Windows only__: By default, Git is not installed on Windows machines. Download Git from [here](https://git-scm.com/downloads/win). When going through the installation, use default options.

Now, navigate to a directory where you want to clone the firmware repository (e.g., `/c/Users/Jacky/UWFE` is my setup on a Windows machine). Inside that directory, right-click and `Open Git Bash Here`. A Git terminal should be opened. Then, run the following command:
```
git clone git@github.com:UWaterloo-Formula-Electric/firmware.git
```
Now, you have cloned the firmware locally to your computer.

## Docker Container Setup for Windows Users
### Overview
Previously, the team used Vagrant to setup a Ubuntu virtual machine to do development but that is method was slow and non-trivial to setup & maintain. We have now transitioned to using a docker container. The process is to use WSL2 to build the Linux container.

### Prerequisites
- Open Powershell and run `wsl --install --distribution Ubuntu-24.04`
- In Visual Studio Code (VSC), install the `Dev Containers` extension: https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers
- In Powershell, run `winget install usbipd`. This is used to pass the ST-Link to WSL
- Install [Docker Desktop](https://desktop.docker.com/win/main/amd64/Docker%20Desktop%20Installer.exe?utm_source=docker&utm_medium=webreferral&utm_campaign=docs-driven-download-win-amd64) or [here](https://docs.docker.com/desktop/setup/install/windows-install/). Please use the 2nd hyperlink if you are using an ARM processor on your Windows machine.

### Setting Up the Container
1. Launch Docker Desktop and do the following steps:
    - Go to `Settings -> General` and ensure "Use the WSL 2 based engine" is ticked
    - Go to `Settings -> Resources -> WSL Integration` and ensure "Enable integration with my default WSL distro" is ticked
2. Verify you can launch WSL and run Docker inside it
    - Open a terminal and run `wsl`
    - `docker --version` -> should return a version (e.g., `Docker version 26.1.1, build 4cf5afa`)
    - `docker run hello-world` -> if you see `Hello from Docker!`, everything is good so far!
3. Open VSC and navigate to the firmware directory that you cloned from earlier. Open a terminal (Ctrl + Shift + `) and launch WSL.
4. Open up the command pallete by pressing *Ctrl + Shift + P* and type in `Dev Containers: Rebuild and Reopen in Container` and press *Enter*
    - The container should be building now (Can take up to 5 minutes for the first time) and if the operation was successful, you should see "Done. Press any key to close the terminal" in the terminal.
    - Click inside the terminal and press any key to close it.
    - You should now see something similar to `vscode ➜ /workspaces/firmware (main) $ ` in the terminal
5. Verify you can build the code by running `make all`
6. __IMPORTANT NOTE__: if you want to flash a board, you need to connect the ST-Link to your computer -> attach it to WSL -> then build the container.
    - To attach the ST-Link to WSL:
    ```
    usbipd list 
    usbipd attach --wsl --busid <BUSID>
    ```

### Useful WSL Commands to Know
The following commands only work inside WSL.
- `exit`  -> to quit WSL environment

### Useful Container Commands to Know
The following commands are expected to be used in VSC, command pallete (*Ctrl + Shift + P*).
- `Dev Containers: Reopen Folder Locally` -> to quit the container

# Mac OS Set Up

### Installation

1. Open a terminal and navigate to a local directory where you want to clone the repository from GitHub (e.g., `Users/jacky/UWFE` is my setup on a Mac). Then run the following command:

```
  git clone git@github.com:UWaterloo-Formula-Electric/firmware.git
```

2. Install home-brew http://brew.sh
3. Install openOCD by running the following commands in a terminal (you may have to install missing packages after):

```
brew install open-ocd
open-ocd
```

4. Download the compiler toolchain from here [Apple silicon](https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-darwin-arm64-arm-none-eabi.tar.xz) or here [x86 Macbook](https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-darwin-x86_64-arm-none-eabi.tar.xz)
5. Unzip the package you just downloaded and put it in the directory `/Applications/ARM` (you may need to create the `ARM` directory)
6. Rename the folder name in the directory `/Applications/ARM` to `arm-none-eabi-gcc`
7. in the file `/etc/paths` add the directory `/Applications/ARM/arm-none-eabi-gcc/bin`. ([see the original instructions](https://gist.github.com/disposedtrolley/06d37e1db82b80ccf8c5d801eaa29373))
  - Enter following command to open paths file in `/etc/paths`:
```
sudo nano /etc/paths
```

  - Add `/Applications/ARM/arm-none-eabi-gcc/bin` to the file
  - Control + X ->  Enter Y -> Hit Enter to save the file
  - Quit the terminal instance
8. Install the required Python packages:
```
python3 -m pip install --upgrade pip
python3 -m pip install -r common/requirements.txt
```
9. Try to build the firmware, run `make all`
   
### ARM Mac security issues
1. If you are getting "Bad CPU type in executable" please install the Mac emulator by running ```softwareupdate --install-rosetta```
2. There are some security issues with ARM Mac when you run `make all`
  - Some messages like this: “arm-none-eabi” cannot be opened because the developer cannot be verified
  - Please do **NOT** click delete the file -> click cancel
  - To fix this, follow the instructions found [here](https://support.apple.com/en-ca/guide/mac-help/mh40616/mac).
  - Continue the above steps until it doesn't show you the message again (there are around 10~15 executables)


### Building the Firmware

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
# /usr/local/heroku/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:<PLACEHOLDER>
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

4. Clone the firmware repo using the following command: `git clone git@github.com:UWaterloo-Formula-Electric/firmware.git`
5. Make sure you are in the same directory as the requirements.txt (firmware/common), and run `pip install -r requirements.txt`
6. Run `make all` within the firmware repo to try and build the code :)

# Other Resources

1. Git tutorial: https://www.freecodecamp.org/news/what-is-git-learn-git-version-control/
2. CAN Bus tutorial: https://www.kvaser.com/can-protocol-tutorial/
3. Learning C, plenty of resources online
4. STM32CubeMX (may be useful) installation: https://www.st.com/en/development-tools/stm32cubemx.html

# FAQ

(will be updated over time, feel free to open a PR)

# Deprecated Section
__Everything below is deprecated!!!__
# Vagrant Environment Set Up for Windows Users

### Overview

Vagrant is an open-source software product for building and maintaining portable virtual software development environments. We will be using it to create an Ubuntu VM for VirtualBox.
The repository we use is found [here](https://github.com/UWaterloo-Formula-Electric/vagrant).

### Prerequisites

- Install the latest version of [Vagrant](https://developer.hashicorp.com/vagrant/downloads)
- Install 7.0.18 version of [VirtualBox](https://www.virtualbox.org/wiki/Download_Old_Builds_7_0). DON'T install the latest version, there are compatibility issues with Vagrant.
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