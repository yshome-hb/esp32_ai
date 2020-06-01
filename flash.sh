#!/bin/bash
while getopts :mp: opt
do
    case $opt in
        m) make all;;
        p) port=$OPTARG ;;
    esac
done

if [ ! $port ]; then
    echo "please input a port"
    exit 1
fi

esptool.py --chip esp32 --port $port --baud 921600 --before default_reset --after hard_reset write_flash -z 0x1000 build/bootloader/bootloader.bin 0x10000 build/esp32_ai.bin 0x8000 build/partitions_ai.bin
