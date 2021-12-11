/*
 * Copyright (c) 2018-2021, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021, Ben Wiederhake <BenWiederhake.GitHub@gmx.de>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Array.h>
#include <AK/Assertions.h>
#include <AK/Format.h>
//#include <AK/IPv4Address.h>
//#include <AK/StdLibExtras.h>
#include <AK/OwnPtr.h>
#include <AK/Types.h>
#include <LibC/sys/arch/i386/regs.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/File.h>
#include <LibCore/System.h>
#include <LibIoTrace/IoTraceFormat.h>
#include <errno.h>
//#include <fcntl.h>
//#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/ioctl.h>
//#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/socket.h>
//#include <sys/time.h>
#include <sys/wait.h>
#include <syscall.h>
#include <unistd.h>

static int g_pid = -1;

#if ARCH(I386)
using syscall_arg_t = u32;
#else
using syscall_arg_t = u64;
#endif

static void handle_sigint(int)
{
    if (g_pid == -1)
        return;

    if (ptrace(PT_DETACH, g_pid, 0, 0) == -1) {
        perror("detach");
    }
}

//static ErrorOr<void> copy_from_process(const void* source, Bytes target)
//{
//    return Core::System::ptrace_peekbuf(g_pid, const_cast<void*>(source), target);
//}
//
//static ErrorOr<ByteBuffer> copy_from_process(const void* source, size_t length)
//{
//    auto buffer = ByteBuffer::create_uninitialized(length);
//    if (!buffer.has_value()) {
//        // Allocation failed. Inject an error:
//        return Error::from_errno(ENOMEM);
//    }
//    TRY(copy_from_process(source, buffer->bytes()));
//    return buffer.release_value();
//}
//
//template<typename T>
//static ErrorOr<T> copy_from_process(const T* source)
//{
//    T value {};
//    TRY(copy_from_process(source, Bytes { &value, sizeof(T) }));
//    return value;
//}

static void handle_syscall(syscall_arg_t syscall_function, syscall_arg_t arg1, syscall_arg_t arg2, syscall_arg_t arg3, syscall_arg_t res)
{
    warnln("syscall {} ({} {} {} __) = {}", syscall_function, arg1, arg2, arg3, res);
}

int main(int argc, char** argv)
{
    // FIXME: pledge stdio wpath cpath proc exec ptrace sigaction

    Vector<const char*> child_argv;

    const char* output_filename = nullptr;
    auto trace_file = Core::File::standard_error();

    Core::ArgsParser parser;
    parser.set_stop_on_first_non_option(true);
    parser.set_general_help("Trace all I/O of a process.");
    parser.add_option(g_pid, "Trace the given PID", "pid", 'p', "pid");
    parser.add_option(output_filename, "Filename to write output to", "output", 'o', "output");
    // FIXME: socket or something?
    parser.add_positional_argument(child_argv, "Arguments to exec", "argument", Core::ArgsParser::Required::No);

    parser.parse(argc, argv);

    if (output_filename != nullptr) {
        auto open_result = Core::File::open(output_filename, Core::OpenMode::WriteOnly);
        if (open_result.is_error()) {
            outln(stderr, "Failed to open output file: {}", open_result.error());
            return 1;
        }
        trace_file = open_result.value();
    }

    int status;
    if (g_pid == -1) {
        if (child_argv.is_empty()) {
            warnln("strace: Expected either a pid or some arguments");
            return 1;
        }

        child_argv.append(nullptr);
        int pid = fork();
        if (pid < 0) {
            perror("fork");
            return 1;
        }

        if (!pid) {
            if (ptrace(PT_TRACE_ME, 0, 0, 0) == -1) {
                perror("traceme");
                return 1;
            }
            int rc = execvp(child_argv.first(), const_cast<char**>(child_argv.data()));
            if (rc < 0) {
                perror("execvp");
                exit(1);
            }
            VERIFY_NOT_REACHED();
        }

        g_pid = pid;
        if (waitpid(pid, &status, WSTOPPED | WEXITED) != pid || !WIFSTOPPED(status)) {
            perror("waitpid");
            return 1;
        }
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa, nullptr);

    if (ptrace(PT_ATTACH, g_pid, 0, 0) == -1) {
        perror("attach");
        return 1;
    }
    if (waitpid(g_pid, &status, WSTOPPED | WEXITED) != g_pid || !WIFSTOPPED(status)) {
        perror("waitpid");
        return 1;
    }

    for (;;) {
        if (ptrace(PT_SYSCALL, g_pid, 0, 0) == -1) {
            perror("syscall");
            return 1;
        }
        if (waitpid(g_pid, &status, WSTOPPED | WEXITED) != g_pid || !WIFSTOPPED(status)) {
            perror("wait_pid");
            return 1;
        }
        PtraceRegisters regs = {};
        if (ptrace(PT_GETREGS, g_pid, &regs, 0) == -1) {
            perror("getregs");
            return 1;
        }
#if ARCH(I386)
        syscall_arg_t syscall_index = regs.eax;
        syscall_arg_t arg1 = regs.edx;
        syscall_arg_t arg2 = regs.ecx;
        syscall_arg_t arg3 = regs.ebx;
#else
        syscall_arg_t syscall_index = regs.rax;
        syscall_arg_t arg1 = regs.rdx;
        syscall_arg_t arg2 = regs.rcx;
        syscall_arg_t arg3 = regs.rbx;
#endif

        if (ptrace(PT_SYSCALL, g_pid, 0, 0) == -1) {
            perror("syscall");
            return 1;
        }
        if (waitpid(g_pid, &status, WSTOPPED | WEXITED) != g_pid || !WIFSTOPPED(status)) {
            perror("wait_pid");
            return 1;
        }

        if (ptrace(PT_GETREGS, g_pid, &regs, 0) == -1) {
            perror("getregs");
            return 1;
        }

#if ARCH(I386)
        u32 res = regs.eax;
#else
        u64 res = regs.rax;
#endif

        handle_syscall(syscall_index, arg1, arg2, arg3, res);

        //if (!trace_file->write(builder.string_view())) {
        //    warnln("write: {}", trace_file->error_string());
        //    return 1;
        //}
    }
}
