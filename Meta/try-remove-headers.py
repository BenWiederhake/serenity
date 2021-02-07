#!/usr/bin/env python3

from collections import Counter, defaultdict
import atomicwrites
import hashlib
import json
import os
import re
import subprocess
import sys


MAGIC_FLAG = '--modify-files-and-run-ninja'

# git ls-files -- '*.cpp' '*.h' | xargs grep -Ph '^ *# *include ' | sort -u | tee includes.txt
INCLUDE_REGEX = re.compile(b'^ *# *include ')

# This only skips the following includes, which (at the time of writing) are all false-positives:
# git ls-files -- '*.cpp' '*.h' | xargs grep -Pn '^# *include <[a-z]+>' | sed -Ee 's,:#,: #,'
CLASS_REGEX = re.compile(b'^ *# *include .*/([^/\\.]+)\\.h[>"]')

KNOWN_STATI = {'necessary', 'class', 'unknown', 'weird', 'unmentioned', 'unnecessary'}
# Technically we don't know anything about 'class' includes, but these would be false positives anyway.
CHECK_STATI_ORDER = ['weird', 'unmentioned', 'unknown']

WHITELIST_INCLUDES = {'#include <AK/Types.h>', '#include <AK/StdLibExtras.h>'}


def eprint(msg, end='\n'):
    print(msg, end=end, file=sys.stderr)
    sys.stderr.flush()


def check_context():
    serenity_root = os.environ.get('SERENITY_ROOT')
    if serenity_root is None:
        eprint('Environment variable SERENITY_ROOT is not set. Refusing to guess.')
        eprint('Hint: Run Meta/refresh-serenity-qtcreator.sh to see how to set it.')
        exit(1)
    arbitrary_root_file = os.path.join(serenity_root, 'Documentation')
    if not os.path.exists(arbitrary_root_file):
        eprint('SERENITY_ROOT is set to "{}", but {} does not exist. Configuration error?'.format(
            serenity_root, arbitrary_root_file))
        exit(1)
    for build_dir_file in ['CMakeCache.txt', 'build.ninja', 'CMakeFiles']:
        if not os.path.exists(build_dir_file):
            eprint('This script must be called from a build directory (could not find {})'.format(build_dir_file))
            eprint('Hint: Try running cmake {} -G Ninja -DBUILD_LAGOM=ON'.format(serenity_root))
            exit(1)
    return serenity_root


def list_cpp_h_files(serenity_root):
    raw_result = subprocess.run(
        [
            "git", "ls-files", "--",
            "*.cpp",
            "*.h",
            # ":!:Base",
            # ":!:Kernel/FileSystem/ext2_fs.h",
            # ":!:Userland/Libraries/LibC/getopt.cpp",
            # ":!:Userland/Libraries/LibCore/puff.h",
            # ":!:Userland/Libraries/LibCore/puff.cpp",
            # ":!:Userland/Libraries/LibELF/exec_elf.h"
        ],
        cwd=serenity_root,
        capture_output=True,
    ).stdout.decode().strip('\n').split('\n')
    return [serenity_root + '/' + filename for filename in raw_result]


def compute_data_hexhash(file_contents):
    return hashlib.sha256(file_contents).hexdigest()


def compute_file_hexhash(filename):
    with open(filename, 'rb') as fp:
        file_contents = fp.read()
    return compute_data_hexhash(file_contents)


class ScopedChange:
    def __init__(self, filename, expect_hexhash, new_content):
        self.filename = filename
        self.expect_hexhash = expect_hexhash
        assert isinstance(new_content, bytes)
        self.new_content = new_content
        self.is_changed = False
        self.old_content = None

    def _write(self, content):
        with open(self.filename, 'wb') as fp:
            fp.write(content)

    def __enter__(self):
        assert not self.is_changed
        self.is_changed = True
        with open(self.filename, 'rb') as fp:
            self.old_content = fp.read()
        if self.expect_hexhash is not None:
            actual_hexhash = compute_data_hexhash(self.old_content)
            assert actual_hexhash == self.expect_hexhash, (self.filename, actual_hexhash, self.expect_hexhash)
        self._write(self.new_content)

    def __exit__(self, exc_type, exc_val, exc_tb):
        if exc_type is not None or exc_val is not None or exc_tb is not None:
            eprint('WARNING: Failed for file {}'.format(self.filename))
        if self.old_content is None:
            # Nothing to roll back to; we probably crashed during read.
            return
        self._write(self.old_content)


class IncludesDatabase:
    def __init__(self, root):
        self.filename = 'inclusions_db.json'
        self.root = root

        if os.path.exists(self.filename):
            eprint('Using cached database. To remove the cache, delete {}.'.format(self.filename))
            with open(self.filename, 'r') as fp:
                self.data = json.load(fp)
        else:
            eprint('Building new database in {}.'.format(self.filename))
            self.data = dict()
        # self.data is a dict.
        # - The keys are the individual filenames.
        # - The values are file-include-descriptions, which are a dict:
        #     * key 'hexhash': value is a string, either empty or contains the hex of the sha256 of the entire file
        #     * key 'includes': value is a list of dicts:
        #         * key 'line_number': value is a number, 0-indexed (add 1 before displaying to a human!)
        #         * key 'status': value is any of the strings in KNOWN_STATI
        #         * key 'filename': value is the filename (redundant, but makes things easier)
        #         * key 'line_content': line in question (kinda redundant, but not quite)

        if os.path.exists('whitelist.json'):
            eprint('Using given whitelist at whitelist.json.')
            with open('whitelist.json', 'r') as fp:
                raw_whitelist = json.load(fp)
        else:
            raw_whitelist = []
            eprint('You can create a whitelist.json. Those entries will always be ignored.')
        # raw_whitelist is a list.
        # - The entries are tuples, represented as a list:
        #     * First comes the full, absolute filename (whitelists are not portable, sorry)
        #     * The line content that is allowed to be ignored, for example "#include <AK/StringView.h>".

        self.whitelist = defaultdict(set)
        for raw_entry in raw_whitelist:
            filename, line_content = raw_entry
            self.whitelist[filename].add(line_content)

    def save(self):
        with atomicwrites.atomic_write(self.filename, overwrite=True) as fp:
            json.dump(self.data, fp, indent=2)

    def scan_files(self):
        eprint('Scanning for new files ...', end='')
        all_files = set(list_cpp_h_files(self.root))

        # Remove non-existing files from database entirely
        eprint('..', end='')
        self.data = {filename: filedata for filename, filedata in self.data.items() if filename in all_files}

        # Add new files
        eprint('..', end='')
        for filename in all_files:
            if filename in self.data:
                continue
            self.data[filename] = dict(hexhash='', includes=[])

        # Populate self.data, actually do the scanning
        eprint('..', end='')
        new_files, new_incs, new_by_type = 0, 0, Counter()
        for filename, filedict in self.data.items():
            current_hash = compute_file_hexhash(filename)
            with open(filename, 'rb') as fp:
                file_contents = fp.read()
            current_hash = hashlib.sha256(file_contents).hexdigest()
            if filedict['hexhash'] == current_hash:
                continue
            new_files += 1
            filedict['hexhash'] = current_hash
            # Reset and recalculate 'includes' list:
            filedict['includes'] = []
            classname_from_filename = filename.split('/')[-1].split('.')[0].encode()
            for i, line_content in enumerate(file_contents.split(b'\n')):
                if not INCLUDE_REGEX.match(line_content):
                    continue
                status = 'unknown'
                # Is this the class header?
                if classname_from_filename in line_content:
                    status = 'class'
                else:
                    # Try to determine what class this includes:
                    included_class = CLASS_REGEX.match(line_content)
                    if included_class is not None:
                        # Yes, this is a class-like!
                        # Is it mentioned anywhere in the file?
                        included_class = included_class.groups(1)[0]
                        occurrences = len(re.findall(re.escape(included_class), file_contents))
                        if occurrences < 1:
                            # We always match the include itself, so this cannot happen.
                            eprint('{}:{}: "{}", but cannot find "{}"?!'.format(
                                filename, i + 1, line_content, included_class))
                        elif occurrences == 1:
                            # The included class is mentioned nowhere in the file. Suspicious.
                            status = 'unmentioned'
                    else:
                        # This is a weird header.
                        status = 'weird'
                assert status in KNOWN_STATI, status
                filedict['includes'].append(dict(
                    line_number=i, line_content=line_content.decode(), status=status, filename=filename))
                new_incs += 1
                new_by_type[status] += 1

        # The whitelist can only now be applied, because we want to apply it to both the items
        # from the file database, as well as what we just scanned.
        changed_to_whitelist = 0
        for filename, filedict in self.data.items():
            for include in filedict['includes']:
                if include['line_content'] not in self.whitelist[filename] \
                        and include['line_content'] not in WHITELIST_INCLUDES:
                    continue
                assert include['status'] in KNOWN_STATI
                if include['status'] == 'necessary':
                    # Already marked, no need to do anything.
                    continue
                include['status'] = 'necessary'
                changed_to_whitelist += 1

        eprint('\nDiscovered {} new files (now {} in total) and {} new includes (now {} in total)'.format(
            new_files, len(self.data), new_incs, sum(len(filedict['includes']) for filedict in self.data.values())))
        eprint('Converted {} to "necessary" by the whitelist'.format(changed_to_whitelist))
        eprint('Discovered includes by type: {}'.format(new_by_type))
        total_by_type = Counter()
        for filedict in self.data.values():
            for include in filedict['includes']:
                assert include['status'] in KNOWN_STATI
                total_by_type[include['status']] += 1
        eprint('Total includes by type: {}'.format(total_by_type))

        # Save current state to disk
        self.save()

    def complain_about_unnecessary(self):
        # If we read the database from disk, we probably should
        # complain *again* about known-unnecessary includes.
        any_complaints = False
        for filename, filedict in self.data.items():
            for include in filedict['includes']:
                if include['status'] != 'unnecessary':
                    continue
                self.print_complaint(include)
                any_complaints = True
        if any_complaints:
            eprint('=== (Old complaints complete)')

    def extract_recommended_checks(self):
        recommendations_by_type = defaultdict(list)
        for filename, filedict in self.data.items():
            for include in filedict['includes']:
                assert include['status'] in KNOWN_STATI
                if include['status'] not in CHECK_STATI_ORDER:
                    continue
                # This has the added benefit of switching files as rarely as possible.
                recommendations_by_type[include['status']].append(include)
        recommendations = []
        for status in CHECK_STATI_ORDER:
            recommendations.extend(recommendations_by_type[status])
        return recommendations

    def change_for_recommendation(self, recommendation):
        filename = recommendation['filename']
        line_number = recommendation['line_number']
        expected_hexhash = self.data[filename]['hexhash']
        with open(filename, 'rb') as fp:
            file_contents = fp.read()
        actual_hexhash = compute_data_hexhash(file_contents)
        assert actual_hexhash == expected_hexhash, (expected_hexhash, actual_hexhash, recommendation)
        file_contents = file_contents.split(b'\n')
        assert len(file_contents) > line_number, (len(file_contents), recommendation)
        assert file_contents[line_number].decode() == recommendation['line_content'], \
            (file_contents[line_number], recommendation)
        file_contents[line_number] = b"// Let's try this without: " + file_contents[line_number]
        file_contents = b'\n'.join(file_contents)
        return ScopedChange(filename, expected_hexhash, file_contents)

    def stringify_recommendation(self, recommendation):
        return 'without {}:{}: /* {} // {} */'.format(
            recommendation['filename'],
            recommendation['line_number'] + 1,
            recommendation['line_content'],
            recommendation['status']
        )

    def print_complaint(self, complaint):
        display_filename = complaint['filename']
        if display_filename.startswith(self.root):
            display_filename = display_filename[len(self.root):]
        if display_filename.startswith('/'):
            display_filename = display_filename[1:]

        print('****** Unnecessary include found! ****** {}:{}: {} // {} ["{}", "{}"],'.format(
            display_filename,
            complaint['line_number'] + 1,
            complaint['line_content'],
            complaint['status'],
            complaint['filename'],
            complaint['line_content'],
        ))

    def report_recommendation(self, recommendation, is_necessary):
        # This assert is accidentally quadratic. Delete it if this gets slow.
        assert recommendation in self.data[recommendation['filename']]['includes']

        new_status = 'necessary' if is_necessary else 'unnecessary'
        assert 'necessary' not in recommendation['status']
        recommendation['status'] = new_status

        # This assert is accidentally quadratic. Delete it if this gets slow.
        assert recommendation in self.data[recommendation['filename']]['includes']

        if not is_necessary:
            self.print_complaint(recommendation)
        self.save()


def does_it_build(how):
    eprint('Building {} ... '.format(how), end='')
    result = subprocess.run(['ninja', '-j3'], stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
    success = result.returncode == 0
    eprint(['failed', 'succeeded'][success])
    return success


def run():
    eprint('Preflight checks ... ', end='')
    serenity_root = check_context()
    eprint('complete')

    if not does_it_build('as-is'):
        eprint('Does not build?! Aborting cowardly!')
        # Don't call exit(1) as it skips the context's __exit__ method
        raise AssertionError('Does not build')

    with ScopedChange(serenity_root + '/Meta/Lagom/TestApp.cpp', None, b'#error "This should not compile"\n1=2\n'):
        if does_it_build('with broken Lagom'):
            eprint('Maybe you forgot to enable Lagom?')
            # Don't call exit(1) as it skips the context's __exit__ method
            raise AssertionError('Use -DBUILD_LAGOM=ON')

    db = IncludesDatabase(serenity_root)
    db.scan_files()
    db.complain_about_unnecessary()

    all_recommendations = db.extract_recommended_checks()
    for checks_done, recommendation in enumerate(all_recommendations):
        if checks_done % 10 == 0:
            eprint('Completed {} of {} checks.'.format(checks_done, len(all_recommendations)))
        reason = db.stringify_recommendation(recommendation)
        with db.change_for_recommendation(recommendation):
            is_necessary = not does_it_build(reason)
        db.report_recommendation(recommendation, is_necessary)

    eprint('Finished! All done. You can go home now.')


if __name__ == '__main__':
    if len(sys.argv) != 2 or sys.argv[1] != MAGIC_FLAG:
        eprint('USAGE: {} {}'.format(sys.argv[0], MAGIC_FLAG))
        eprint('Running this will modify your files and run ninja!')
        exit(1)

    run()
