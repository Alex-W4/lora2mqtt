# lora2mqtt
Intigrate and configure LoRa Devices. Currently, this Project contains a backend driver for the SX1278 Chip
on a specific custom Module. Together with a simple but functional web frontend this project provides a complete
configutaion suite for compatible devices.

## Installation
This software is meant to run on a Raspberry Pi 3 or newer with GPIO extention pins. The LoRa receiver module must
be attached in the shown orientation and position. Please power down the pi when attaching or detaching the module 
to prevent damage to the module or the pi.

![LoRa module position image](img/pi_position.png)

This Software has dependencies which can be installed by the following command:

```bash
sudo apt install pigpio nodejs mosquitto libpaho-mqttpp-dev libpaho-mqtt-dev npm -y
sudo -i npm install forever -g
```

To download and install this software you can clone this repo into a directory of your choice and run install.sh:

```bash
git clone https://github.com/Alex-W4/lora2mqtt.git
cd lora2mqtt
sudo bash install.sh
```

Reboot the pi to complete the installation.

## Usage
A Webserver is hosted under the ip of the raspberry pi and port `80` by default. The port can be changed in `/etc/lora2mqtt/config.json`.


## TODO
There are still some features missing and planed in future updates like:
* Documentation
* English language support
* Settings menu
* User authentication
* Further interface documentation
* Support for more modules