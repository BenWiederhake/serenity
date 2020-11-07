#!/usr/bin/env python3

import json
import sys

# Must be exactly three lines each!
# No trailing newline! (I.e. keep it as backslash-newline-tripleapostrophe.)
TEMPLATE_PUSH = '''\
{actor} pushed master: {status} https://github.com/SerenityOS/serenity/actions/runs/{run_id}
Subject: {commit}{post_commit}
Details: {compare}\
'''
TEMPLATE_PR = '''\
{actor} {action} PR #{number}: {status} https://github.com/SerenityOS/serenity/actions/runs/{run_id}
Subject: {title}
Details: {link}\
'''

def compute_lines(wrapper):
    actor, run_id, raw_status, event = wrapper

    if raw_status == 'success':
        status = 'The build passed.'
    elif raw_status == 'failure':
        status = 'The build failed.'
    else:
        status = 'The build {}(?)'.format(raw_status)

    if 'action' not in event:
        # This is a push.
        if 'commits' not in event or not event['commits']:
            commit = '??? (No commits in event?!)'
            post_commit = ''
        else:
            commits = event['commits']
            # First line of the last commit:
            commit = commits[-1]['message'].split('\n')[0]
            if len(commits) == 1:
                post_commit = ''
            elif len(commits) == 2:
                post_commit = ' (+1 commit)'
            else:
                post_commit = ' (+{} commits)'.format(len(commits))
        text = TEMPLATE_PUSH.format(
            actor=actor,
            status=status,
            run_id=run_id,
            commit=commit,
            post_commit=post_commit,
            compare=event.get('compare', '???'),
        )
        return text.split('\n')
    elif 'pull_request' in event:
        # This is a PR.
        raw_action = event['action']
        if raw_action == 'opened':
            action = 'opened'
        elif raw_action == 'reopened':
            action = 'reopened'
        elif raw_action == 'synchronize':
            action = 'updated'
        else:
            action = '{}(?)'.format(raw_action)
        text = TEMPLATE_PR.format(
            actor=actor,
            action=action,
            number=event['pull_request'].get('number', '???'),
            status=status,
            run_id=run_id,
            title=event['pull_request'].get('title', '???'),
            link=event['pull_request'].get('_links', dict()).get('html', dict()).get('href', '???'),
        )
        return text.split('\n')
    else:
        print('Unrecognized event type?!')
        return []


def run_on(json_string):
    wrapper = json.loads(json_string)
    lines = compute_lines(wrapper)
    assert len(lines) == 3
    for i, line in enumerate(lines):
        print('::set-output name=line{}::{}'.format(i + 1, line))
        print('> ::set-output name=line{}::{}'.format(i + 1, line))


def run():
    run_on(sys.stdin.read())


if __name__ == '__main__':
    run()
