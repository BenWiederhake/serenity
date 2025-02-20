#!/bin/sh

set -e

rm -f commit-evolution.csv
for commit in $(cat special-commits.lst)
do
    echo "Looking at $commit ..."
    git checkout $commit
    shh ninja -C Build/x86_64/ all_generated
    SERENITY_ARCH=x86_64 shh ./Meta/check-headers-acyclic.py
    echo "$commit,$(./impactful_headers.py | ./sum_is.awk)" | tee -a commit-evolution.csv
done
