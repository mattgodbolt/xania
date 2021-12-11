#!/usr/bin/env bash
# This script checks for symbolic links that we expect to be present in the source tree. 
function check_symlink {
    if [ ! -h $1 ]; then
        echo "$1 is not a symlink, expected it to point to: $2"
        echo "This can happen if you download the code as a ZIP file instead of using 'git clone'."
        echo "Either 'git clone' it or manually fix the symlink."
        exit 1
    fi

}
check_symlink "${PROJ_ROOT}/area/version.are" "${PROJ_ROOT}/area/version.are.in"
check_symlink "${PROJ_ROOT}/src/test/test_data/area" "${PROJ_ROOT}/area"

