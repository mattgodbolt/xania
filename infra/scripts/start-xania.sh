#!/usr/bin/env bash

set -euo pipefail

ulimit -c unlimited

export MUD_DATA_DIR=~/data
export MUD_AREA_DIR=~/releases/current/area
export MUD_HTML_DIR=~/releases/current/html

~/releases/current/bin/xania
