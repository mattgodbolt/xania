#!/usr/bin/env bash

set -euo pipefail

BACKUP_NAME=$(date '+%Y-%m-%d-%H%M%S.tar.xz')
BACKUP_FILENAME=/tmp/backup-$BACKUP_NAME

cd $HOME

tar Jcf "${BACKUP_FILENAME}" data
aws s3 cp "${BACKUP_FILENAME}" "s3://mud.xania.org/backups/${BACKUP_NAME}"
rm "${BACKUP_FILENAME}"