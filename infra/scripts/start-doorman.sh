#!/usr/bin/env bash

set -euo pipefail

ulimit -c unlimited
cd ~/releases/current/area
~/releases/current/bin/doorman
