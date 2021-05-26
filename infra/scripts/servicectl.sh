#!/bin/bash

set -euo pipefail

env XDG_RUNTIME_DIR=/run/user/$(id -u) systemctl --user "$@"
