#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -ne 1 ]; then
  echo "use upgrade-xania.sh <version>"
  exit 1
fi

VERSION=$1

if [[ ! -d ~/releases/xania-$VERSION ]]; then
  echo "Version not installed"
  exit 1
fi

echo "TODO I really don't want sudo for xania?"
#service ... etc stop

#systemctl enable xania
#systemctl enable doorman

rm -f ~/releases/current
ln -s ~/releases/xania-$VERSION ~/releases/current

service start xania
service start doorman
