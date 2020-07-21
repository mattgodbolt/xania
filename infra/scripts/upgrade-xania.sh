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

rm -f ~/releases/current
ln -s ~/releases/xania-$VERSION ~/releases/current

echo "Doorman and xania updated"
echo "To restart doorman, you'll need to run (as your own user):"
echo "  $ sudo service doorman restart"
echo "To restart the MUD, best to do so from within the MUD itself"