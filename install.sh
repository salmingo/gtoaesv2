#!/bin/sh

PROGRAM_NAME=gtoaesv2
sudo cp ${PROGRAM_NAME} /usr/local/bin
sudo cp ${PROGRAM_NAME}.xml /usr/local/etc
sudo cp ${PROGRAM_NAME}.service /lib/systemd/system
sudo systemctl enable ${PROGRAM_NAME}
sudo systemctl start ${PROGRAM_NAME}
