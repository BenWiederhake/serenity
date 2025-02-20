#!/usr/bin/env python3

from collections import defaultdict, Counter
import os
import random
import re
import subprocess
import sys

# Hard-code some types that are too hard to detect automatically,
# in particular those built with 'using Foo = Bar<…>;'
ALTERNATIVE_TYPES = {
    'Array.h': {'integer_sequence_generate_array'},
    'Atomic.h': {'atomic_thread_fence'},
    'BIOS.h': {'map_bios'},
    'CommandLine.h': {'kernel_command_line'},
    'DistinctNumeric.h': {'TYPEDEF_DISTINCT_ORDERED_ID'},
    'Format.h': {'outln', 'warnln', 'dbgln'},
    'Forward.h': {'ReadonlyBytes', 'Bytes'},
    'Interrupts.h': {'GENERATE_GENERIC_INTERRUPT_HANDLER_ASM_ENTRY'},
    'IO.h': {'in8', 'out8'},
    'JsonArray.h': {'.as_array->'},  # Ugly hack
    'JsonObject.h': {'.as_object().'},  # Ugly hack
    'JsonObjectSerializer.h': {'add_object('},  # Ugly hack
    'MemoryManager.h': {'MM'},
    'PhysicalPage.h': {'paddr()'},
    'PrintfImplementation.h': {'printf_internal'},
    'Random.h': {'get_random_uniform', 'get_fast_random'},
    'Span.h': {'ReadonlyBytes', 'Bytes'},
    'StdLib.h': {'_from_user', 'user_atomic', 'copy_to_user', 'RemoveReference', 'memcpy'},
    'StdLibExtraDetails.h': {'IsArithmetic', 'IsTriviallyConstructible', 'IsUnsigned', 'RemoveReference'},
    'TSS.h': {'TSS'},
    'TypedMapping.h': {'map_typed'},
    'Types.h': {
        'u8', 'u16', 'u32', 'u64',
        'i8', 'i16', 'i32', 'i64',
        'size_t', 'FlatPtr',
        # And so many more
        'class DistinctNumeric {',  # Ugly hack
    },
    'Userspace.h': {'static_ptr_cast'},
}

ALTERNATIVE_RESTRICTIONS = {
    'AllOf.h': {'all_of'},
    'AnyOf.h': {'any_of'},
    'Assertions.h': {'assert', 'TODO', 'VERIFY'},
    'Base64.h': {'decode_base64', 'encode_base64'},
    'CharacterTypes.h': {'to_unicode_', 'code_point_has_', 'is_ascii', 'is_unicode', 'to_ascii_base36_digit'},
    'Find.h': {'find'},
    'HashFunctions.h': {'int_hash', 'double_hash', 'ptr_hash'},
    'Hex.h': {'decode_hex', 'encode_hex'},
    'mman.h': {'MAP_ANON', 'PROT_READ', 'MAP_FAILED', 'mmap', 'munmap', 'mprotect', 'mlock'},
    'MemMem.h': {'memmem'},
    'Memory.h': {'fast_u32_copy', 'fast_u32_fill', 'memcmp', 'strcmp', 'memcpy', 'memset'},
    'Noncopyable.h': {'AK_MAKE_NONCOPYABLE', 'AK_MAKE_NONMOVABLE'},
    'Platform.h': {'ALWAYS_INLINE', 'NEVER_INLINE', 'count_trailing_zeroes_32', 'using u64 = __UINT64_TYPE__;'},  # Ugly hack
    'QuickSort.h': {'quick_sort'},
    'StringHash.h': {'string_hash'},
    'Try.h': {'class [[nodiscard]] KResult {'},  # Ugly hack
    # 'StdLibExtras.h': {
    #     'round_up_to_power_of_two', 'forward', 'move', 'array_size',
    #     'min', 'max', 'clamp', 'swap', 'is_constant_evaluated',
    # },
}

GIT_LS_FILES_COMMAND = [
    'git',
    'ls-files',
    '--',
    '*.cpp',
    '*.h',
    ':!:Base',
    ':!:Kernel/FileSystem/ext2_fs.h',
    ':!:Userland/DevTools/HackStudio/LanguageServers/Cpp/Tests/*',
    ':!:Userland/Libraries/LibCpp/Tests/parser/*',
    ':!:Userland/Libraries/LibCpp/Tests/preprocessor/*',
]

NINJA_BUILD_COMMAND = [
    'ninja',
    '-C',
    'Build/i686',
    '-j7',
]

GIT_COMMIT_COMMAND_PREFIX = [
    'git',
    'commit',
    '--no-verify',
    '-q',
    '-m',
]

DESTRUCTIVE = True

# We also detect bare declarations here. Over-zealous "forward" declarations make this entire lint too lenient.
RE_CLASS_DETECTION = re.compile('^(?!//)(?:requires\([^)]+\) )?(?<!friend )(?:enum class|enum|struct|class) (?:\[\[(?:nodiscard|gnu::packed)\]\] )*([A-Z][A-Za-z0-9_]+)(?:[ ;<]|$)')
RE_IMPORT_DETECTION = re.compile('^#include [<"](?:[^>"/]+/)*([A-Za-z0-9_]+\.h)[>"]$')
FMT_RE_USAGE_DETECTION = '^(?! *#)(?:(?!//).)*({})'


def list_files():
    result = subprocess.run(GIT_LS_FILES_COMMAND, capture_output=True, text=True)
    if result.returncode or result.stderr:
        print("Call failed:", file=sys.stderr)
        print(result.stderr, file=sys.stderr)
        exit(result.returncode or 1)
    files = result.stdout.split('\n')[:-1]
    print('Found {} files'.format(len(files)))
    #random.shuffle(files)
    return files


def readlines(filename):
    with open(filename, 'r') as fp:
        lines = fp.read().split('\n')
    assert lines[-1] == ''
    return [l + '\n' for l in lines[:-1]]


def determine_classes(lines):
    akku = []
    for line in lines:
        akku.extend(RE_CLASS_DETECTION.findall(line))
    return akku


def barename(filename):
    return filename.split('/')[-1]


def determine_imports(lines):
    akku = []
    for i, line in enumerate(lines):
        detected = RE_IMPORT_DETECTION.findall(line)
        assert 0 <= len(detected) <= 1, (line, detected)
        if not detected:
            continue
        akku.append((i, detected[0]))
    return akku


def has_any_uses(lines, class_names):
    assert len(class_names) > 0 and all(len(cn) > 0 for cn in class_names), class_names
    re_usage = re.compile(FMT_RE_USAGE_DETECTION.format('|'.join(re.escape(cn) for cn in class_names)))
    for line in lines:
        usages = re_usage.findall(line)
        if usages:
            return usages[0]
    return False


def write_lines(filename, lines):
    with open(filename, 'w') as fp:
        fp.write(''.join(lines))

def try_build(filename, lines, importline):
    if not DESTRUCTIVE:
        return False
    new_lines = lines.copy()
    assert new_lines[importline] != ''
    new_lines[importline] = ''
    write_lines(filename, new_lines)
    result = subprocess.run(NINJA_BUILD_COMMAND, capture_output=True, text=True)
    return result.returncode == 0


def do_commit(filename, lines):
    assert DESTRUCTIVE
    module, mod_filename = filename.split('/', 1)
    command = GIT_COMMIT_COMMAND_PREFIX + ['{}: Remove unused imports in {}'.format(module, mod_filename), filename]
    result = subprocess.run(command, check=True)


def run():
    files = list_files()
    ignore_count = Counter()

    # Pass 1: Determine which files make what classes available:
    classes_dict = defaultdict(set)
    for filename in files:
        classes = determine_classes(readlines(filename))
        if classes:
            # If multiple files (in different directories) have the same name, we can't always
            # easily figure out which one was meant. Therefore, treat them identically.
            classes_dict[barename(filename)].update(classes)

    for a_key, a_value in ALTERNATIVE_TYPES.items():
        if a_key not in classes_dict:
            print('WARNING: {} has no classes?! Ignoring whitelist!'.format(a_key))
            continue
        classes_dict[a_key].update(a_value)
    for a_key, a_value in ALTERNATIVE_RESTRICTIONS.items():
        if a_key in classes_dict:
            print('WARNING: {} has detected classes?! ({}) Ignoring restictions!'.format(a_key, classes_dict[a_key]))
            continue
        classes_dict[a_key] = a_value

    # Pass 2: Check each import for justification
    any_problems = False
    for filenumber, filename in enumerate(files):
        if '/x86/' in filename:
            # Actually, I don't wanna interfere too much in here.
            continue
        if not filename.endswith('.cpp'):
            # Actually, I wanna focus my efforts better.
            continue
        lines = readlines(filename)
        imports = determine_imports(lines)
        file_changed = False
        for importline, importname in imports:
            if importname not in classes_dict:
                # Not a recognized "class(es)" file, so let's err on the lenient side.
                # print('___ IGNORED: {} in {} (no associations)'.format(importname, filename))
                ignore_count[importname] += 1
                continue
            class_names = classes_dict[importname]
            assert class_names, (filename, importname)
            usage = has_any_uses(lines, class_names)
            if usage:
                # Great!
                # print('___ GOOD: {} in {} is justified! ("{}")'.format(importname, filename, usage))
                continue
            print('___ FOUND: {} in {} is not justified: Couldn\'t find any of {}'.format(importname, filename, class_names))
            if try_build(filename, lines, importline):
                print('___ GOOD: {} in {} has been removed.'.format(importname, filename))
                # Need to update own model; already written by try_build
                lines[importline] = ''
                file_changed = True
            else:
                print('BAD: {} in {} is not justified: Couldn\'t find any of {}'.format(importname, filename, class_names))
                # Need to roll back
                write_lines(filename, lines)
        if file_changed:
            do_commit(filename, lines)
            print('___ GOOD: {} committed. (File {}/{})'.format(filename, filenumber, len(files)))

    #if ignore_count and ignore_count.most_common(1)[0][1] > 5:
    print('Ignored imports:')
    for importname, count in ignore_count.most_common():
        print('    {} ({} times)'.format(importname, count))

    print('Run finished.')

if __name__ == '__main__':
    os.chdir(f"{os.path.dirname(__file__)}/../")
    run()
