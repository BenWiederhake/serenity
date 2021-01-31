#!/usr/bin/env python3

import json
import os
import sys


MAGIC_FLAG = '--modify-files-and-run-ninja'


def eprint(msg):
    print(msg, file=sys.stderr)


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
    return subprocess.run(
        [
            "git", "ls-files", serenity_root, "--",
            "*.cpp",
            "*.h",
            #":!:Base",
            #":!:Kernel/FileSystem/ext2_fs.h",
            #":!:Userland/Libraries/LibC/getopt.cpp",
            #":!:Userland/Libraries/LibCore/puff.h",
            #":!:Userland/Libraries/LibCore/puff.cpp",
            #":!:Userland/Libraries/LibELF/exec_elf.h"
        ],
        capture_output=True,
    ).stdout.decode().strip('\n').split('\n')


class IncludesDatabase:
    def __init__(self, root):
        self.filename = 'inclusions_db.json'
        self.root = root
        if os.path.exists(self.filename):
            eprint('Using cached database. To remove the cache, delete {}.'.format(self.filename))
            with open(self.filename, 'r') as fp:
                self.data = json.load(fp)
        else:
            self.data = dict(files=dict())
            self.save()

    def save(self):
        with open(self.filename, 'w') as fp:
            json.dump(fp, self.data, indent=2)

    def scan_files(self):
        all_files = set(list_cpp_h_files(self.root))

        # Remove non-existing files from database entirely
        self.data = {filename: filedata for filename, filedata in self.data if filename in all_files}

        # Add new files
        for filename in all_files:
            if filename in self.data:
                continue
            self.data[filename] = dict(hashhex='NOT_SCANNED')

        # Populate self.data, actually do the scanning

        # Save current state to disk
        self.save()



def run():
    serenity_root = check_context()

    #if not does_it_build('as-is'):
    #    print('')
    #    raise NotImplementedError

    #with replaced_file('Lagom/file/', '#error "This should not compile"'):
    ## FIXME: Intentionally break some lagom-only code
    #    if does_it_build('with broken Lagom'):
    #        print('')
    #        raise NotImplementedError

    db = IncludesDatabase(serenity_root)
    db.update()
    db.complain_about_unnecessary()

    while True
        file_and_line = db.next_unchecked_include()
        if file_and_line is None:
            break
        raise NotImplementedError

    eprint('Finished! All ')


if __name__ == '__main__':
    if len(sys.argv) != 2 or sys.argv[1] != MAGIC_FLAG:
        eprint('USAGE: {} {}'.format(sys.argv[0], MAGIC_FLAG))
        eprint('Running this will modify your files and run ninja!')
        exit(1)

    run()
