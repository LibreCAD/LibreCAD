/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#ifndef LC_TEXTSOURCECODEC_H
#define LC_TEXTSOURCECODEC_H

#include "lc_textdocument.h"

/** Lossless raw-source caret-convention conversion, not generic text decoding.
 * Formatting is retained, not reconstructed from logical text. Same-convention
 * values are copied exactly, including nullness, malformed UTF-16/UTF-8 and NUL.
 * Cross-convention conversion follows LC_TextDocument's explicit conventions;
 * it neither activates profiles nor certifies another CAD reader's behavior.
 */
class LC_TextSourceCodec final {
public:
    struct Options {
        LC_TextDocument::Syntax syntax = LC_TextDocument::Syntax::Plain;
        bool sourceCaretCodes = false;
        bool targetCaretCodes = false;
        // This policy is identical on both sides. Unicode/%% spellings are
        // retained; conversion never changes their decoding convention.
        bool decodeDxfEscapes = true;
        int maxSourceUnits = 1024 * 1024;
        int maxOutputUnits = 2 * 1024 * 1024;
        // Byte fast paths enforce byte budgets without decoding storage.
        // Cross conversion additionally enforces the UTF-16 budgets above.
        int maxSourceBytes = 4 * 1024 * 1024;
        int maxOutputBytes = 8 * 1024 * 1024;
        int maxScopeDepth = 64;
        int maxStackDepth = 16; // Hard safety ceiling: 64.
        int maxDiagnostics = 1024;
        int maxTraversalNodes = 4 * 1024 * 1024;
    };
    enum class ErrorCode { None, InvalidOptions, ResourceLimit, AmbiguousSyntax, InvalidUtf8, SemanticMismatch };
    struct Error {
        ErrorCode code = ErrorCode::None;
        // UTF-16 input source range, also for decoded UTF-8. Empty when input
        // could not be decoded or the error concerns a whole-input budget.
        LC_TextDocument::Range source;
        QString message;
        explicit operator bool() const { return code != ErrorCode::None; }
    };
    struct Result {
        std::optional<QString> source;
        Error error;
        bool changed = false;
        explicit operator bool() const { return source.has_value() && !error; }
    };
    struct ByteResult {
        std::optional<QByteArray> source;
        Error error;
        bool changed = false;
        explicit operator bool() const { return source.has_value() && !error; }
    };
    // Errors are atomic: no successful partial output, and input is untouched.
    // Unknown/malformed sensitive arguments fail rather than being guessed.
    static Result transcode(const QString &, const Options &);
    // Explicit-length UTF-8. Invalid UTF-8 is retained across a convention
    // change only if conservative ASCII preflight proves no affected caret/C0.
    static ByteResult transcode(const QByteArray &, const Options &);
};

#endif
