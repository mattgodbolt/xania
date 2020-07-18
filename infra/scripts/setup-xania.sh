#!/usr/bin/env bash

set -euo pipefail

if [[ -d xania ]]; then
  git -C xania pull
else
  git clone https://github.com/mattgodbolt/xania
fi
crontab <xania/infra/scripts/crontab.xania
mkdir -p releases data/player data/gods data/log

aws s3 sync s3://mud.xania.org/player/ data/player/
