/*
 * Copyright (c) 2022, Luke Wilde <lukew@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibCore/EventLoop.h>
#include <LibWeb/Bindings/MainThreadVM.h>
#include <LibWeb/CSS/Parser/Parser.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/HTML/Window.h>
#include <LibWeb/Platform/EventLoopPlugin.h>

/* LibWeb creates many Timers eagerly, which attempt to register with the EventLoop.
 * This makes sense for the main use case, but not here. */

class DummyTimer final : public Web::Platform::Timer {
public:
    static NonnullRefPtr<DummyTimer> create()
    {
        return adopt_ref(*new DummyTimer);
    }

    virtual ~DummyTimer() = default;

    virtual void start() override { VERIFY_NOT_REACHED(); };
    virtual void start(int) override { VERIFY_NOT_REACHED(); };
    virtual void restart() override { VERIFY_NOT_REACHED(); };
    virtual void restart(int) override { VERIFY_NOT_REACHED(); };
    virtual void stop() override { VERIFY_NOT_REACHED(); };

    virtual void set_active(bool) override { VERIFY_NOT_REACHED(); };
    virtual bool is_active() const override { VERIFY_NOT_REACHED(); };

    virtual int interval() const override { VERIFY_NOT_REACHED(); };
    virtual void set_interval(int) override {};

    virtual bool is_single_shot() const override { VERIFY_NOT_REACHED(); };
    virtual void set_single_shot(bool) override {};

private:
    DummyTimer() = default;
};

class DummyEventLoopPlugin final : public Web::Platform::EventLoopPlugin {
public:
    DummyEventLoopPlugin()
    {
        Web::Platform::EventLoopPlugin::install(*this);
    };
    virtual ~DummyEventLoopPlugin() override
    {
        Web::Platform::EventLoopPlugin::uninstall(*this);
    }

    virtual void spin_until(Function<bool()>) override { VERIFY_NOT_REACHED(); };
    virtual void deferred_invoke(Function<void()>) override { VERIFY_NOT_REACHED(); };
    virtual NonnullRefPtr<Web::Platform::Timer> create_timer() override { return DummyTimer::create(); };
};

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
    Core::EventLoop loop;
    DummyEventLoopPlugin plugin;
    // auto vm = JS::VM::create();
    auto vm = adopt_ref(Web::Bindings::main_thread_vm());
    // auto realm = JS::Realm::create(*vm);
    auto realm = vm->current_realm();
    auto window = Web::HTML::Window::create(*realm);
    auto document = Web::DOM::Document::create(*window);
    (void)Web::parse_css_stylesheet(Web::CSS::Parser::ParsingContext(document), { data, size });
    // Un-do the intentional leak of the JS VM in Web::Bindings::main_thread_vm():
    // vm->unref();
    return 0;
}
