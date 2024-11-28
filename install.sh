#!/bin/sh
PATH=$(pwd)
sed -i -e 's/SERVER_DIR/$PATH/g' lora2mqtt
sudo cp lora2mqtt /etc/init.d/lora2mqtt