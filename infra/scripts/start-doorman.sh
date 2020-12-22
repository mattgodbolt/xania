#!/usr/bin/env bash

set -euo pipefail

ulimit -c unlimited

~/releases/current/bin/doorman
