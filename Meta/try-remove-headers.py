#!/usr/bin/env python3

from collections import Counter
import atomicwrites
import hashlib
import json
import os
import re
import subprocess
import sys


MAGIC_FLAG = '--modify-files-and-run-ninja'
INCLUDE_REGEX = re.compile(b'^ *# *include ')


def eprint(msg, end='\n'):
    print(msg, end=end, file=sys.stderr)


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
            #":!:Base",
            #":!:Kernel/FileSystem/ext2_fs.h",
            #":!:Userland/Libraries/LibC/getopt.cpp",
            #":!:Userland/Libraries/LibCore/puff.h",
            #":!:Userland/Libraries/LibCore/puff.cpp",
            #":!:Userland/Libraries/LibELF/exec_elf.h"
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
            assert actual_hexhash == self.expect_hexhash, (filename, actual_hexhash, self.expect_hexhash)
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
            self.data = dict()
        # self.data is a dict.
        # - The keys are the individual filenames.
        # - The values are file-include-descriptions, which are a dict:
        #     * key 'hexhash': value is a string, either empty or contains the hex of the sha256 of the entire file
        #     * key 'includes': value is a list of dicts:
        #         * key 'line': value is a number, 0-indexed (add 1 before displaying to a human!)
        #         * key 'status': value is either the string 'necessary', 'unnecessary', or 'unknown'

    def save(self):
        with atomicwrites.atomic_write(self.filename, overwrite=True) as fp:
            json.dump(self.data, fp, indent=2)

    def scan_files(self):
        eprint('Scanning for new files ...', end='')
        all_files = set(list_cpp_h_files(self.root)[:100])

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
        new_files, new_incs = 0, 0
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
            for i, line_content in enumerate(file_contents.split(b'\n')):
                if INCLUDE_REGEX.match(line_content):
                    filedict['includes'].append(dict(line=i, status='unknown'))
                    new_incs += 1
        eprint('\nDiscovered {} new files (now {} in total) and {} new includes (now {} in total)'.format(
            new_files, len(self.data), new_incs, sum(len(filedict['includes']) for filedict in self.data.values())))

        # Save current state to disk
        self.save()

    def complain_about_unnecessary(self):
        # If we read the database from disk, we probably should
        # complain *again* about known-unnecessary includes.
        for filename, filedict in self.data.items():
            for include in filedict['includes']:
                if include['status'] != 'unnecessary':
                    continue
                print('{}:{}: Unnecessary include found! [cached]'.format(filename, include['line'] + 1))

    def extract_recommended_checks(self):
        recommendations = []
        for filename, filedict in self.data.items():
            for include in filedict['includes']:
                if include['status'] != 'unknown':
                    continue
                # This has the added benefit of switching files as rarely as possible.
                recommendations.append((filename, include['line']))
        return recommendations

    def change_for_recommendation(self, recommendation):
        filename, line = recommendation
        hexhash = self.data[filename]['hexhash']
        with open(filename, 'rb') as fp:
            file_contents = fp.read()
        actual_hexhash = compute_data_hexhash(file_contents)
        assert actual_hexhash == hexhash, (filename, hexhash, actual_hexhash, line)
        file_contents = file_contents.split(b'\n')
        assert len(file_contents) > line, (filename, len(file_contents), line, hexhash)
        file_contents[line] = b"// Let's try this without: " + file_contents[line]
        file_contents = b''.join(file_contents)
        return ScopedChange(filename, hexhash, file_contents)

    def stringify_recommendation(self, recommendation):
        filename, line = recommendation
        return 'without {}:{}'.format(filename, line + 1)

    def report_recommendation(self, recommendation, is_necessary):
        nec_string = 'necessary' if is_necessary else 'unnecessary'
        filename, line = recommendation
        # Dang, this is accidentally quadratic.
        # If a file has i includes, this will cause i iterations, and will be called i times.
        # Thankfully, the number of includes should be small anyway.
        for include in self.data[filename]['includes']:
            if include['line'] != line:
                continue
            assert include['status'] == 'unknown', (filename, line, nec_string, include)
            include['status'] = nec_string
            if nec_string != 'necessary':
                print('{}:{}: ***NEW*** unnecessary include found!'.format(filename, line + 1))
            self.save()
            return
        raise AssertionError((filename, line, nec_string))


def does_it_build(how):
    eprint('Building {} ... '.format(how), end='')
    result = subprocess.run('ninja', stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
    success = result.returncode == 0
    eprint(['failed', 'succeeded'][success])
    return success


def run():
    serenity_root = check_context()

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

    for recommendation in db.extract_recommended_checks():
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
