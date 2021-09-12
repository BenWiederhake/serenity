/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibGfx/Forward.h>
#include <LibWeb/HTML/HTMLElement.h>
#include <LibWeb/Loader/ImageLoader.h>

namespace Web::HTML {

class HTMLImageElement final : public HTMLElement {
public:
    using WrapperType = Bindings::HTMLImageElementWrapper;

    HTMLImageElement(DOM::Document&, QualifiedName);
    virtual ~HTMLImageElement() override;

    virtual void parse_attribute(const FlyString& name, const String& value) override;

    String alt() const { return attribute(HTML::AttributeNames::alt); }
    String src() const { return attribute(HTML::AttributeNames::src); }

    const Gfx::Bitmap* bitmap() const;

private:
    virtual void apply_presentational_hints(CSS::StyleProperties&) const override;

    void animate();

    virtual RefPtr<Layout::Node> create_layout_node() override;

    ImageLoader m_image_loader;
};

}
