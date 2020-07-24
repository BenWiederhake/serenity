/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <AK/Optional.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// The maximum integer we can definitely exactly represent as a double (2^52).
// Note that clock_nanosleep can handle even larger numbers (up to around 2^63),
// but I think it's fine to cap silently at a hundred million years.
static const double MAX_SLEEPABLE = 4503599627370496;

void handle_sigint(int)
{
}

double parse_amount(char* arg, bool& ok)
{
    // This can't go into AK/* because AK can't run anything from stdlib,
    // so strtod is not available yet.
    // TODO: Find a way to put this function into AK/
    // Note that `strtod` assumes that `arg` is NUL-terminated.

    char* arg_end = arg + strlen(arg);
    if (arg_end == arg) {
        ok = false;
        return 0.0;
    }

    char* parse_end = nullptr;
    double amount = strtod(arg, &parse_end);
    if (parse_end != arg_end) {
        ok = false;
        return 0.0;
    }

    // We are okay with value overflow; sleeping longer than 1e304 seconds is probably the same as sleeping +inf anyway.

    ok = true;
    return amount;
}

int main(int argc, char** argv)
{
    if (pledge("stdio", nullptr) < 0) {
        perror("pledge");
        return 1;
    }

    if (argc != 2) {
        printf("Usage:\n        sleep <seconds>\n");
        printf("\nArguments:\n        seconds Fractional amount of seconds, e.g. 3, 0.01, 1337.42, or inf. The number will be rounded down to the nearest possible amount (usually by 1 ms).\n");
        return 1;
    }

    bool ok;
    double secs = parse_amount(argv[1], ok);
    if (!ok) {
        fprintf(stderr, "Not a valid number of seconds: \"%s\"\n", argv[1]);
        return 1;
    }
    if (secs < 0) {
        // Don't even bother sleeping.
        return 0;
    }
    if (!(secs >= 0)) {
        // 'msecs' is NaN.
        fprintf(stderr, "Cannot sleep NaN seconds: %s\n", argv[1]);
        return 1;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa, nullptr);

    timespec requested_sleep;
    if (secs > MAX_SLEEPABLE) {
        // The user probably won't notice the difference between
        // 100 million years and anything larger than that.
        secs = MAX_SLEEPABLE;
    }
    requested_sleep.tv_sec = static_cast<long long>(secs);
    requested_sleep.tv_nsec = static_cast<long>((secs - requested_sleep.tv_sec) * 1000000000);

    timespec remaining_sleep;
    int rc = clock_nanosleep(CLOCK_MONOTONIC, 0, &requested_sleep, &remaining_sleep);

    if (rc) {
        fprintf(stderr, "Sleep interrupted with %lld seconds and %ld nanoseconds remaining.\n", remaining_sleep.tv_sec, remaining_sleep.tv_nsec);
    }
    return 0;
}
