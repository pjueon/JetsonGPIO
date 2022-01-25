#!/usr/bin/env bash

echo "Creating group gpio..."
groupadd -f -r gpio
LOGNAME=$(logname)
echo "Adding $LOGNAME to gpio..."
usermod -a -G gpio $LOGNAME
echo "Reloading udev rules..."
udevadm control --reload-rules
udevadm trigger
echo "Done"
