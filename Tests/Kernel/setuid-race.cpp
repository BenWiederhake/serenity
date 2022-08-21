/*
 * Copyright (c) 2018-2020, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Format.h>
#include <LibCore/System.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

uid_t my_uid = -1;

static void* spin_setuid(void*)
{
    dbgln("\033[31;1mSpinning on thread {}\033[0m", gettid());

    for (;;) {
        setuid(my_uid);
    }
}

static ErrorOr<int> simple_main()
{
    dbgln("\033[31;1mStarting\033[0m");
    TRY(Core::System::pledge("id thread stdio recvfd sendfd proc exec rpath unix"));

    my_uid = getuid();

    int rc = setuid(my_uid);
    if (rc != 0) {
        perror("setuid, initial");
    }

    dbgln("\033[31;1mCreating\033[0m");

    for (size_t i = 0; i < 10; ++i) {
        pthread_t spinner_thread;
        rc = pthread_create(&spinner_thread, nullptr, spin_setuid, nullptr);
        if (rc != 0) {
            perror("pthread");
            return 1;
        }
    }

    spin_setuid(nullptr);

    return 0;
}

int main()
{
    auto result = simple_main();
    if (result.is_error()) {
        auto error = result.release_error();
        dbgln("\033[31;1mExiting with runtime error\033[0m: {}", error);
        return 1;
    }
    return result.value();
}
