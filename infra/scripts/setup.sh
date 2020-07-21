#!/usr/bin/env bash

set -euo pipefail

hostname mud.xania.org
echo mud.xania.org > /etc/hostname
apt-get update
apt-get upgrade -y
apt-get install -y awscli git cmake ninja-build gcc g++ cronic jq ssmtp mailutils
apt-get autoremove -y

useradd -m -s /bin/bash xania
chfn -f 'Xania MUD' xania
sudo --login -u xania "$(pwd)/setup-xania.sh"
loginctl enable-linger xania

# Configure email
SMTP_PASS=$(aws ssm get-parameter --name /admin/smtp_pass | jq -r .Parameter.Value)
cat >/etc/ssmtp/ssmtp.conf <<EOF
root=postmaster
mailhub=email-smtp.us-east-1.amazonaws.com
hostname=compiler-explorer.com
FromLineOverride=NO
AuthUser=AKIAJZWPG4D3SSK45LJA
AuthPass=${SMTP_PASS}
UseTLS=YES
UseSTARTTLS=YES
EOF
cat >/etc/ssmtp/revaliases <<EOF
xania:mud@xania.org:email-smtp.us-east-1.amazonaws.com
EOF

chmod 640 /etc/ssmtp/*

pushd /root
git clone https://github.com/mattgodbolt/xania
crontab </root/xania/infra/scripts/crontab.root
popd

cp xania.logrotate /etc/logrotate.d/

./sync-users.sh
