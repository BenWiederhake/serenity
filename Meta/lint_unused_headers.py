#!/usr/bin/env python3

from collections import defaultdict
import json
import re
import subprocess
import sys

HEADER_NETWORK_JSON = "header_inclusion.json"
HEADER_INCLUDE_REGEX_FMT = "\n# *include <([^>]*/)?{}>\n"
HEADER_INCLUDE_REGEX_REPLACEMENT = "\n"

KNOWN_SYMBOLS = [
    # ("AK/AllOf.h", "\\b(all_of)\\b"),
    # ("AK/AnyOf.h", "\\b(any_of)\\b"),
    # ("AK/ArbitrarySizedEnum.h", "\\b(ArbitrarySizedEnum(?!\.h>)|AK_MAKE_ARBITRARY_SIZED_ENUM)\\b"),
    # ("AK/Array.h", "\\b(Array(?!\.h>)|iota_array|integer_sequence_generate_array)\\b"),
    # ("AK/Badge.h", "\\b(Badge(?!\.h>))\\b"),
    # ("AK/BinaryBufferWriter.h", "\\b(BinaryBufferWriter(?!\.h>))\\b"),
    # ("AK/BinaryHeap.h", "\\b(BinaryHeap(?!\.h>))\\b"),
    # ("AK/BitCast.h", "\\b(bit_cast)\\b"),
    # ("AK/Bitmap.h", "\\b(Bitmap(?!\.h>))\\b"),
    # ("AK/BitmapView.h", "\\b(BitmapView(?!\.h>)|bitmask_first_byte|bitmask_last_byte)\\b"),
    # ("AK/BumpAllocator.h", "\\b(BumpAllocator(?!\.h>)|UniformBumpAllocator)\\b"),
    # ("AK/ByteBuffer.h", "\\b(ByteBuffer(?!\.h>)|Traits)\\b"),
    # ("AK/Span.h", "\\b(Bytes|ReadonlyBytes|Span(?!\.h>)|Traits)\\b"),
    # ("AK/Assertions.h", "\\b(ak_verification_failed|assert|VERIFY|VERIFY_NOT_REACHED|TODO|TODO_AARCH64)\\b"),
    # ("AK/Concepts.h", "\\b(AnyString|Arithmetic|ArrayLike|DerivedFrom|Enum|FallibleFunction|FloatingPoint|Fundamental|HashCompatible|Indexable|Integral|IterableContainer|IteratorFunction|IteratorPairWith|OneOf|OneOfIgnoringCV|SameAs|Signed|SpecializationOf|Unsigned|VoidFunction)\\b"),
    # ("AK/Format.h", "\\b(CheckedFormatString|critical_dmesgln|dbgln|dbgln_if|dmesgln|FormatBuilder|__FormatIfSupported|FormatIfSupported|FormatParser|FormatString|Formattable|Formatter|__format_value|HasFormatter|max_format_arguments|out|outln|set_debug_enabled|StandardFormatter|TypeErasedFormatParams|TypeErasedParameter|VariadicFormatParams|v_critical_dmesgln|vdbgln|vdmesgln|vformat|vout|warn|warnln|warnln_if)\\b"),
    ("AK/HashTable.h", "\\b(BucketState|HashSetExistingEntryBehavior|HashSetResult|HashTable(?!\.h>)|HashTableIterator|is_free_bucket|is_used_bucket|OrderedHashTable|OrderedHashTableIterator)\\b"),
    # ("AK/IterationDecision.h", "\\b(IterationDecision(?!\.h>))\\b"),
    # ("AK/Math.h", "\\b(acos|acosh|asin|asinh|atan|atan2|atanh|cbrt|ceil|cos|cosh|E|exp|exp2|fabs|fmod|hypot|log|log10|log2|NaN|Pi|pow|product_even|product_odd|remainder|round_to|rsqrt|sin|sincos|sinh|sqrt|Sqrt1_2|Sqrt2|tan|tanh)\\b"),
    # ("AK/MemMem.h", "\\b(MemMem(?!\.h>)|bitap_bitwise|memmem|memmem_optional)\\b"),
    # ("AK/Memory.h", "\\b(fast_u32_copy|fast_u32_fill|secure_zero|timing_safe_compare)\\b"),
    # ("AK/StdLibExtras.h", "\\b(abs|AK_REPLACED_STD_NAMESPACE|array_size|ceil_div|clamp|exchange|forward|is_constant_evaluated|is_power_of_two|max|min|mix|move|_RawPtr|RawPtr|round_up_to_power_of_two|swap|to_underlying)\\b"),
    # ("AK/StdLibExtraDetails.h", "\\b(AddConst|__AddConstToReferencedType|AddConstToReferencedType|AddLvalueReference|__AddReference|AddRvalueReference|__AssertSize|AssertSize|__CommonType|CommonType|__Conditional|Conditional|CopyConst|__Decay|Decay|declval|DependentFalse|FalseType|__IdentityType|IdentityType|IndexSequence|IntegerSequence|IntegralConstant|IsArithmetic|IsAssignable|IsBaseOf|IsCallableWithArguments|IsClass|IsConst|IsConstructible|IsConvertible|IsCopyAssignable|IsCopyConstructible|IsDestructible|IsEnum|__IsFloatingPoint|IsFloatingPoint|IsFunction|IsFundamental|IsHashCompatible|__IsIntegral|IsIntegral|IsLvalueReference|IsMoveAssignable|IsMoveConstructible|IsNullPointer|IsOneOf|IsOneOfIgnoringCV|IsPOD|IsPointer|__IsPointerHelper|IsPointerOfType|IsRvalueReference|IsSame|IsSameIgnoringCV|IsSigned|IsSpecializationOf|IsTrivial|IsTriviallyAssignable|IsTriviallyConstructible|IsTriviallyCopyable|IsTriviallyCopyAssignable|IsTriviallyCopyConstructible|IsTriviallyDestructible|IsTriviallyMoveAssignable|IsTriviallyMoveConstructible|IsUnion|IsUnsigned|IsVoid|MakeIndexSequence|MakeIntegerSequence|make_integer_sequence_impl|__MakeSigned|MakeSigned|__MakeUnsigned|MakeUnsigned|__RemoveConst|RemoveConst|RemoveCV|RemoveCVReference|__RemovePointer|RemovePointer|__RemoveReference|RemoveReference|__RemoveVolatile|RemoveVolatile|TrueType|UnderlyingType|Void|VoidType)\\b"),
    # ("AK/UFixedBigInt.h", "\\b(Formatter|IsSigned|IsUnsigned|NumericLimits|u1024|u128|u2048|u256|u4096|u512|UFixedBigInt(?!\.h>)|UFixedBigIntMultiplicationResult)\\b"),
    ("AK/Vector.h", "\\b(CanBePlacedInsideVectorHelper|find_all|split|split_limit|split_view|split_view_if|Vector(?!\.h>))\\b"), # HACK: find_all|split|split_limit|split_view|split_view_if
    #("Build/x86_64/AK/Debug.h", "\\bdbgln_if\(|_DEBUG\\b"),
    #("Build/x86_64/Kernel/Debug.h", "\\bdbgln_if\(|_DEBUG\\b"),
    ("Userland/Libraries/LibC/assert.h", "\\b(assert(?!\.h>))\\b"),
    #("Userland/Libraries/LibC/stdlib.h", "\\b(_abort|abort|abs|aligned_alloc|arc4random|arc4random_buf|arc4random_uniform|atexit|atof|atoi|atol|atoll|bsearch|calloc|clearenv|div|div_t|exit|_Exit|EXIT_FAILURE|EXIT_SUCCESS|free|getenv|getprogname|grantpt|labs|ldiv|ldiv_t|llabs|lldiv|lldiv_t|malloc|malloc_good_size|malloc_size|mblen|mbstowcs|mbtowc|mkdtemp|mkstemp|mkstemps|mktemp|posix_memalign|posix_openpt|ptsname|ptsname_r|putenv|qsort|qsort_r|rand|RAND_MAX|random|realloc|realpath|secure_getenv|serenity_dump_malloc_stats|serenity_setenv|setenv|setprogname|srand|srandom|strtod|strtof|strtol|strtold|strtoll|strtoul|strtoull|system|unlockpt|unsetenv|wcstombs|wctomb)\\b"),
    #("AK/Atomic.h", "\\b(Atomic(?!\.h>)|atomic_compare_exchange_strong|atomic_exchange|atomic_fetch_add|atomic_fetch_and|atomic_fetch_or|atomic_fetch_sub|atomic_fetch_xor|atomic_is_lock_free|atomic_load|atomic_signal_fence|atomic_store|atomic_thread_fence|full_memory_barrier)\\b"),
    #("AK/AtomicRefCounted.h", "\\b(AtomicRefCountedBase|AtomicRefCounted(?!\.h>))\\b"),
    #("AK/Base64.h", "\\b(calculate_base64_decoded_length|calculate_base64_encoded_length|decode_base64|decode_forgiving_base64|encode_base64)\\b"),
    #("AK/BinarySearch.h", "\\b(binary_search|DefaultComparator)\\b"),
    #("AK/BuiltinWrappers.h", "\\b(bit_scan_forward|count_leading_zeroes|count_leading_zeroes_safe|count_trailing_zeroes|count_trailing_zeroes_safe|popcount)\\b"),
    #("AK/ByteReader.h", "\\b(ByteReader(?!\.h>)|load|load16|load32|load64|load_pointer|store)\\b"),
    #("AK/CharacterTypes.h", "\\b(is_ascii|is_ascii_alpha|is_ascii_alphanumeric|is_ascii_binary_digit|is_ascii_blank|is_ascii_c0_control|is_ascii_control|is_ascii_digit|is_ascii_graphical|is_ascii_hex_digit|is_ascii_lower_alpha|is_ascii_octal_digit|is_ascii_printable|is_ascii_punctuation|is_ascii_space|is_ascii_upper_alpha|is_unicode|is_unicode_control|is_unicode_noncharacter|is_unicode_scalar_value|is_unicode_surrogate|parse_ascii_base36_digit|parse_ascii_digit|parse_ascii_hex_digit|to_ascii_base36_digit|to_ascii_lowercase|to_ascii_uppercase)\\b"),
    #("AK/Checked.h", "\\b(Checked(?!\.h>)|is_within_range|make_checked|TypeBoundsChecker)\\b"),
    #("AK/CircularBuffer.h", "\\b(CircularBuffer(?!\.h>))\\b"),
    #("AK/CircularDeque.h", "\\b(CircularDeque(?!\.h>))\\b"),
    #("AK/CircularDuplexStream.h", "\\b(CircularDuplexStream(?!\.h>))\\b"),
    #("AK/CircularQueue.h", "\\b(CircularQueue(?!\.h>))\\b"),
    #("AK/Complex.h", "\\b(Complex(?!\.h>)|COMPLEX_NOEXCEPT)\\b"),
    #("AK/DateConstants.h", "\\b(long_day_names|long_month_names|micro_day_names|mini_day_names|short_day_names|short_month_names)\\b"),
    #("AK/DateTimeLexer.h", "\\b(DateTimeLexer(?!\.h>))\\b"),
    #("AK/DefaultDelete.h", "\\b(DefaultDelete(?!\.h>))\\b"),
    #("AK/Demangle.h", "\\b(demangle)\\b"),
    #("AK/DeprecatedString.h", "\\b(build|CaseInsensitiveStringTraits|DeprecatedString(?!\.h>)|escape_html_entities|System|to_deprecated_string|Traits)\\b"),  # HACK: build|System|to_deprecated_string
    #("AK/Diagnostics.h", "\\b(AK_IGNORE_DIAGNOSTIC|_AK_PRAGMA|AK_PRAGMA)\\b"),
    #("AK/DisjointChunks.h", "\\b(ChunkAndOffset|DisjointChunks(?!\.h>)|DisjointIterator|DisjointSpans|shatter_chunk)\\b"),
    #("AK/DistinctNumeric.h", "\\b(AK_TYPEDEF_DISTINCT_NUMERIC_GENERAL|AK_TYPEDEF_DISTINCT_ORDERED_ID|DistinctNumeric(?!\.h>)|DistinctNumericFeature|Traits)\\b"),
    #("AK/DoublyLinkedList.h", "\\b(DoublyLinkedList(?!\.h>)|DoublyLinkedListIterator)\\b"),
    #("AK/Endian.h", "\\b(be16toh|be32toh|be64toh|BigEndian|__BIG_ENDIAN|__BYTE_ORDER|convert_between_host_and_big_endian|convert_between_host_and_little_endian|convert_between_host_and_network_endian|htobe16|htobe32|htobe64|htole16|htole32|htole64|le16toh|le32toh|le64toh|LittleEndian|__LITTLE_ENDIAN|NetworkOrdered)\\b"),
    #("AK/EnumBits.h", "\\b(AK_ENUM_BITWISE_FRIEND_OPERATORS|AK_ENUM_BITWISE_OPERATORS|_AK_ENUM_BITWISE_OPERATORS_INTERNAL)\\b"),
    #("AK/Error.h", "\\b(Error(?!\.h>)|ErrorOr)\\b"),
    #("AK/ExtraMathConstants.h", "\\b(M_TAU|M_DEG2RAD|M_RAD2DEG)\\b"),
    #("AK/Find.h", "\\b(find|find_if|find_index)\\b"),
    #("AK/FixedArray.h", "\\b(FixedArray(?!\.h>))\\b"),
    #("AK/FixedPoint.h", "\\b(FixedPoint(?!\.h>))\\b"),
    #("AK/FloatingPoint.h", "\\b(convert_from_native_double|convert_from_native_float|convert_to_native_double|convert_to_native_float|DoubleFloatingPointBits|FloatExtractor|FloatingPointBits|float_to_float|SingleFloatingPointBits)\\b"),
    #("AK/FloatingPointStringConversions.h", "\\b(floating_point_decimal_separator|FloatingPointError|FloatingPointParseResults|parse_first_floating_point|parse_first_floating_point_until_zero_character|parse_first_hexfloat_until_zero_character|parse_floating_point_completely)\\b"),
    #("AK/FlyString.h", "\\b(FlyString(?!\.h>))\\b"),
    #("AK/Forward.h", "\\b(Array|Atomic|Badge|Bitmap|ByteBuffer|Bytes|CircularBuffer|CircularDuplexStream|CircularQueue|DeprecatedString|DoublyLinkedList|DuplexMemoryStream|Error|ErrorOr|FixedArray|FixedPoint|FlyString|Function|GenericLexer|HashMap|HashTable|InputBitStream|InputMemoryStream|InputStream|IPv4Address|JsonArray|JsonObject|JsonValue|LockRefPtr|LockRefPtrTraits|NonnullLockRefPtr|NonnullLockRefPtrVector|NonnullOwnPtr|NonnullOwnPtrVector|NonnullRefPtr|NonnullRefPtrVector|Optional|OrderedHashMap|OrderedHashTable|OutputBitStream|OutputMemoryStream|OutputStream|OwnPtr|ReadonlyBytes|RefPtr|SimpleIterator|SinglyLinkedList|Span|StackInfo|String|StringBuilder|StringImpl|StringView|Time|Traits|URL|Utf16View|Utf32View|Utf8CodePointIterator|Utf8View|Vector|WeakPtr)\\b"),
    #("AK/FPControl.h", "\\b(get_cw_x87|get_mxcsr|MXCSR|RoundingMode|set_cw_x87|set_mxcsr|SSERoundingModeScope|X87ControlWord|X87RoundingModeScope)\\b"),
    #("AK/Function.h", "\\b(Function(?!\.h>)|IsFunctionObject|IsFunctionPointer)\\b"),
    #("AK/FuzzyMatch.h", "\\b(fuzzy_match|FuzzyMatchResult)\\b"),
    #("AK/GenericLexer.h", "\\b(GenericLexer(?!\.h>)|is_any_of|is_path_separator|is_quote)\\b"),
    #("AK/GenericShorthands.h", "\\b(first_is_larger_or_equal_than_all_of|first_is_larger_or_equal_than_one_of|first_is_larger_than_all_of|first_is_larger_than_one_of|first_is_one_of|first_is_smaller_or_equal_than_all_of|first_is_smaller_or_equal_than_one_of|first_is_smaller_than_all_of|first_is_smaller_than_one_of)\\b"),
    #("AK/HashFunctions.h", "\\b(double_hash|int_hash|pair_int_hash|ptr_hash|u64_hash)\\b"),
    #("AK/HashMap.h", "\\b(HashMap(?!\.h>)|OrderedHashMap)\\b"),
    #("AK/Hex.h", "\\b(decode_hex|decode_hex_digit|encode_hex)\\b"),
    #("AK/IDAllocator.h", "\\b(IDAllocator(?!\.h>))\\b"),
    #("AK/InsertionSort.h", "\\b(insertion_sort)\\b"),
    #("AK/IntegralMath.h", "\\b(pow|ceil_log2|exp2|log2)\\b"),
    #("AK/IntrusiveDetails.h", "\\b(SelfReferenceIfNeeded|SubstituteIntrusiveContainerType)\\b"),
    #("AK/IntrusiveList.h", "\\b(ExtractIntrusiveListTypes|IntrusiveList(?!\.h>)|IntrusiveListNode|IntrusiveListStorage|SubstitutedIntrusiveListNode)\\b"),
    #("AK/IntrusiveListRelaxedConst.h", "\\b(IntrusiveListRelaxedConst(?!\.h>))\\b"),
    #("AK/IntrusiveRedBlackTree.h", "\\b(ExtractIntrusiveRedBlackTreeTypes|IntrusiveRedBlackTree(?!\.h>)|IntrusiveRedBlackTreeNode|SubstitutedIntrusiveRedBlackTreeNode)\\b"),
    #("AK/IPv4Address.h", "\\b(IPv4Address(?!\.h>))\\b"),
    #("AK/IPv6Address.h", "\\b(IPv6Address(?!\.h>))\\b"),
    #("AK/Iterator.h", "\\b(SimpleIterator)\\b"),
    #("AK/JsonArray.h", "\\b(JsonArray(?!\.h>)|as_array)\\b"),
    #("AK/JsonArraySerializer.h", "\\b(IsLegacyBuilder|JsonArraySerializer(?!\.h>)|JsonObjectSerializer)\\b"),
    #("AK/JsonObject.h", "\\b(JsonObject(?!\.h>)|JsonValue|as_object)\\b"),
    #("AK/JsonObjectSerializer.h", "\\b(JsonObjectSerializer(?!\.h>)|JsonArraySerializer)\\b"),
    #("AK/JsonParser.h", "\\b(JsonParser(?!\.h>))\\b"),
    #("AK/JsonPath.h", "\\b(JsonPath(?!\.h>)|JsonPathElement)\\b"),
    #("AK/JsonValue.h", "\\b(JsonValue(?!\.h>))\\b"),
    # -------------------------------------
    # -------------------------------------
    # -------------------------------------
    #("AK/kmalloc.h", "\\b(kcalloc|kfree_sized|kmalloc(?!\.h>)|kmalloc_array|kmalloc_good_size|malloc_good_size|new|nothrow)\\b"),
    #("AK/kstdio.h", "\\b(dbgputstr|snprintf|sprintf)\\b"),
    #("AK/LEB128.h", "\\b(LEB128(?!\.h>))\\b"),
    #("AK/LexicalPath.h", "\\b(LexicalPath(?!\.h>))\\b"),
    #("AK/MACAddress.h", "\\b(MACAddress(?!\.h>))\\b"),
    # -------------------------------------
    # -------------------------------------
    # -------------------------------------
    # ("AK/NeverDestroyed.h", "\\b(NeverDestroyed(?!\.h>))\\b"),
    # ("AK/NoAllocationGuard.h", "\\b(NoAllocationGuard(?!\.h>))\\b"),
    # ("AK/Noncopyable.h", "\\b(AK_MAKE_NONCOPYABLE|AK_MAKE_NONMOVABLE)\\b"),
    # ("AK/NonnullOwnPtr.h", "\\b(adopt_own|make|NonnullOwnPtr(?!\.h>)|swap|WeakPtr)\\b"),
    # ("AK/NonnullOwnPtrVector.h", "\\b(NonnullOwnPtrVector(?!\.h>))\\b"),
    # ("AK/NonnullPtrVector.h", "\\b(NonnullPtrVector(?!\.h>))\\b"),
    # ("AK/NonnullRefPtr.h", "\\b(adopt_ref|make_ref_counted|NonnullRefPtr(?!\.h>)|NONNULLREFPTR_SCRUB_BYTE|ref_if_not_null|RefPtr|swap|unref_if_not_null)\\b"),
    # ("AK/NonnullRefPtrVector.h", "\\b(NonnullRefPtrVector(?!\.h>))\\b"),
    # ("AK/NumberFormat.h", "\\b(HumanReadableBasedOn|human_readable_digital_time|human_readable_quantity|human_readable_size|human_readable_size_long|human_readable_time)\\b"),
    # ("AK/NumericLimits.h", "\\b(NumericLimits(?!\.h>))\\b"),
    # ("AK/Optional.h", "\\b(ConditionallyResultType|Optional(?!\.h>)|OptionalNone)\\b"),
    # ("AK/OwnPtr.h", "\\b(adopt_nonnull_own_or_enomem|adopt_own_if_nonnull|OwnPtr(?!\.h>)|OWNPTR_SCRUB_BYTE|swap|try_make)\\b"),
    # ("AK/Platform.h", "\\b(AK_ARCH_32_BIT|AK_ARCH_64_BIT|AK_ARCH_AARCH64|AK_ARCH_WASM32|AK_ARCH_X86_64|AK_CACHE_ALIGNED|AK_COMPILER_CLANG|AK_COMPILER_GCC|AK_HAS_CONDITIONALLY_TRIVIAL|AK_OS_ANDROID|AK_OS_BSD_GENERIC|AK_OS_DRAGONFLY|AK_OS_EMSCRIPTEN|AK_OS_FREEBSD|AK_OS_LINUX|AK_OS_MACOS|AK_OS_NETBSD|AK_OS_OPENBSD|AK_OS_SERENITY|AK_OS_WINDOWS|AK_SYSTEM_CACHE_ALIGNMENT_SIZE|ALWAYS_INLINE|ARCH|ASAN_POISON_MEMORY_REGION|ASAN_UNPOISON_MEMORY_REGION|CLOCK_MONOTONIC_COARSE|CLOCK_REALTIME_COARSE|DISALLOW|FLATTEN|HAS_ADDRESS_SANITIZER|__has_feature|NAKED|NEVER_INLINE|NO_SANITIZE_ADDRESS|PAGE_SIZE|RETURNS_NONNULL|__STR|STR|USING_AK_GLOBALLY|VALIDATE_IS_AARCH64|VALIDATE_IS_X86)\\b"),
    # ("AK/PrintfImplementation.h", "\\b(print_double|PrintfImpl|PrintfImplementation(?!\.h>)|printf_internal|print_hex|print_octal_number|print_signed_number|print_string|strlen|VaArgNextArgument)\\b"),
    # ("AK/Ptr32.h", "\\b(Ptr32(?!\.h>))\\b"),
    # ("AK/Queue.h", "\\b(Queue(?!\.h>))\\b"),
    # ("AK/QuickSort.h", "\\b(dual_pivot_quick_sort|INSERTION_SORT_CUTOFF|quick_sort|single_pivot_quick_sort)\\b"),
    # ("AK/Random.h", "\\b(fill_with_random|get_random|get_random_uniform|shuffle)\\b"),
    # ("AK/RecursionDecision.h", "\\b(RecursionDecision(?!\.h>))\\b"),
    # ("AK/RedBlackTree.h", "\\b(BaseRedBlackTree|RedBlackTree(?!\.h>)|RedBlackTreeIterator)\\b"),
    # ("AK/RefCounted.h", "\\b(RefCounted(?!\.h>)|RefCountedBase)\\b"),
    # ("AK/RefCountForwarder.h", "\\b(RefCountForwarder(?!\.h>))\\b"),
]
assert len(KNOWN_SYMBOLS) == len(set(e[0] for e in KNOWN_SYMBOLS))

COMMIT_MSG_TEMPLATE = """\
Everywhere: Remove unused includes of {header}

These instances were detected by searching for files that include
{header}, but don't match the following regex:

{regex_wrapped}

This regex is pessimistic, so there might be more files that don't
use this header at all.

(This is a semi-automated commit.)\
"""
LINE_LENGTH = 72


def invert_network(network):
    the_inverse = defaultdict(list)
    for parent, children in network.items():
        for child in children:
            the_inverse[child].append(parent)
    return the_inverse


def check_git_clean_or_squash(header_name):
    has_diff = subprocess.run(['git', 'diff', '--quiet'])
    if has_diff.returncode == 0:
        return
    subprocess.run(['git', 'diff', '-U0'], check=True)
    squash_yn = input(f"Git is not clean! Squash working directory into \x1b[33m{header_name}\x1b[0m changes? [yn] >")
    if squash_yn != "y":
        exit(2)


def verify_that_it_builds(changed_files):
    for target in 'lagom x86_64'.split():
        # FIXME: If aarch, use "Clang", with capital "C".
        print(f"\x1b[32mBUILDING\x1b[0m {target} ...")
        subprocess.run(
            ['./Meta/serenity.sh', 'build', target],
            check=True
        )


def wrap_regex(regex):
    regex = regex.replace('\\', '\\\\')
    if len(regex) <= LINE_LENGTH:
        return regex
    regex_wrapped = []
    while regex:
        part, regex = regex[ : LINE_LENGTH], regex[LINE_LENGTH : ]
        regex_wrapped.append(part)
    linebreak_plural_letter = "s" if len(regex_wrapped) > 2 else ""
    regex_wrapped.append("")
    regex_wrapped.append(f"(without the linebreak{linebreak_plural_letter})")
    return '\n'.join(regex_wrapped)


def phrase_commit(header, regex):
    return COMMIT_MSG_TEMPLATE.format(
        header=header.split("/")[-1],
        regex_wrapped=wrap_regex(regex),
    )


def commit_it(header, regex):
    print(f"\x1b[32mCOMMITTING\x1b[0m because it looks so good!")
    subprocess.run(['git', 'add', '-u'], check=True)
    subprocess.run(
        ['git', 'commit', '-m', phrase_commit(header, regex)],
        check=True,
    )


def check_necessity(includers, header_name, usage_regex, do_replace):
    print(f"Checking {len(includers)} files for usages of {header_name} ...")
    if len(includers) == 0:
        print(f"\x1b[31mWARNING\x1b[0m: {header_name} has no includers?! Check for errors in {HEADER_NETWORK_JSON}")
        return 0
    usage_re = re.compile(usage_regex)
    header_include_re = re.compile(HEADER_INCLUDE_REGEX_FMT.format(header_name.split("/")[-1]))
    if usage_re.search(f"#include <{header_name}>"):
        print("\x1b[31mWARNING\x1b[0m: Regex '{usage_regex}' matches its own include! This search will likely be false-negative.")
    num_unused = 0
    num_removed = 0
    changed_files = []
    for includer in includers:
        with open(includer, "r") as fp:
            content = fp.read()
        if usage_re.search(content):
            continue
        print(f"\x1b[32mFINDING\x1b[0m: {includer}")
        num_unused += 1
        if not do_replace:
            continue
        new_content, replacements = header_include_re.subn(HEADER_INCLUDE_REGEX_REPLACEMENT, content, re.DOTALL)
        if replacements != 1:
            print(f"\x1b[31mERROR\x1b[0m: Expected file {includer} to include header {header_name} exactly once, but found {replacements} occurrences of regex '{header_include_re}'. Not replacing!")
            exit(2)
            continue
        if num_removed == 0:
            # Assert that the working directory is clean OR that the user know what they're doing.
            check_git_clean_or_squash(header_name)
        num_removed += 1
        changed_files.append(includer)
        with open(includer, "w") as fp:
            fp.write(new_content)
        subprocess.run(['clang-format-15', '-i', includer], check=True)
    assert len(changed_files) == num_removed
    if num_unused != 0:
        if do_replace:
            verify_that_it_builds(changed_files)
            commit_it(header_name, usage_regex)
        else:
            print(phrase_commit(header_name, usage_regex))
    return num_unused, num_removed


def run(do_replace):
    with open(HEADER_NETWORK_JSON, 'r') as fp:
        network = json.load(fp)
    network_inverse = invert_network(network)
    num_unused = 0
    num_checked = 0
    num_removed = 0
    for header_name, usage_regex in KNOWN_SYMBOLS:
        includers = network_inverse[header_name]
        num_checked += len(includers)
        newly_unused, newly_removed = check_necessity(includers, header_name, usage_regex, do_replace)
        num_unused += newly_unused
        num_removed += newly_removed
    total_includes = sum(len(v) for v in network_inverse.values())
    total_includes -= num_removed
    total_remaining_checked = num_checked - num_removed
    ratio_checked = 100 * total_remaining_checked / total_includes
    print(f"Checked {total_remaining_checked}/{total_includes} = {ratio_checked:.5}% includes.")
    if num_unused:
        print(f"There were {num_unused} unused includes, removed {num_removed} of them.")
    else:
        print(f"No unused includes. Hooray!")
    if num_unused != num_removed:
        exit(1)


if __name__ == '__main__':
    if len(sys.argv) == 1:
        run(False)
    elif len(sys.argv) == 2 and sys.argv[1] == "--auto-replace":
        run(True)
    else:
        print(f"USAGE: {sys.argv[0]} [--auto-replace]", file=sys.stderr)
        exit(1)
