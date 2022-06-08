#!/usr/bin/env bash

# In order to use the Jetson GPIO Library, the correct user permissions/groups must be set.
echo "[Setting user permissions]"
echo "Creating group gpio..."
groupadd -f -r gpio
LOGNAME=$(logname)
echo "Adding $LOGNAME to gpio..."
usermod -a -G gpio $LOGNAME
echo "Reloading udev rules..."
udevadm control --reload-rules
udevadm trigger
echo "Done"
