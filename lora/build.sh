#!/bin/bash
if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi
cmd='cmake-js build'
$cmd

cmd='node ../web/server.js'
$cmd
