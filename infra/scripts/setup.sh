#!/usr/bin/env bash

set -euo pipefail

apt-get update
apt-get upgrade -y
apt-get install -y awscli git cmake ninja-build gcc g++ cronic
apt-get autoremove -y

useradd -m xania
sudo --login -u xania bash -c \
  'git clone https://github.com/mattgodbolt/xania ' \
  '&& crontab <$HOME/xania/infra/scripts/crontab.xania'

pushd /root
git clone https://github.com/mattgodbolt/xania
crontab </root/xania/infra/scripts/crontab.root
popd

./sync-users.sh
