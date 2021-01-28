# 2018\_BMU

## Project Structure

```
.
├── Cube-F7-Src/                # Legacy autogen CubeMX HAL
├── Cube-F7-Src-respin/         # Current autogen CubeMX HAL
├── Src/                        # WFE board-specifc source code 
├── Inc/                        # WFE board-specifc source headers
├── common-all/                 # Source code shared among boards
├── Makefile                    # GNU Make build script
└── README.md                   # You are here 

```

## Downloading the Code to your System 

You can clone this repository using git by pressing the button above that says
"clone". This will give you a command that you can copy into you terminal to
copy this code.

An important step that is often forgotten is to download the common-all
submodule (a repo that contains code/data common to multiple different
boards). Do this by running the following two commands:

```
git submodule init
git submodule update
```

Alternative, you can tell git to download the submodules from the very
beginning:

```
git clone --recurse-submodule <git_repo_address>
```

## Building the Code 

We use the [GNU make](https://www.gnu.org/software/make/manual/make.html) program to build our code.
You can start a build by running `make all` or just `make`.

## Loading the Code to a Board

We've created a custom make target called `load` to run the commands for
loading the code to the board. The load target will also detect any changes
made to the source and rebuild them as appropriate.

Run the `load` target with the following command:

```
make load
```

You should see OpenOCD starting and attempting to load the code to the board.
Any errors encountered during the load process will be reported here.

