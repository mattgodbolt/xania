# Common setup needed by the pfile upgrade scripts.
set -euo pipefail

if [ $# != 1 ]; then
    echo "Usage: $0 [release number]"
    exit 1
fi
if [[ $LOGNAME == "xania" ]]; then
    . ~/xania/infra/scripts/mud-settings-prod.sh
    PATH=~/releases/current/bin:$PATH
else
    . mud-settings-dev.sh
    PATH=`pwd`/install/bin:$PATH
fi
snapshot_base=~/tmp
snapshot_name=pfiles-$1
snapshot_dir=$snapshot_base/$snapshot_name
snapshot_archive_path=$snapshot_dir.tar.xz
