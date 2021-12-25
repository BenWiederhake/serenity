#!/usr/bin/env bash

set -e

clean=false
verbose=false
ci_output=false
all_files=$(ls)

for arg in "$@" ; do
    case "$arg" in
        ci-output)
            ci_output=true
            ;;
        clean)
            clean=true
            ;;
        verbose)
            verbose=true
            ;;
        randomize)
            all_files=$(ls | shuf)
            ;;
        *)
            echo "USAGE: $0 {ci-output | clean | randomize | verbose}*"
            exit 1
            ;;
    esac
done

failed_ports=()
built_ports=""

if [ "$ci_output" = true ]; then
    echo "::group::Build order"
    echo "$all_files"
    echo "::endgroup::"
fi

for file in $all_files; do
    if [ -d $file ]; then
        if [ "$ci_output" = true ]; then
            echo "::group::Build package $file"
        fi
        pushd $file > /dev/null
            port=$(basename $file)
            port_built=0
            for built_port in $built_ports; do
                if [ "$built_port" = "$port" ]; then
                    port_built=1
                    break
                fi
            done
            if [ $port_built -eq 1 ]; then
                echo "Built $port."
                popd > /dev/null
                if [ "$ci_output" = true ]; then
                    echo "::endgroup::"
                fi
                continue
            fi
            if ! [ -f package.sh ]; then
                echo "ERROR: Skipping $port because its package.sh script is missing."
                popd > /dev/null
                if [ "$ci_output" = true ]; then
                    echo "::endgroup::"
                fi
                continue
            fi

            if [ "$clean" == true ]; then
                if [ "$verbose" == true ]; then
                    ./package.sh clean_all
                else
                    ./package.sh clean_all > /dev/null 2>&1
                fi
            fi
            if [ "$verbose" == true ]; then
                if ./package.sh; then
                    echo "Built ${port}."
                else
                    echo "ERROR: Build of ${port} was not successful!"
                    failed_ports+=("${port}")
                    popd > /dev/null
                    if [ "$ci_output" = true ]; then
                        echo "::endgroup::"
                    fi
                    continue
                fi
            else
                if ./package.sh > /dev/null 2>&1; then
                    echo "Built ${port}."
                else
                    echo "ERROR: Build of ${port} was not successful!"
                    failed_ports+=("${port}")
                    popd > /dev/null
                if [ "$ci_output" = true ]; then
                    echo "::endgroup::"
                fi
                    continue
                fi
            fi

            built_ports="$built_ports $port $(./package.sh showproperty depends) "
        popd > /dev/null
        if [ "$ci_output" = true ]; then
            echo "::endgroup::"
        fi
    fi
done

if (( ${#failed_ports[@]} )); then
    echo "Failed ports:"  "${failed_ports[@]}"
    exit 1
else
    echo "Success!"
    exit 0
fi
