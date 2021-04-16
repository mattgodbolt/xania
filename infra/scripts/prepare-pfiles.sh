#!/usr/bin/env bash
# This script is run on the mud host as part of a player file upgrade workflow.
# It creates an archive of the player and gods directories. The archive can be
# used to restore the original pfiles if the upgrades don't work out.
# It adds the copied files to a temp git repo then runs `pfu` to perform upgrades on the 
# player files. You can then review the git diffs before carrying the upgrades
# back into the production player directory using upgrade-pfiles.sh
# In dev, run this from the project root directory.
. "${0%/*}/pfile-upgrade-lib.sh"
orig_dir=`pwd`
cd $MUD_DATA_DIR
if [ -d $snapshot_dir ]; then
    echo "Snapshot already exists, replacing it."
    rm -rf $snapshot_dir
fi
mkdir -p $snapshot_dir
echo "Snapshotting player files into $snapshot_dir"
cp -r player gods $snapshot_dir
echo "Creating backup tarball of this snapshot: $snapshot_archive_path"
cd $snapshot_base
tar Jcf $snapshot_archive_path $snapshot_name
cd $snapshot_name
echo "Initializing git repo for player file upgrade"
git init -q
git add .
git commit -qm 'importing pfiles'
cd $orig_dir
echo "Running pfu to upgrade the player files in $snapshot_dir"
# As pfu uses the same configuration as the mud, point MUD_DATA_DIR
# at the snapshot directory so that it processes the copy of the player files.
MUD_DATA_DIR=$snapshot_dir pfu
echo "Review the git diff output in $snapshot_dir then run upgrade-pfiles.sh"