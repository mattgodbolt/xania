#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -ne 1 ]; then
  echo "use install-xania.sh <version>"
  exit 1
fi

VERSION=$1

mkdir -p ~/releases
cd ~/releases
if [[ -d xania-$VERSION ]]; then
  echo "Xania version $VERSION already exists!"
  exit 1
fi

curl -sL "$(aws s3 presign s3://mud.xania.org/releases/xania-$VERSION.tar.xz)" | tar Jxf -

cd "xania-$VERSION"

for dir in "$HOME/data/"*; do
  ln -sf "$dir" .
done
