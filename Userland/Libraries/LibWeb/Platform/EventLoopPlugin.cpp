/*
 * Copyright (c) 2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Format.h>
#include <AK/Function.h>
#include <LibWeb/Platform/EventLoopPlugin.h>

namespace Web::Platform {

EventLoopPlugin* s_the;

EventLoopPlugin& EventLoopPlugin::the()
{
    VERIFY(s_the);
    return *s_the;
}

void EventLoopPlugin::install(EventLoopPlugin& plugin)
{
    VERIFY(!s_the);
    s_the = &plugin;
}

void EventLoopPlugin::uninstall(EventLoopPlugin& plugin)
{
    VERIFY(s_the == &plugin);
    s_the = nullptr;
}

EventLoopPlugin::~EventLoopPlugin() = default;

}
