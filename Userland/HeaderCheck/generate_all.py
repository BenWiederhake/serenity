#!/usr/bin/env python3

import os
import sys
import subprocess

EXCEPTIONS = [
    ':!:Kernel/*', 
    ':!:Userland/DevTools/UserspaceEmulator/*',
    ':!:Userland/DevTools/UserspaceEmulator/MmapRegion.h',  # Weird cycle
    ':!:Userland/Shell/Job.h',  # Weird cycle
]

TEST_FILE_TEMPLATE = '''\
#include <{filename}>
// Check idempotency:
#include <{filename}>
#include <HeaderCheck/Counter.h>

static Counter count_me {{}};
'''

EXPECTED_FILE_TEMPLATE = '''\
#include <HeaderCheck/Counter.h>

int Counter::s_expected = {count};
'''

CMAKELISTS_FILE_TEMPLATE = '''\
set(HEADER_CHECK_SOURCES
    main.cpp
    expected.cpp
    {filenames}
)
set(SOURCES ${{HEADER_CHECK_SOURCES}})

serenity_bin(HeaderCheck)
#    LibC
#    LibCStatic
#    LibGfxDemo
#    LibGfxScaleDemo
target_link_libraries(HeaderCheck
    LibAudio
    LibChess
    LibCompress
    LibCore
    LibCoreDump
    LibCpp
    LibCrypt
    LibCrypto
    LibDebug
    LibDesktop
    LibDiff
    LibGemini
    LibGfx
    LibGUI
    LibHTTP
    LibImageDecoderClient
    LibIPC
    LibJS
    LibKeyboard
    LibLine
    LibM
    LibMarkdown
    LibPCIDB
    LibProtocol
    LibPthread
    LibRegex
    LibShell
    LibTar
    LibTextCodec
    LibThread
    LibTLS
    LibTTF
    LibVT
    LibWeb
    LibX86
)
'''


def get_headers_here():
    result = subprocess.run(['git', 'ls-files', '*.h', *EXCEPTIONS], check=True, capture_output=True)
    assert result.stderr == b''
    output = result.stdout.decode().split('\n')
    assert output[-1] == ''  # Trailing newline
    return output[:-1]


def as_filename(header_path):
    return header_path.replace('/', '__') + '__test.cpp'


def verbosely_write(path, new_content):
    print('Writing {} ...'.format(path))
    # FIXME: Ensure directory exists
    if os.path.exists(path):
        with open(path, 'r') as fp:
            old_data = fp.read()
        if old_data == new_content:
            # Fast path! Don't trigger ninja
            return
    with open(path, 'w') as fp:
        fp.write(new_content)


def generate_part(header):
    content = TEST_FILE_TEMPLATE.format(filename=header)
    if header.startswith('Kernel/'):
        content += '#define KERNEL\n'
    verbosely_write(as_filename(header), content)


def generate_expected(header_list):
    verbosely_write('expected.cpp', EXPECTED_FILE_TEMPLATE.format(count=len(header_list)))


def generate_cmakelists(headers_list):
    filenames = '\n    '.join(as_filename(h) for h in headers_list);
    verbosely_write('CMakeLists.txt', CMAKELISTS_FILE_TEMPLATE.format(filenames=filenames))


def run(root_path):
    os.chdir(root_path)
    headers_list = get_headers_here()
    headers_list.append('WindowServer/WindowClientEndpoint.h')

    generated_files_path = os.path.join(root_path, 'Build', 'Userland', 'HeaderCheck')
    if not os.path.exists(generated_files_path):
        os.mkdir(generated_files_path)
    os.chdir(generated_files_path)
    for header in headers_list:
        generate_part(header)
    generate_expected(headers_list)

    os.chdir(os.path.join(root_path, 'Userland', 'HeaderCheck'))
    generate_cmakelists(headers_list)

    print('Done! Found {} headers.'.format(len(headers_list)))


if __name__ == '__main__':
    if 'SERENITY_ROOT' not in os.environ:
        print('Must set SERENITY_ROOT first!', file=sys.stderr)
        exit(1)
    run(os.environ['SERENITY_ROOT'])
