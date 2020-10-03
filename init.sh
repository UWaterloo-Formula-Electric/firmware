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
	echo "Error: Did not clone Buildroot as a repo. Look above for errors."
fi

make -C $BR_FOLDER BR2_EXTERNAL=.. O=$(pwd) beaglebone_wfe_defconfig
