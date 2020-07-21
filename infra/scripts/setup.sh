#!/usr/bin/env bash

set -euo pipefail

hostname mud.xania.org
echo mud.xania.org > /etc/hostname
apt-get update
apt-get upgrade -y
apt-get install -y awscli git cmake ninja-build gcc g++ cronic
apt-get autoremove -y

useradd -m -s /bin/bash xania
sudo --login -u xania "$(pwd)/setup-xania.sh"
loginctl enable-linger xania

pushd /root
git clone https://github.com/mattgodbolt/xania
crontab </root/xania/infra/scripts/crontab.root
popd

cp xania.logrotate /etc/logrotate.d/

./sync-users.sh
