/*
 * Copyright (c) 2021, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stddef.h>
#include <stdint.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    auto* decoder = TextCodec::decoder_for("windows-1251");
    VERIFY(decoder);
    decoder->to_utf8({ data, size });
    return 0;
}
