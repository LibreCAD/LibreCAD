/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#ifndef LC_TRACKEDLISTENER_H
#define LC_TRACKEDLISTENER_H

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

template <class Listener, class Source>
class LC_ListenerList;

/**
 * Base of a listener interface whose sources (a layer list, a block list, a UCS list, a list of
 * named views, a selection) keep raw pointers to their listeners.
 *
 * A listener that is destroyed while it is still listed would leave a dangling pointer in the
 * source, called by its next notification, and a source that is destroyed first leaves the listener
 * with a dangling pointer to it. Here each side knows the other: a source that gets a listener
 * (through LC_ListenerList) is recorded in the listener, forgotten when the listener is removed or
 * the source is destroyed, and a listener that is destroyed removes itself from every source that
 * still has it. Neither order of destruction leaves anything dangling, whatever the derived class
 * does. (What is not covered: a listener destroyed by a callback while its source loops over its
 * listeners to notify them, which unlists it under the loop: no callback in the tree does that.)
 *
 * This is only about the listener and the source finding each other. A listener that keeps a pointer
 * to the source (a dock's list pointer, and rows for the items) still has to be told that the source
 * is going away, and forget them: see the *Destroyed() callback of each listener interface.
 *
 * \p Listener is the interface that derives from this (it only tells the families apart: a class that
 * listens to several has one base of each). Nothing here needs the source type to be complete: the
 * source hands over a function that removes a listener from it.
 */
template <class Listener>
class LC_TrackedListener {
public:
    LC_TrackedListener() = default;
    /// a copy is listed nowhere
    LC_TrackedListener(const LC_TrackedListener&) noexcept {}
    LC_TrackedListener& operator=(const LC_TrackedListener&) noexcept { return *this; }

    /// Removes this listener from every source that still lists it. It only compares pointers: the
    /// derived part is gone by now, so nothing is called on it.
    ~LC_TrackedListener() {
        const std::vector<Registration> registrations = std::exchange(m_registrations, {});
        for (const Registration& registration : registrations) {
            registration.m_remove(registration.m_source, m_self);
        }
    }

    /// how many sources list this listener
    [[nodiscard]] std::size_t sourceCount() const {
        return m_registrations.size();
    }

private:
    template <class L, class S>
    friend class LC_ListenerList;

    struct Registration {
        void* m_source;
        void (*m_remove)(void* source, Listener* listener);
    };

    std::vector<Registration> m_registrations;
    /// the listener this base belongs to: set when it is listed, so that the destructor needs no cast
    Listener* m_self = nullptr;
};

/**
 * The listeners of a source, kept by the source as a member, constructed with the source as its
 * argument (a default member initializer {this}). The source's addListener() and removeListener()
 * forward to add() and remove(), and only the source's destructor should call drain().
 *
 * \p Source needs a public removeListener(Listener*) that calls remove() here, and only has to be
 * complete where add() is used.
 */
template <class Listener, class Source>
class LC_ListenerList {
public:
    explicit LC_ListenerList(Source* owner) : m_owner{owner} {}
    LC_ListenerList(const LC_ListenerList&) = delete;
    LC_ListenerList& operator=(const LC_ListenerList&) = delete;

    /// Lists no listener any more: a source that did not drain them is going away, and they must not
    /// keep a pointer to it.
    ~LC_ListenerList() {
        while (!m_listeners.empty()) {
            remove(m_listeners.back());
        }
    }

    /// Lists \p listener once (nothing if it is null or listed already).
    void add(Listener* listener) {
        if (listener == nullptr || contains(listener)) {
            return;
        }
        Tracked& tracked = static_cast<Tracked&>(*listener);
        tracked.m_self = listener;
        tracked.m_registrations.push_back({m_owner, &removeFrom});
        m_listeners.push_back(listener);
        m_tracked.push_back(&tracked);
    }

    /// Unlists \p listener. \return whether it was listed.
    bool remove(Listener* listener) {
        const auto found = std::find(m_listeners.begin(), m_listeners.end(), listener);
        if (found == m_listeners.end()) {
            return false;
        }
        const auto index = found - m_listeners.begin();
        Tracked* tracked = m_tracked[static_cast<std::size_t>(index)];
        m_listeners.erase(found);
        m_tracked.erase(m_tracked.begin() + index);
        std::vector<typename Tracked::Registration>& registrations = tracked->m_registrations;
        registrations.erase(std::remove_if(registrations.begin(), registrations.end(),
                                           [this](const typename Tracked::Registration& registration) {
                                               return registration.m_source == m_owner;
                                           }),
                            registrations.end());
        return true;
    }

    /**
     * Unlists every listener in turn, oldest first, and calls \p callback with it, once it is no
     * longer listed: a callback that deletes another listener (which removes itself from here) or
     * unregisters itself finds the list current, and no listener is skipped or called twice.
     */
    template <class Callback>
    void drain(Callback&& callback) {
        while (!m_listeners.empty()) {
            Listener* listener = m_listeners.front();
            remove(listener);
            callback(listener);
        }
    }

    [[nodiscard]] bool contains(const Listener* listener) const {
        return std::find(m_listeners.begin(), m_listeners.end(), listener) != m_listeners.end();
    }
    [[nodiscard]] std::size_t size() const {
        return m_listeners.size();
    }
    [[nodiscard]] bool empty() const {
        return m_listeners.empty();
    }

    /// to loop over the listeners, as over the list they replace
    [[nodiscard]] typename std::vector<Listener*>::const_iterator begin() const {
        return m_listeners.begin();
    }
    [[nodiscard]] typename std::vector<Listener*>::const_iterator end() const {
        return m_listeners.end();
    }
    [[nodiscard]] typename std::vector<Listener*>::const_iterator begin() {
        return static_cast<const std::vector<Listener*>&>(m_listeners).begin();
    }
    [[nodiscard]] typename std::vector<Listener*>::const_iterator end() {
        return static_cast<const std::vector<Listener*>&>(m_listeners).end();
    }

private:
    using Tracked = LC_TrackedListener<Listener>;

    static void removeFrom(void* source, Listener* listener) {
        static_cast<Source*>(source)->removeListener(listener);
    }

    Source* m_owner;
    std::vector<Listener*> m_listeners;
    /// the tracking base of each listener, in the same order (taken while the listener was alive)
    std::vector<Tracked*> m_tracked;
};

#endif
