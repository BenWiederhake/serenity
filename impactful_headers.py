#!/usr/bin/env python3

from collections import defaultdict
import json

HEADER_NETWORK_JSON = "header_inclusion.json"


def compute_single_impact(header, inverse_network):
    visited = set()
    impact = 0
    open_set = [header]
    while open_set:
        next_filename = open_set.pop()
        if next_filename.endswith(".cpp"):
            impact += 1
        elif next_filename.endswith(".h") or next_filename.endswith(".ipp"):
            pass
        else:
            raise AssertionError(next_filename)
        for parent in inverse_network[next_filename]:
            if parent in visited:
                continue
            visited.add(parent)
            open_set.append(parent)
    return impact


def invert_network(network):
    the_inverse = defaultdict(list)
    for parent, children in network.items():
        for child in children:
            the_inverse[child].append(parent)
    return the_inverse


def compute_impacts(network):
    the_inverse = invert_network(network)
    actual_headers = list(the_inverse.keys())
    return {header: compute_single_impact(header, the_inverse) for header in actual_headers}


def run():
    with open(HEADER_NETWORK_JSON, 'r') as fp:
        network = json.load(fp)
    header_impacts = compute_impacts(network)
    header_impacts = list(header_impacts.items())
    header_impacts.sort()
    for header_filename, impact in header_impacts:
        with open(header_filename, 'r') as fp:
            line_count = len(fp.readlines())
        print(f"{header_filename},{impact},{impact * line_count}")


if __name__ == '__main__':
    run()
