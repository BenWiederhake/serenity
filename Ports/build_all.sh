#!/usr/bin/env bash

set -e

clean=false
verbose=false
all_files=$(ls)

for arg in "$@" ; do
    case "$arg" in
        clean)
            clean=true
            ;;
        verbose)
            verbose=true
            ;;
        *)
            ;;
        randomize)
            # shellcheck disable=SC2012
            all_files=$(ls | shuf)
            ;;
        *)
            echo "USAGE: $0 {clean | randomize}*"
            exit 1
            ;;
    esac
done

some_failed=false

for file in $all_files; do
    if [ -d $file ]; then
        pushd $file > /dev/null
            dirname=$(basename $file)
            if [ "$clean" == true ]; then
                if [ "$verbose" == true ]; then
                    ./package.sh clean_all
                else
                    ./package.sh clean_all > /dev/null 2>&1
                fi
            fi
            if [ "$verbose" == true ]; then
                if ./package.sh; then
                    echo "Built ${dirname}."
                else
                    echo "ERROR: Build of ${dirname} was not successful!"
                    some_failed=true
                fi
            else
                if ./package.sh > /dev/null 2>&1; then
                    echo "Built ${dirname}."
                else
                    echo "ERROR: Build of ${dirname} was not successful!"
                    some_failed=true
                fi
            fi
        popd > /dev/null
    fi
done

if [ "$some_failed" == false ]; then
    exit 0
else
    exit 1
fi
