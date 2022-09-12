#!/usr/bin/env bash

set -eo pipefail

script_path=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
cd "$script_path/.."

# We absolutely need the power of PCRE here. Therefore, sort out platforms which don't support it:
# Use escape sequence for '-' to get around the commit linter.
if ! grep $'\x2D'P a <(echo a) >/dev/null 2>/dev/null; then
    echo "'grep' does not support '-P'. Skipping lint-missing-resources.sh"
    exit 0
fi

git ls-files '*.cpp' | xargs grep $'\x2D'Pho '(?<!file://)(?<!\.)(?<!})(?<!\()/(etc|res|usr)/[^'"'"'":\*\(\)\{\}, \t]+(?<!/)' | \
sort -u | \
while read -r referenced_resource
do
    if ! [ -r "Base/${referenced_resource}" ] && ! [ -r "Build/Root/${referenced_resource}" ]
    then
        echo "Potentially missing resource: ${referenced_resource}"
    fi
done
