/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Format.h>
#include <AK/Vector.h>
#include <LibMain/Main.h>

static constexpr size_t BufferSize = 128;

using MyVector = Vector<u8, 32>;

static ErrorOr<MyVector> make_vector()
{
    u8 mybytes[BufferSize];
    memset(mybytes, 'A', BufferSize);

    auto buffer = MyVector();
    TRY(buffer.try_resize(BufferSize));
    // The compiler thinks this will overflow. Why?!
    __builtin_memcpy(buffer.data(), mybytes, BufferSize);

    return buffer;
}

ErrorOr<int> serenity_main(Main::Arguments)
{
    auto buf = TRY(make_vector());
    for (size_t i = 0; i < buf.size(); ++i) {
        out("{:02x}", buf[i]);
    }
    outln();
    return 0;
}
