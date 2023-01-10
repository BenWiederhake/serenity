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
    auto buffer = MyVector();
    TRY(buffer.try_resize(BufferSize));
    // The compiler thinks this will overflow. Why?!
    memset(buffer.data(), 'A', buffer.size());

    // Uncomment the following outln() and it magically works, *and* shows that it uses the correct buffer. WTF?!
    outln("buffer is at {:p}, buffer.data()={:p}, buffer.capacity()={}, buffer.size()={}",
        &buffer, buffer.data(), buffer.capacity(), buffer.size());

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
