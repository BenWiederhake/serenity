#!/usr/bin/env bash

set -e

# We cannot handle port with spaces/newlines/tabs in the directory name,
# hence SC2012 is a false-positive.

clean=false
all_files=$(ls)
# TODO: Support '$0 clean randomize'
case "$1" in
    clean)
        clean=true
        ;;
    randomize)
        # shellcheck disable=SC2012
        all_files=$(ls | shuf)
        ;;
    "")
        ;;
    *)
        echo "USAGE: $0 [clean | randomize]"
        exit 1
        ;;
esac

some_failed=false

for file in $all_files; do
    if [ -d "$file" ]; then
        pushd "$file" > /dev/null
            dirname=$(basename "$file")
            if [ "$clean" == true ]; then
                ./package.sh clean_all > /dev/null 2>&1
            fi
            if ./package.sh > /dev/null 2>&1 ; then
                echo "Built ${dirname}."
            else
                echo "ERROR: Build of ${dirname} was not successful!"
                some_failed=true
            fi
        popd > /dev/null
    fi
done

if [ "$some_failed" == false ]; then
    exit 0
else
    exit 1
fi
