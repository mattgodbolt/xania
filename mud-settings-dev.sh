# During development if you intend to launch either xania or doorman 
# from a shell, source this file so that the shell inherits some
# reasonable config settings for the processes and run the processes from 
# the project's root directory e.g. after running the build:
#   $ cd ~/xania
#   $ . mud-settings-dev.sh 
#   $ ./install/bin/doorman
# You can also use 'make start', see README.md.
export MUD_PORT=9000
export MUD_AREA_DIR=area
export MUD_HTML_DIR=html
export MUD_DATA_DIR=.
