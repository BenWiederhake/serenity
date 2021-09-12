/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibGUI/Widget.h>

namespace HackStudio {

class Locator final : public GUI::Widget {
    C_OBJECT(Locator)
public:
    virtual ~Locator() override;

    void open();
    void close();

private:
    void update_suggestions();
    void open_suggestion(const GUI::ModelIndex&);

    Locator(Core::Object* parent = nullptr);

    RefPtr<GUI::TextBox> m_textbox;
    RefPtr<GUI::Window> m_popup_window;
    RefPtr<GUI::TableView> m_suggestion_view;
};

}
