#!/usr/bin/env bash

set -euo pipefail

if [[ -d xania ]]; then
  git -C xania pull
else
  git clone https://github.com/mattgodbolt/xania
fi
crontab <xania/infra/scripts/crontab.xania
mkdir -p ~/releases ~/data/player ~/data/gods ~/data/log

aws s3 sync s3://mud.xania.org/player/ ~/data/player/

# workaround some sudo nonsense (maybe should go in bashrc?)
export XDG_RUNTIME_DIR=/run/user/$(id -u)

mkdir -p .config/systemd/user/
for service in doorman xania; do
  ln -s "$HOME/xania/infra/scripts/init/$service.service" .config/systemd/user/
  systemctl --user enable $service
done

systemctl --user daemon-reload
