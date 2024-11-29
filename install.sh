#!/bin/sh
if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi
set -e
/bin/apt update
/bin/apt install pigpio nodejs mosquitto libpaho-mqttpp-dev libpaho-mqtt-dev npm -y
/bin/npm install forever -g

PATH_=$(pwd)"/web"
/bin/cp lora2mqtt lora2mqtt.temp
/bin/sed -i -e "s@SERVER_DIR@$PATH_@g" lora2mqtt.temp
/bin/chmod +x lora2mqtt.temp
/bin/mv lora2mqtt.temp /etc/init.d/lora2mqtt
/bin/systemctl daemon-reload
/usr/sbin/update-rc.d lora2mqtt defaults

echo ""
echo "Install complete! Please reboot the system."