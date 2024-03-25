#!/usr/bin/env bash

set -euo pipefail

hostname mud.xania.org
echo mud.xania.org > /etc/hostname

# PPA currently hosting g++-11
add-apt-repository ppa:ubuntu-toolchain-r/test

apt-get update
apt-get upgrade -y
apt-get install -y awscli git cmake ninja-build gcc g++ gcc-11 g++11 curl cronic jq ssmtp mailutils
apt-get autoremove -y

useradd -m -s /bin/bash xania
chfn -f 'Xania MUD' xania
sudo --login -u xania "$(pwd)/setup-xania.sh"
loginctl enable-linger xania

# Configure email
SMTP_USER=$(aws ssm get-parameter --name smtp_user | jq -r .Parameter.Value)
SMTP_PASS=$(aws ssm get-parameter --name smtp_password | jq -r .Parameter.Value)
cat >/etc/ssmtp/ssmtp.conf <<EOF
root=postmaster
mailhub=email-smtp.us-east-2.amazonaws.com:587
hostname=xania.org
FromLineOverride=NO
AuthUser=${SMTP_USER}
AuthPass=${SMTP_PASS}
UseSTARTTLS=YES
EOF
cat >/etc/ssmtp/revaliases <<EOF
EOF

chmod 640 /etc/ssmtp/*

pushd /root
git clone https://github.com/mattgodbolt/xania
crontab </root/xania/infra/scripts/crontab.root
popd

cp xania.logrotate /etc/logrotate.d/

./sync-users.sh
