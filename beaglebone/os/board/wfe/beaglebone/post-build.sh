#!/bin/sh
BOARD_DIR="$(dirname $0)"
APP_DIR="${BOARD_DIR}/../../../../app/"
HOME_DIR="${TARGET_DIR}/home/wfe/"
DATA_DIR="${TARGET_DIR}/home/wfe/data"
DBC_FILE="${APP_DIR}/../../common/Data/2024CAR.dbc"

cp $BOARD_DIR/uEnv.txt $BINARIES_DIR/uEnv.txt

# Push beaglebone/app to /home/wfe/app on the beaglebone
mkdir -p $HOME_DIR
mkdir -p $DATA_DIR
cp -R $APP_DIR/ $HOME_DIR/
cp $DBC_FILE $DATA_DIR/
