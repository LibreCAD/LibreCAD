/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

// Issue #2969: the layer, block, UCS and named-view lists and the selection keep raw pointers to their
// listeners. A listener that is destroyed while it is listed must not be called later, and a source
// that is destroyed first must not leave the listener with a dangling pointer to it: LC_TrackedListener
// lets each side find the other, for every one of the five families. Run under ASan, the tests fail on
// a dangling pointer of either kind.

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <memory>

#include "lc_selectedset.h"
#include "lc_ucslist.h"
#include "lc_viewslist.h"
#include "rs_blocklist.h"
#include "rs_blocklistlistener.h"
#include "rs_layerlist.h"
#include "rs_layerlistlistener.h"

namespace {

/// a listener of one family that counts what it hears when its source is destroyed
template <class Interface>
class Recording : public Interface {
public:
    /// what to do when told: a listener that unregisters itself, deletes another one, ...
    std::function<void()> m_hook;
    int m_destroyed = 0;
    /// how many sources still listed this listener when it was told
    std::size_t m_sourcesWhenTold = 99;
    bool m_sourceEmptyWhenTold = false;
    std::function<bool()> m_isSourceEmpty;

    void told() {
        ++m_destroyed;
        m_sourcesWhenTold = this->sourceCount();
        if (m_isSourceEmpty) {
            m_sourceEmptyWhenTold = m_isSourceEmpty();
        }
        if (m_hook) {
            m_hook();
        }
    }
};

struct LayerFamily {
    using Source = RS_LayerList;
    struct Listener final : Recording<RS_LayerListListener> {
        void layerListDestroyed() override { told(); }
    };
    static std::unique_ptr<Source> make() { return std::make_unique<RS_LayerList>(); }
    static bool isEmpty(const Source& source) { return source.count() == 0; }
};

struct BlockFamily {
    using Source = RS_BlockList;
    struct Listener final : Recording<RS_BlockListListener> {
        void blockListDestroyed() override { told(); }
    };
    static std::unique_ptr<Source> make() { return std::make_unique<RS_BlockList>(true); }
    static bool isEmpty(const Source& source) { return source.count() == 0; }
};

struct UcsFamily {
    using Source = LC_UCSList;
    struct Listener final : Recording<LC_UCSListListener> {
        void ucsListDestroyed() override { told(); }
    };
    static std::unique_ptr<Source> make() { return std::make_unique<LC_UCSList>(); }
    static bool isEmpty(const Source& source) { return source.count() == 0; }
};

struct ViewFamily {
    using Source = LC_ViewList;
    struct Listener final : Recording<LC_ViewListListener> {
        void viewsListDestroyed() override { told(); }
    };
    static std::unique_ptr<Source> make() { return std::make_unique<LC_ViewList>(); }
    static bool isEmpty(const Source& source) { return source.count() == 0; }
};

struct SelectionFamily {
    using Source = LC_SelectedSet;
    struct Listener final : Recording<LC_SelectedSetListener> {
        void selectionChanged() override {}
        void selectedSetDestroyed() override { told(); }
    };
    static std::unique_ptr<Source> make() { return std::make_unique<LC_SelectedSet>(); }
    static bool isEmpty(Source& source) { return !source.hasSelection(); }
};

}

TEMPLATE_TEST_CASE("A listener destroyed before its source is not called by it", "[tracked][listener][2969]",
                   LayerFamily, BlockFamily, UcsFamily, ViewFamily, SelectionFamily) {
    auto source = TestType::make();
    auto listener = std::make_unique<typename TestType::Listener>();
    source->addListener(listener.get());
    REQUIRE(source->listenerCount() == 1);
    REQUIRE(listener->sourceCount() == 1);

    listener.reset();
    CHECK(source->listenerCount() == 0);
    // the source is destroyed with nobody listed (under ASan: nobody dangling)
    source.reset();
}

TEMPLATE_TEST_CASE("A source destroyed before its listener leaves the listener listed nowhere", "[tracked][listener][2969]",
                   LayerFamily, BlockFamily, UcsFamily, ViewFamily, SelectionFamily) {
    auto source = TestType::make();
    typename TestType::Listener listener;
    source->addListener(&listener);
    REQUIRE(listener.sourceCount() == 1);
    // (the source is read from its own destructor here, as a test: a listener must not)
    typename TestType::Source* raw = source.get();
    listener.m_isSourceEmpty = [raw] { return TestType::isEmpty(*raw); };

    source.reset();
    // told once, when it was already unregistered and the source was empty
    CHECK(listener.m_destroyed == 1);
    CHECK(listener.m_sourcesWhenTold == 0);
    CHECK(listener.m_sourceEmptyWhenTold);
    CHECK(listener.sourceCount() == 0);
    // (the listener is destroyed after the source, by the end of the test)
}

TEMPLATE_TEST_CASE("A listener is listed once, and can unregister", "[tracked][listener][2969]",
                   LayerFamily, BlockFamily, UcsFamily, ViewFamily, SelectionFamily) {
    auto source = TestType::make();
    typename TestType::Listener listener;
    source->addListener(&listener);
    source->addListener(&listener);
    CHECK(source->listenerCount() == 1);
    CHECK(listener.sourceCount() == 1);
    source->addListener(nullptr);
    CHECK(source->listenerCount() == 1);

    source->removeListener(&listener);
    CHECK(source->listenerCount() == 0);
    CHECK(listener.sourceCount() == 0);
    source->removeListener(&listener); // not listed: nothing
    source.reset();
    CHECK(listener.m_destroyed == 0);
}

TEMPLATE_TEST_CASE("A listener on two sources is unregistered from the one that is left", "[tracked][listener][2969]",
                   LayerFamily, BlockFamily, UcsFamily, ViewFamily, SelectionFamily) {
    auto first = TestType::make();
    auto second = TestType::make();
    auto listener = std::make_unique<typename TestType::Listener>();
    first->addListener(listener.get());
    second->addListener(listener.get());
    REQUIRE(listener->sourceCount() == 2);

    first.reset();
    CHECK(listener->m_destroyed == 1);
    CHECK(listener->sourceCount() == 1);
    CHECK(second->listenerCount() == 1);

    listener.reset();
    CHECK(second->listenerCount() == 0);
}

TEMPLATE_TEST_CASE("A copy of a listener is listed nowhere", "[tracked][listener][2969]",
                   LayerFamily, BlockFamily, UcsFamily, ViewFamily, SelectionFamily) {
    auto source = TestType::make();
    typename TestType::Listener listener;
    source->addListener(&listener);
    {
        typename TestType::Listener copy(listener);
        CHECK(copy.sourceCount() == 0);
        typename TestType::Listener assigned;
        assigned = listener;
        CHECK(assigned.sourceCount() == 0);
    }
    // destroying the copies removed nothing of the original
    CHECK(source->listenerCount() == 1);
    CHECK(listener.sourceCount() == 1);
}

TEMPLATE_TEST_CASE("A listener that unregisters itself when told does not make another miss or repeat", "[tracked][listener][2969]",
                   LayerFamily, BlockFamily, UcsFamily, ViewFamily, SelectionFamily) {
    auto source = TestType::make();
    typename TestType::Listener before;
    typename TestType::Listener removing;
    typename TestType::Listener afterOne;
    typename TestType::Listener afterTwo;
    // (through the raw pointer: a unique_ptr is null while its deleter runs)
    typename TestType::Source* raw = source.get();
    removing.m_hook = [&] { raw->removeListener(&removing); };
    for (auto* listener : {&before, &removing, &afterOne, &afterTwo}) {
        source->addListener(listener);
    }
    source.reset();
    CHECK(before.m_destroyed == 1);
    CHECK(removing.m_destroyed == 1);
    CHECK(afterOne.m_destroyed == 1);
    CHECK(afterTwo.m_destroyed == 1);
}

TEMPLATE_TEST_CASE("A listener that deletes another one when told: the other is not called", "[tracked][listener][2969]",
                   LayerFamily, BlockFamily, UcsFamily, ViewFamily, SelectionFamily) {
    auto source = TestType::make();
    typename TestType::Listener first;
    auto victim = std::make_unique<typename TestType::Listener>();
    typename TestType::Listener last;
    first.m_hook = [&victim] { victim.reset(); };
    source->addListener(&first);
    source->addListener(victim.get());
    source->addListener(&last);
    source.reset();
    CHECK(first.m_destroyed == 1);
    CHECK(last.m_destroyed == 1);
    CHECK(victim == nullptr);
}
