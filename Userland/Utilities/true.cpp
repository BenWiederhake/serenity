/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/ByteBuffer.h>
#include <AK/NumberFormat.h>
#include <AK/Random.h>
#include <LibCore/ElapsedTimer.h>
#include <LibCore/File.h>
#include <LibCore/System.h>
#include <LibMain/Main.h>
#include <stdlib.h> // FIXME: exit

static constexpr size_t TEST_SIZE = 20 * MiB;

ErrorOr<void> run_child_process(int pipefd, ByteBuffer const& expected_buf);
ErrorOr<void> run_child_process(int pipefd, ByteBuffer const& expected_buf)
{
    auto actual_buf = TRY(ByteBuffer::create_uninitialized(TEST_SIZE));
    auto file = TRY(Core::File::adopt_fd(pipefd, Core::File::OpenMode::Read));
    TRY(file->read_until_filled({ actual_buf.offset_pointer(0), 1 }));
    // Only start the timer *after* the first byte for slightly better accuracy.
    // This is because the parent might take a few more ms before actually starting to send any data.
    Core::ElapsedTimer timer { true };
    timer.start();
    TRY(file->read_until_filled({ actual_buf.offset_pointer(1), actual_buf.size() - 1 }));
    auto time_taken = timer.elapsed_time();
    dbgln("Took {}ms to transfer {}.",
        time_taken.to_milliseconds(),
        AK::human_readable_size(TEST_SIZE));
    if (actual_buf != expected_buf) {
        dbgln("ERROR: DIFFERING DATA!");
        VERIFY_NOT_REACHED();
    }
    return {};
}

ErrorOr<void> run_parent_process(int pipefd, ByteBuffer const& expected_buf);
ErrorOr<void> run_parent_process(int pipefd, ByteBuffer const& expected_buf)
{
    auto file = TRY(Core::File::adopt_fd(pipefd, Core::File::OpenMode::Write));
    // We can't reasonably measure on this side without affecting the read-side.
    // Try to stay as hands-off as possible:
    TRY(file->write_until_depleted(expected_buf));
    return {};
}

ErrorOr<int> serenity_main(Main::Arguments)
{
    auto pipefd = TRY(Core::System::pipe2(0));
    dbgln("Created pipe into fds {} and {}.", pipefd[0], pipefd[1]);
    // pipefd[0] refers to the read end of the pipe.
    // pipefd[1] refers to the write end of the pipe.
    auto expected_buf = TRY(ByteBuffer::create_uninitialized(TEST_SIZE));
    AK::fill_with_random(expected_buf);

    if (auto child_pid = TRY(Core::System::fork()); child_pid == 0) {
        dbgln("Child beginning test");
        TRY(Core::System::close(pipefd[1]));
        TRY(run_child_process(pipefd[0], expected_buf));
        exit(0);
    } else {
        dbgln("Parent beginning test");
        TRY(Core::System::close(pipefd[0]));
        TRY(run_parent_process(pipefd[1], expected_buf));
        TRY(Core::System::waitpid(child_pid));
    }
    return 0;
}
