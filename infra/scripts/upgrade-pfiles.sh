#!/usr/bin/env bash
# This script is run on the mud host as part of a player file upgrade workflow.
# It's used after prepare-pfiles.sh to accept the upgraded pfiles and copy
# them back into the mud's player dir.
. "${0%/*}/pfile-upgrade-lib.sh"

cd $MUD_DATA_DIR
cp --preserve=timestamps $snapshot_dir/player/* player
cp --preserve=timestamps $snapshot_dir/gods/* gods
