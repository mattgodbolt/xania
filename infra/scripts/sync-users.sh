#!/usr/bin/env bash

set -euo pipefail

aws s3 sync s3://mud.xania.org/ssh_users /tmp/ssh_users
for user_path in /tmp/ssh_users/*; do
  USERNAME=$(basename $user_path)
  if ! id "${USERNAME}" >/dev/null 2>&1; then
    echo "Adding user ${USERNAME}..."
    useradd -m "${USERNAME}"
    sudo --login -u "${USERNAME}" bash -c "mkdir -m 700 .ssh && cat ${user_path} > .ssh/authorized_keys"
    echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" >"/etc/sudoers.d/99-${USERNAME}"
  fi
done

rm -rf /tmp/ssh_users
