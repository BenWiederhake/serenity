#!/bin/sh

set -e

git grep -Prho 'Config::read_[^)]+\)' | \
sed -Ee 's%^Config::read_([^(]+)\(%\1, %' -e 's%\)$%%' | \
sort | \
awk -F ', ' '
BEGIN {
    saved_needs_printing = 0
}
{
    # printf("-- Now handling: %s\n", $0)
    if ($0 != saved_0) {
        if ($1 == saved_1 && $2 == saved_2 && $3 == saved_3 && $4 == saved_4) {
            # We definitely have a problem.
            if (saved_needs_printing == 1) {
                saved_needs_printing = 0
                printf("%s\n", saved_0)
                # printf("-- detected conflict to %s\n", saved_0)
            }
            printf("%s\n", $0)
            # printf("-- conflict run\n")
        } else {
            # There is no problem yet, but it may still be part of a conflict.
            saved_needs_printing = 1;
            # printf("-- Skip, is first of triplet: %s\n", $0)
        }
    } else {
        # printf("-- Skipping duplicate %s\n", $0)
    }
    saved_0=$0
    saved_1=$1
    saved_2=$2
    saved_3=$3
    saved_4=$4
    # printf("-- Done handling: %s\n", $0)
}
'
