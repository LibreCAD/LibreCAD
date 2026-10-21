/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textprofilebinding.h"

#include <cstdio>
#include <cstdlib>
#include <new>
#include <optional>

// Dedicated executable only: never link these allocator overrides into the
// application or a shared Catch test binary. Arm after all setup/snapshots.
static thread_local bool failNextAllocation = false;
static thread_local unsigned int injectedFailures = 0;
static thread_local std::size_t injectedSize = 0;

void *operator new(std::size_t size) {
    if (failNextAllocation) {
        failNextAllocation = false;
        ++injectedFailures;
        injectedSize = size;
        throw std::bad_alloc();
    }
    if (auto *memory = std::malloc(size ? size : 1)) return memory;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void *memory) noexcept { std::free(memory); }
void operator delete[](void *memory) noexcept { std::free(memory); }
void operator delete(void *memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void *memory, std::size_t) noexcept { std::free(memory); }

namespace {
using Binding = LC_TextProfileBinding;
using Profile = LC_TextProfile;
using Scope = Profile::Scope;
using Authority = Binding::Authority;

Profile::Value value(Scope scope, bool legacy = false) {
    Profile::Selection selection;
    if (!legacy) selection = {Profile::Backend::HarfBuzzOpenType,
        {QStringLiteral("probe-profile"), QByteArray(64, 'a'), 0,
         QStringLiteral("probe-metric"), QStringLiteral("probe-shaping"),
         QStringLiteral("HarfBuzz-OT/14.6.0")}};
    return {scope, selection, QByteArray("ar"), Profile::Direction::Auto};
}
bool equal(const Profile::Error &left, const Profile::Error &right) {
    return left.code == right.code && left.message == right.message && left.field == right.field;
}
bool equal(const Profile::Decoded &left, const Profile::Decoded &right) {
    return left.status == right.status && left.value == right.value && left.raw == right.raw &&
        equal(left.error, right.error);
}
bool equal(const Binding &left, const Binding &right) {
    if (left.scope() != right.scope() || left.authority() != right.authority() ||
        !equal(left.imported(), right.imported()) || !equal(left.current(), right.current()) ||
        !(left.replacement() == right.replacement())) return false;
    const auto *a = left.pendingEncoding();
    const auto *b = right.pendingEncoding();
    return bool(a) == bool(b) && (!a || (a->block == b->block && equal(a->error, b->error)));
}
bool coherent(const Binding &binding) {
    if (binding.authority() != Authority::Replace)
        return !binding.pendingEncoding() && !binding.replacement();
    const auto current = binding.current();
    const auto *encoding = binding.pendingEncoding();
    if (!current.known() || !encoding || encoding->error || !binding.replacement()) return false;
    const auto canonical = Profile::encode(*current.value);
    return canonical && canonical.block == encoding->block && current.value == binding.replacement();
}
} // namespace

int main() {
    bool allPreserved = true;
    for (bool engaged : {false, true}) {
        const auto sourceValue = value(Scope::StyleDefaults);
        Binding source(Scope::StyleDefaults, Profile::encode(sourceValue).block);
        if (source.replace(sourceValue)) return 90;
        Binding target(Scope::EntityOverride);
        if (engaged && target.replace(value(Scope::EntityOverride, true))) return 91;
        const Binding sourceBefore(source), targetBefore(target);
        const auto *encodingBefore = target.pendingEncoding();
        bool threw = false;
        failNextAllocation = true;
        try { target = source; }
        catch (const std::bad_alloc &) { threw = true; }
        failNextAllocation = false;
        const bool sourcePreserved = equal(source, sourceBefore);
        const bool targetPreserved = equal(target, targetBefore) && target.pendingEncoding() == encodingBefore;
        const bool stateCoherent = coherent(target);
        // Guard the observed allocation site on each tested toolchain. Do not
        // count a different allocation or an unexecuted injection as coverage.
        const auto vectorBytes = source.pendingEncoding()->block.size() * sizeof(Profile::Tag);
        std::printf("target=%s threw=%d injected=%u allocation=%zu vector_bytes=%zu source_preserved=%d target_preserved=%d coherent=%d\n",
            engaged ? "Replace" : "Untouched", int(threw), injectedFailures, injectedSize, vectorBytes,
            int(sourcePreserved), int(targetPreserved), int(stateCoherent));
        allPreserved = allPreserved && threw && injectedSize == vectorBytes &&
            sourcePreserved && targetPreserved && stateCoherent;
    }
    return allPreserved && injectedFailures == 2 ? 0 : 2;
}
