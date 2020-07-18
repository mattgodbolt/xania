#!/usr/bin/env bash

set -euo pipefail

apt-get update
apt-get install -y awscli git cmake ninja-build gcc g++ cronic

adduser --system --disabled-password xania
sudo --login -u xania \
  "git clone https://github.com/mattgodbolt/xania " \
  "&& crontab < $HOME/xania/infra/crontab.xania"

pushd /root
git clone https://github.com/mattgodbolt/xania
crontab </root/xania/infra/crontab.root
popd

./sync-users.sh
