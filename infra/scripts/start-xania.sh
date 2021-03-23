#!/usr/bin/env bash
# Started by xania systemd service and assumes the pwd is this directory.

set -euo pipefail

ulimit -c unlimited

. mud-settings-prod.sh

~/releases/current/bin/xania
