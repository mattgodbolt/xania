#!/bin/csh
# Written by Furey.
# With additions from Tony and Alander.

# Set the port number.
set port = 9000
if ( "$1" != "" ) set port="$1"

# Change to area directory.
cd ../area

# Set limits.
if ( -e shutdown.txt ) rm -f shutdown.txt

while ( 1 )
    # If you want to have logs in a different directory,
    #   change the 'set logfile' line to reflect the directory name.
    set index = 1000
    while ( 1 )
        set logfile = ../log/`date +%a_%d_%b_%k.%M.log`
        if ( ! -e $logfile ) break
        sleep 60
    end

    # Run rom.
    nice -6 ../src/xania $port >&! $logfile


    # Restart, giving old connections a chance to die.
    if ( -e shutdown.txt ) then
        rm -f shutdown.txt
        exit 0
    endif
    sleep 15
end
