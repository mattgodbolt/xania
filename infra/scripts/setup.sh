#!/usr/bin/env bash

set -euo pipefail

hostname mud.xania.org
apt-get update
apt-get upgrade -y
apt-get install -y awscli git cmake ninja-build gcc g++ cronic
apt-get autoremove -y

useradd -m -s /bin/bash xania
sudo --login -u xania $(pwd)/setup-xania.sh

pushd /root
git clone https://github.com/mattgodbolt/xania
crontab </root/xania/infra/scripts/crontab.root
popd

cp ./init/*.service /lib/systemd/system/
systemctl daemon-reload
#systemctl enable compiler-explorer
#systemctl enable doorman -- for the "upgrade" command

./sync-users.sh
