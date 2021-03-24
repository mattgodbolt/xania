#!/usr/bin/env bash
cd "${0%/*}"
echo "Starting up doorman...."
. mud-settings-dev.sh
install/bin/doorman > log/doorman.log 2>&1 &
sleep 1
echo "Starting up xania...."
install/bin/xania > log/xania.log 2>&1 &
sleep 5
echo "All being well, telnet localhost $MUD_PORT to log in"
