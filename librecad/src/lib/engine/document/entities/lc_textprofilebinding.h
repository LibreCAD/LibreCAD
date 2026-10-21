/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#ifndef LC_TEXTPROFILEBINDING_H
#define LC_TEXTPROFILEBINDING_H

#include "lc_textprofile.h"

/** Explicit edit authority over one provisional LibreCADText namespace block.
 * Import and edits are separate: imported provenance is never rewritten by an
 * edit or reset. No fonts, registry, host entity, filter or activation policy is
 * involved. The owning host must still enforce schema/transport/runtime gates.
 *
 * Scope is locked for in-place edits. Ordinary whole-value copy/move assignment
 * replaces the entire binding, including scope and provenance; an entity/style
 * setter accepting a binding must enforce its own required scope. Instances are
 * value objects, not thread-safe mutable shared state. Inputs must not be mutated
 * concurrently with construction or replace.
 */
class LC_TextProfileBinding final {
public:
    using Profile = LC_TextProfile;
    using Scope = Profile::Scope;
    enum class Authority { Untouched, Replace, Remove };

    explicit LC_TextProfileBinding(Scope scope);
    // Owning import domain: QString, QByteArray, Int, UInt, LongLong,
    // ULongLong, Double, Bool and invalid QVariant. Values, exact types,
    // nullness and floating storage are preserved, even for invalid/opaque or
    // over-limit blocks. QString/QByteArray buffers are copied, not shared with
    // input. Unsupported metatypes throw std::invalid_argument before a binding
    // is constructed; input is untouched. There is no arbitrary Qt metatype
    // deep-ownership guarantee. A transport bridge must diagnose and retain
    // unsupported raw outside this binding, never drop it or leak the exception.
    LC_TextProfileBinding(Scope scope, const Profile::Block &importedBlock);
    // Same owning domain, including for a const shared block: a caller may
    // retain mutable block/buffer aliases. A null pointer denotes absent raw.
    // Copies of this binding share only its own immutable imported snapshot.
    // Ordinary pre-call conversions still apply: converting a mutable shared_ptr
    // rvalue to shared_ptr<const Block> can consume that pointer before entry.
    LC_TextProfileBinding(Scope scope, const std::shared_ptr<const Profile::Block> &importedBlock);
    // Nonthrowing type-domain preflight, NOT wire validation/activation. False
    // means the caller must preserve/handle the original raw outside this API.
    static bool supportsImport(const Profile::Block &block) noexcept;
    LC_TextProfileBinding(const LC_TextProfileBinding &) = default;
    // Moves transfer pending authority but COPY immutable imported provenance.
    // The source retains its scope/import and becomes Untouched, fully usable.
    // Self-move assignment preserves the complete state without resetting it.
    LC_TextProfileBinding(LC_TextProfileBinding &&other);
    // Whole-value assignment prepares a complete incoming state, then commits
    // without throwing. A failed copy leaves every destination field unchanged.
    // Self-copy and self-move are no-ops, including pending encoding pointers.
    LC_TextProfileBinding &operator=(const LC_TextProfileBinding &other);
    LC_TextProfileBinding &operator=(LC_TextProfileBinding &&other);

    Scope scope() const { return m_scope; }
    Authority authority() const { return m_authority; }
    // Sole raw provenance authority. Unknown, invalid, wrong-scope and over-limit
    // imports remain retained here. There is intentionally no re-import mutator.
    const Profile::Decoded &imported() const { return m_imported; }
    // Modeled metadata for inheritance, NOT a raw replay instruction. raw is
    // always null. Untouched mirrors import status/value/error; Replace is Known;
    // Remove is Absent even if an unknown/invalid namespace was imported.
    Profile::Decoded current() const;
    const std::optional<Profile::Value> &replacement() const { return m_replacement; }
    // Only an explicit successful Replace produces canonical encoding. nullptr
    // under Untouched or Remove: neither requests an empty replacement block.
    // Reading does not acknowledge a save or reset authority. The pointer is
    // valid only until the next edit/reset or whole-value assignment.
    const Profile::Encoded *pendingEncoding() const;

    // Deep-own typed buffers, then validate matching scope and encode that owned
    // snapshot before changing state. Retained input data pointers cannot mutate
    // an accepted edit. No concurrent input mutation is supported at a boundary.
    // Errors preserve the previous pending edit AND imported provenance. Known
    // wrong-scope imports do not prevent an explicit correctly scoped replacement.
    Profile::Error replace(const Profile::Value &value);
    // Explicit removal of this namespace, not absence of an edit/inheritance.
    // The service never mutates unrelated application data or the imported block.
    Profile::Error remove();
    // Discard only the pending edit; restore the modeled imported metadata.
    void reset();

private:
    void swapState(LC_TextProfileBinding &other) noexcept;
    Scope m_scope;
    Profile::Decoded m_imported;
    Authority m_authority = Authority::Untouched;
    std::optional<Profile::Value> m_replacement;
    std::optional<Profile::Encoded> m_encoding;
};

#endif
