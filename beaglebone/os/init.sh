#!/bin/sh

# Initialize the 2020_beaglebone_os folder with a version of Buildroot
# specified by $BR_VER

BR_VER=2020.08
BR_FOLDER=buildroot-$BR_VER # desired relative path to Buildroot

git clone git://git.buildroot.net/buildroot $BR_FOLDER

if [ -d "$BR_FOLDER/.git/" ]
then
	if git -C $BR_FOLDER tag | grep -q $BR_VER
	then
		git -C $BR_FOLDER checkout $BR_VER
	else
		echo "Error: Specified BR_VER did not exist."
	fi
else
	echo "Error: Problem cloning Buildroot repo. Look above for errors."
        exit 1
fi

# After downloading the right version, apply some patches that are not
# yet merged to the stable branch we want to use

PATCHES=$(ls $(pwd)/board/wfe/beaglebone/patches/buildroot/*.patch)

for PATCH in $PATCHES
do
    echo "Processing $PATCH... "
    git -C $BR_FOLDER am "$PATCH"
    STATUS=$?
    if [ ! $STATUS ]
    then
        echo "Failed."
        exit 1
    fi

done

echo "Patching $BR_FOLDER succeeded."


# Load the out-of-tree build configuration and the default defconfig
# we wish to use

make -C $BR_FOLDER BR2_EXTERNAL=.. O=$(pwd) beaglebone_wfe_defconfig

