# UWFE Firmware

This is the firmware monorepo for the UW Formula Electric team. This repository contains vehicle/HIL board firmware, cell tester firmware, and our firmware validation testbed.

# Onboarding Instructions

- [Notion Access](#notion)
- [SSH Key](#ssh-key-set-up)
- [Git Configuration](#git-configuration)
- [Development Setup](#dev-environment-macos--windowswsl2--linux)
- [Other Resources](#other-resources)
- [FAQ](#FAQ)

# Notion

We use Notion for task management and documentation. Steps to get set up:

1. Create a Notion account with your school email
2. Go to UWFE's Discord and in the #firmware channel, ask to be invited to the Notion workspace and drop your school email.

# SSH Key Set Up

SSH key is used for access credentials (think of it as your username and password). Without it, you can't contribute to the repository.

1. Create a SSH key by following the instructions [here](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent)
2. Add the SSH key to your GitHub account by following this [page](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent)

Note: You can skip this step if you already have a SSH key configured on GitHub

## Installing Git
__For Windows only__: By default, Git is not installed on Windows machines. Download Git from [here](https://git-scm.com/downloads/win). When going through the installation, use default options.

# Git Configuration
Regardless of what operating system you use, please run the following commands in a terminal (remember to replace the arguments with your name + email). If you are using a Windows machine, you need to install [Git first](#installing-git)
```
git config --global user.name "Your Name"
git config --global user.email "your_email@example.com"
git config --global push.autoSetupRemote true
git config --global core.autocrlf false
```
# Dev environment (macOS · Windows/WSL2 · Linux)
We ship a Dev Container so everyone uses the same toolchain (ARM GCC, OpenOCD, Python deps) without installing things natively. The default workflow is:
- Build & flash inside the container.
- __Optional__: To debug, run OpenOCD on the host (USB stays on host), and let the container attach to it via host.docker.internal:3333 (OpenOCD’s default GDB port). 

## Prerequisites
- Docker Desktop (Mac/Win) or Docker Engine (Linux). Click on your operating system -> [Windows](https://desktop.docker.com/win/main/amd64/Docker%20Desktop%20Installer.exe?utm_source=docker&utm_medium=webreferral&utm_campaign=docs-driven-download-win-amd64) / [Mac with Apple Silicon](https://desktop.docker.com/mac/main/arm64/Docker.dmg?utm_source=docker&utm_medium=webreferral&utm_campaign=docs-driven-download-mac-arm64) / [Mac on x86](https://desktop.docker.com/mac/main/arm64/Docker.dmg?utm_source=docker&utm_medium=webreferral&utm_campaign=docs-driven-download-mac-arm64) / [Ubuntu](https://docs.docker.com/engine/install/ubuntu/)
    - Docker Desktop exposes the special hostname host.docker.internal for reaching host services from containers. On plain Linux engines, we add an equivalent mapping automatically. 
    - After installing, run __Docker Desktop__ to finish the setup process.
- VS Code (VSC) + [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers).
    - To start the container environment in VSC, `Ctrl + Shift + P` or `Cmd + Shift + P` to open the command palette and type in `Open Folder in Container`
- [Git](#installing-git)
-  Flashing tool on the host (i.e., not in the container).
    - macOS: `brew install open-ocd` (run the command in a terminal)
    - Linux: your distro’s openocd package
    - Windows: [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html#overview)
- __IMPORTANT__: If you are using Windows, there are additional steps required. Please [read here](#windows-setup). Then return to [Quick Start](#quick-start).

Tuning WSL2 (Windows): if builds feel RAM-starved, adjust `%UserProfile%\.wslconfig` and wsl --shutdown. (Optional.). You will need to create `.wslconfig` if it doesn't exist. See an example of `.wslconfig` below:
```
[wsl2]
memory=4GB          # RAM for the WSL2 VM
processors=6        # logical CPUs for the VM
swap=8GB            # swap size (0 disables)
```
## Quick start

1. Clone the repo to your local machine.
- Windows/macOS/Linux: anywhere on your machine is fine so pick your preference.
  In a terminal, navigate to a directory where you want to clone the firmware repository (e.g., `/c/Users/Jacky/UWFE` is my setup on a Windows machine, `/Users/jacky/UWFE/` is my setup on a Macbook). Then, run the following command:
  ```
  git clone git@github.com:UWaterloo-Formula-Electric/firmware.git
  ```
2. In Visual Studio Code (VSC), open the firmware repository (click on `Open Folder` or `Ctrl + K` and `Ctrl + O`).
3. In VSC, open the Dev Container. To do this: `Ctrl + Shift + P` or `Cmd + Shift + P` and type `Dev Containers: Open Folder in Container`
    - This builds the image once, installs the toolchain & Python deps, and opens a shell in the container. 
    - Note: First time building the container will take a long time (~5-8 minutes) but subsequent builds will be significantly faster (~15 seconds)
3. Open a terminal inside VSC Dev Container. Then build the code by running the following command:
    ```
    # Format
    make {board}

    # Example: build the binary for VCU
    make vcu

    # Example: build the binary for all the boards
    make all

    # Example: Erase the binary/build directory (recommended after modifying code)
    make clean
    ```
4. To flash a board, you will need a ST-Link connected to your computer + the board you're flashing: 
    ```
    # Command format (only for Mac + Linux)
    make load LOAD_TARGET={board}

    # Example: Flashing the BMU (only for Mac + Linux)
    make load LOAD_TARGET=bmu

    # Example for Macbook & Linux:
    1. Open a terminal and navigate to your firmware directory (not using the container here)
    2. make load LOAD_TARGET={board}

    # Example for Windows:
    1. Open STM32CubeProgrammer
    2. On the left sidebar, click on the download icon (2nd from the top) to open up the 'Erasing & Programming Section'
    3. Click on 'Browse', navigate to your firmware directory, and select the elf. file for the desired board
       e.g., firmware/Bin/bmu/Release/bmu.elf
    4. Click 'Connect' on the top right
    5. Click 'Start Programming'
    ```
5. To push your changes, please review [Git Workflow](#git-workflow).
6. __Optional__: To debug the target, start OpenOCD on the host (USB connected to host)
    ```
    # example target; adjust your target cfg (f4x, f7x, etc.)
    openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
    ```
    OpenOCD’s __GDB server = :3333__ (telnet = :4444). 

7. __Optional__: Debug from the container
    - In the container, point GDB (or VS Code Cortex-Debug) at: `host.docker.internal:3333`. Docker Desktop resolves this hostname to the host; on Linux engines we inject an equivalent mapping. 

### __Optional:__ VS Code debug sample (`.vscode/launch.json`)
```
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Attach to OpenOCD@host",
      "type": "cortex-debug",
      "request": "launch",
      "servertype": "external",
      "gdbTarget": "host.docker.internal:3333",
      "executable": "${workspaceFolder}/build/firmware.elf"
    }
  ]
}
```
### __Optional:__ USB inside the container (Linux / Windows-WSL)
If you prefer to run OpenOCD in the container:
- Linux: add a device map when starting the container:
--device=/dev/bus/usb:/dev/bus/usb (we keep this out of the shared config so macOS still works).

- Windows-WSL: first attach your ST-LINK to WSL with usbipd:
```
usbipd wsl list
usbipd wsl attach --busid <BUSID>
```
While attached to WSL, the device is not available to Windows; detach to return it. 

Then, inside the container:
```
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
# and debug with servertype:"openocd" in Cortex-Debug
```
## Example Workflow
1. Create a new branch for your task
    - Branch Name Format: `{year}_{baseBranch}_{taskName}`
    - Branch Name Example: `2025_main_fix_IMD_value`
    - Command: `git checkout -b 2025_main_fix_IMD_value`
2. Make changes in code for the bmu
3. In the container, ensure it can compile by running `make bmu`
4. Upon successful compilation, flash it onto the BMU
    - Windows: Use STM32CubeProgrammer to flash the __.elf__ file onto the board
    - Mac/Linux: Open a terminal outside of the container environment and run `make load LOAD_TARGET=bmu`
5. The changes has been verified on the board and I have reviewed my changes thoroughly. Now I am ready to push my changes to Git
    - `git add {modifiedFiles}`
    - `git commit -m "message summarizing the changes I made"`
    - `git push`
6. Once I am confident with my work, create a pull request (PR) and notify one of the firmware leads

## Notes & troubleshooting

- “How does the container reach my host?”
Use host.docker.internal. On Docker Desktop this is built-in; on Linux engines we map it to the host gateway so it works the same. (OpenOCD listens on :3333 by default.) 

- Git “safe.directory” warning: our container sets a system-wide trust for /workspaces/* on first run. If you still see it, run:
git config --global --add safe.directory $(pwd)

### Useful WSL Commands to Know (Windows only)
The following commands only work inside WSL.
- `exit`  -> to quit WSL environment

### Useful Container Commands to Know
The following commands are expected to be used in VSC, command pallete (*Ctrl + Shift + P*).
- `Dev Containers: Reopen Folder Locally` -> to quit the container

## Windows Setup
### Prerequisites
- Open Powershell and run `wsl --install --distribution Ubuntu-24.04`

1. Launch Docker Desktop and do the following steps:
    - Go to `Settings -> General` and ensure "Use the WSL 2 based engine" is ticked
    - Go to `Settings -> Resources -> WSL Integration` and ensure "Enable integration with my default WSL distro" is ticked
2. Verify you can launch WSL and run Docker inside it
    - Open a terminal and run `wsl`
    - `docker --version` -> should return a version (e.g., `Docker version 26.1.1, build 4cf5afa`)
    - `docker run hello-world` -> if you see `Hello from Docker!`, everything is good so far!
3. Now, follow the instructions in __[Quick Start](#quick-start)__.

# Git Workflow
[Git Tutorial](https://www.freecodecamp.org/news/what-is-git-learn-git-version-control/)

__Note__: On Windows system, if you are running git commands in the host (i.e., not the container), you will need to run `git status` inside the container to pull any git changes (e.g., making a commit, pushing, etc).

# __Everything below is deprecated!!!__

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