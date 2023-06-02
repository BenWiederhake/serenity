/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021-2022, kleines Filmröllchen <filmroellchen@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/NumberFormat.h>
#include <AK/Random.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/ElapsedTimer.h>
#include <LibCore/EventLoop.h>
#include <LibMain/Main.h>
#include <LibSQL/SQLClient.h>

static constexpr size_t MAX_PRINT_MESSAGES = 10;
static constexpr size_t MSG_OVERHEAD = 8;

static ErrorOr<void> send(NonnullRefPtr<SQL::SQLClient> client, DeprecatedString junk_filename)
{
    StringView a;
    auto maybe_conn = client->connect(junk_filename);
    if (maybe_conn.has_value())
        return Error::from_string_literal("Accidentally guessed a valid filename?!");
    return {};
}

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    Optional<size_t> value_size = 10 * MiB;
    Optional<size_t> num_messages = 10;

    Core::ArgsParser args_parser;
    args_parser.add_option(value_size, "Value size per message", "value-size", 's', "number-in-bytes");
    args_parser.add_option(num_messages, "Number of messages to send", "number-tests", 'n', "bare-number");
    args_parser.parse(arguments);

    Core::EventLoop event_loop;
    auto sql_client = TRY(SQL::SQLClient::try_create());

    auto buffer = TRY(ByteBuffer::create_uninitialized(value_size.value()));
    fill_with_random(buffer);
    // Make it extremely unlikely that the name is valid:
    buffer[0] = 0;
    DeprecatedString junk_filename { StringView(buffer) };

    dbgln("Warmup ...");
    TRY(send(sql_client, junk_filename));
    dbgln("Warmup done");

    Core::ElapsedTimer total_timer { true };
    total_timer.start();
    for (size_t i = 0; i < num_messages.value(); ++i) {
        Core::ElapsedTimer timer { true };
        if (i < MAX_PRINT_MESSAGES) {
            dbgln("Sending message #{} ...", i);
            timer.start();
        }
        TRY(send(sql_client, junk_filename));
        if (i < MAX_PRINT_MESSAGES) {
            auto time_taken = timer.elapsed_time();
            auto ms = time_taken.to_milliseconds();
            auto speed = (MSG_OVERHEAD + value_size.value()) * 1e6f / time_taken.to_microseconds();
            dbgln("Sending message #{} took {}ms ({}/s).",
                i, ms, human_readable_size(static_cast<size_t>(speed)));
        }
    }
    auto time_taken = total_timer.elapsed_time();
    auto ms = time_taken.to_milliseconds();
    auto speed = num_messages.value() * 1e6f / time_taken.to_microseconds();
    dbgln("Sending all messages took {}ms ({} msgs/s).", ms, speed);
    speed = num_messages.value() * (MSG_OVERHEAD + value_size.value()) * 1e6f / time_taken.to_microseconds();
    dbgln("  Average speed is {}/s.", human_readable_size(static_cast<size_t>(speed)));

    return 0;
}
