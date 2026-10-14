/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textprofile.h"

#include <QMetaType>
#include <map>
#include <set>
#include <utility>

namespace {
using Profile = LC_TextProfile;
using Code = Profile::ErrorCode;
Profile::Error error(Code code, const char *message, QString field = {}) {
    return {code, QString::fromLatin1(message), std::move(field)};
}
bool alpha(unsigned char ch) { return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'); }
bool digit(unsigned char ch) { return ch >= '0' && ch <= '9'; }
bool token(const QString &text) {
    if (text.isEmpty() || text.size() > 127 || text == "." || text == "..") return false;
    for (qsizetype i = 0; i < text.size(); ++i) {
        const auto ch = text.at(i).unicode();
        if (ch > 127 || !(alpha(ch) || digit(ch) || (i > 0 && (ch == '-' || ch == '_' || ch == '.' || ch == '+')))) return false;
    }
    return true;
}
bool engine(const QString &text) {
    if (text.size() > 127) return false;
    const auto slash = text.indexOf('/');
    return slash < 0 ? token(text) : text.indexOf('/', slash + 1) < 0 &&
        token(text.left(slash)) && token(text.mid(slash + 1));
}
bool language(const QByteArray &text) {
    if (text.isEmpty() || text.size() > 63) return false;
    int segment = 0;
    bool first = true;
    for (unsigned char ch : text) {
        if (ch == '-') {
            if (segment == 0) return false;
            segment = 0; first = false;
        } else {
            if (!(alpha(ch) || (!first && digit(ch))) || ++segment > 8) return false;
        }
    }
    return segment > 0;
}
bool hash(const QByteArray &text) {
    if (text.size() != 64) return false;
    for (unsigned char ch : text) if (!(digit(ch) || (ch >= 'a' && ch <= 'f'))) return false;
    return true;
}
bool emptyIdentity(const Profile::Identity &id) {
    return id.profileId.isEmpty() && id.fontSha256.isEmpty() && id.faceIndex == 0 &&
           id.metricPolicy.isEmpty() && id.shapingPolicy.isEmpty() && id.engineIdentity.isEmpty();
}
bool validUtf16(const QString &text) {
    for (qsizetype i = 0; i < text.size(); ++i) {
        if (text.at(i).isHighSurrogate()) {
            if (++i >= text.size() || !text.at(i).isLowSurrogate()) return false;
        } else if (text.at(i).isLowSurrogate()) return false;
    }
    return true;
}
Profile::Error blockBounds(const Profile::Block &block) {
    if (block.size() > Profile::MaximumTags) return error(Code::ComplexityLimit, "The text-profile tag limit was exceeded");
    std::size_t bytes = 0;
    for (const auto &tag : block) {
        std::size_t size = 0;
        const auto type = tag.value.metaType().id();
        if (type == QMetaType::QString) {
            const auto text = tag.value.toString();
            if (text.size() > Profile::MaximumStringBytes)
                return error(Code::ComplexityLimit, "A text-profile string exceeds the portable byte limit");
            if (text.contains(QChar(0)) || !validUtf16(text))
                return error(Code::InvalidField, "A text-profile string contains NUL or malformed UTF-16");
            size = std::size_t(text.toUtf8().size());
            if (size > Profile::MaximumStringBytes)
                return error(Code::ComplexityLimit, "A text-profile string exceeds the portable byte limit");
            size += 1;
        } else if (type == QMetaType::QByteArray) {
            const auto data = tag.value.toByteArray();
            if (data.size() > Profile::MaximumStringBytes)
                return error(Code::ComplexityLimit, "An opaque text-profile byte field exceeds the retention budget");
            size = std::size_t(data.size()) + 1;
        } else if (type == QMetaType::Int || type == QMetaType::UInt || type == QMetaType::LongLong ||
                   type == QMetaType::ULongLong || type == QMetaType::Double || type == QMetaType::Bool) {
            size = 8; // Conservative fixed-scalar accounting; 1070 itself uses 2.
        } else {
            return error(Code::InvalidType, "The isolated text-profile block contains an unsupported value type");
        }
        bytes += 2 + size;
        if (bytes > Profile::MaximumBlockBytes)
            return error(Code::ComplexityLimit, "The text-profile block byte budget was exceeded");
    }
    return {};
}
bool shortInteger(const Profile::Tag &tag, int &value) {
    if (tag.code != 1070 || tag.value.metaType().id() != QMetaType::Int) return false;
    value = tag.value.toInt();
    return value >= -32768 && value <= 32767;
}
Profile::Decoded invalid(Profile::Decoded result, Profile::Error issue) {
    result.status = Profile::DecodeStatus::Invalid;
    result.value.reset(); result.error = std::move(issue);
    return result;
}
Profile::Decoded opaque(Profile::Decoded result, Profile::Error issue) {
    result.status = Profile::DecodeStatus::Opaque;
    result.value.reset(); result.error = std::move(issue);
    return result;
}
Profile::Decoded modeled(const std::optional<Profile::Value> &value) {
    Profile::Decoded result;
    if (!value) return result;
    result.value = value;
    result.error = Profile::validate(*value);
    result.status = result.error ? Profile::DecodeStatus::Invalid : Profile::DecodeStatus::Known;
    return result;
}
} // namespace

QString LC_TextProfile::applicationName() { return QStringLiteral("LibreCADText"); }
bool LC_TextProfile::Identity::operator==(const Identity &other) const {
    return profileId == other.profileId && fontSha256 == other.fontSha256 && faceIndex == other.faceIndex &&
           metricPolicy == other.metricPolicy && shapingPolicy == other.shapingPolicy && engineIdentity == other.engineIdentity;
}
bool LC_TextProfile::Selection::operator==(const Selection &other) const { return backend == other.backend && identity == other.identity; }
bool LC_TextProfile::Value::operator==(const Value &other) const {
    return scope == other.scope && selection == other.selection && language == other.language && direction == other.direction;
}
bool LC_TextProfile::Tag::operator==(const Tag &other) const {
    return code == other.code && value.metaType() == other.value.metaType() && value == other.value;
}

LC_TextProfile::Error LC_TextProfile::validate(const Value &value) {
    if (value.scope != Scope::StyleDefaults && value.scope != Scope::EntityOverride)
        return error(Code::InvalidScope, "The text-profile scope is invalid", "scope");
    if (value.scope == Scope::StyleDefaults && (!value.selection || !value.language || !value.direction))
        return error(Code::MissingField, "Style defaults require explicit selection, language and direction");
    if (value.direction) {
        switch (*value.direction) {
        case Direction::Auto: case Direction::LeftToRight: case Direction::RightToLeft: break;
        case Direction::ByStyle:
            if (value.scope == Scope::StyleDefaults)
                return error(Code::InvalidField, "Style defaults cannot recursively inherit their own direction", "direction");
            break;
        default: return error(Code::InvalidField, "The direction enum is invalid", "direction");
        }
    }
    if (value.language && !language(*value.language))
        return error(Code::InvalidField, "Language must use bounded ASCII BCP47-shaped subtags", "language");
    if (value.selection) {
        const auto &selection = *value.selection;
        const auto &id = selection.identity;
        if (selection.backend == Backend::LegacyLff) {
            if (!emptyIdentity(id)) return error(Code::InvalidField, "A legacy opt-out cannot carry an OpenType identity", "backend");
        } else if (selection.backend == Backend::HarfBuzzOpenType) {
            if (!token(id.profileId)) return error(Code::InvalidField, "The profile identifier is not a bounded token", "profile");
            if (!hash(id.fontSha256)) return error(Code::InvalidField, "The font checksum must contain 64 lowercase hexadecimal digits", "sha256");
            if (id.faceIndex > 32767) return error(Code::InvalidField, "The face index exceeds the portable signed 1070 range", "face");
            if (!token(id.metricPolicy)) return error(Code::InvalidField, "The metric policy is not a bounded token", "metrics");
            if (!token(id.shapingPolicy)) return error(Code::InvalidField, "The shaping policy is not a bounded token", "shaping");
            if (!engine(id.engineIdentity)) return error(Code::InvalidField, "The engine identifier is invalid or path-like", "engine");
        } else return error(Code::InvalidField, "The backend enum is invalid", "backend");
    }
    return {};
}

LC_TextProfile::Decoded LC_TextProfile::decode(Block block) {
    return decode(std::make_shared<const Block>(std::move(block)));
}
LC_TextProfile::Decoded LC_TextProfile::decode(std::shared_ptr<const Block> block) {
    Decoded result;
    result.raw = std::move(block);
    if (!result.raw || result.raw->empty()) return result;
    const auto &tags = *result.raw;
    if (auto issue = blockBounds(tags)) return invalid(std::move(result), std::move(issue));
    if (tags.front().code != 1001 || tags.front().value.metaType().id() != QMetaType::QString ||
        tags.front().value.toString() != applicationName())
        return invalid(std::move(result), error(Code::InvalidNamespace, "The codec requires one isolated LibreCADText APPID block"));
    for (std::size_t i = 1; i < tags.size(); ++i)
        if (tags[i].code == 1001)
            return invalid(std::move(result), error(Code::InvalidNamespace, "A second APPID block must remain outside this isolated codec"));
    int schema = 0;
    if (tags.size() < 2 || !shortInteger(tags[1], schema) || schema <= 0)
        return invalid(std::move(result), error(Code::InvalidSchema, "The profile schema requires a positive signed 1070 integer"));
    if (schema != SchemaVersion)
        return opaque(std::move(result), error(Code::UnknownSchema, "The unknown text-profile schema was retained without activation"));

    std::map<QString, QString> strings;
    std::map<QString, int> integers;
    std::set<QString> fields;
    bool unknown = false;
    for (std::size_t i = 2; i < tags.size(); ++i) {
        const auto &tag = tags[i];
        if (tag.code != 1000 || tag.value.metaType().id() != QMetaType::QString) {
            // Unknown XDATA codes are unsupported semantics, not permission to
            // interpret a real/binary/1071 value as a recognized field.
            if (tag.code != 1000 && tag.code != 1070) { unknown = true; continue; }
            return invalid(std::move(result), error(Code::InvalidType, "A profile field must be a 1000 string or a keyed 1070 integer"));
        }
        const auto text = tag.value.toString();
        const auto equals = text.indexOf('=');
        const auto key = equals < 0 ? text : text.left(equals);
        if (!token(key)) return invalid(std::move(result), error(Code::InvalidField, "The profile field name is invalid"));
        if (!fields.insert(key).second)
            return invalid(std::move(result), error(Code::DuplicateField, "A duplicate profile field was retained without activation", key));
        if (key == "face" || key == "direction") {
            int number = 0;
            if (equals >= 0 || i + 1 >= tags.size() || !shortInteger(tags[++i], number))
                return invalid(std::move(result), error(Code::InvalidType, "The keyed profile field requires a signed 1070 integer", key));
            integers.emplace(key, number);
        } else if (key == "scope" || key == "backend" || key == "profile" || key == "sha256" ||
                   key == "metrics" || key == "shaping" || key == "engine" || key == "language") {
            if (equals < 0) return invalid(std::move(result), error(Code::InvalidType, "The recognized profile field requires a string value", key));
            strings.emplace(key, text.mid(equals + 1));
        } else {
            unknown = true;
            if (equals < 0 && i + 1 < tags.size() && tags[i + 1].code == 1070) ++i;
        }
    }
    if (unknown) return opaque(std::move(result), error(Code::UnknownField, "Unknown profile fields were retained without activation"));
    Value value;
    const auto scope = strings.find("scope");
    if (scope == strings.end()) return invalid(std::move(result), error(Code::MissingField, "The profile scope is required", "scope"));
    if (scope->second == "style-default") value.scope = Scope::StyleDefaults;
    else if (scope->second == "entity-override") value.scope = Scope::EntityOverride;
    else return invalid(std::move(result), error(Code::InvalidScope, "The profile scope string is invalid", "scope"));
    const auto backend = strings.find("backend");
    const bool identityPresent = strings.count("profile") || strings.count("sha256") || integers.count("face") ||
        strings.count("metrics") || strings.count("shaping") || strings.count("engine");
    if (backend == strings.end()) {
        if (identityPresent) return invalid(std::move(result), error(Code::MissingField, "A partial selection cannot inherit its backend", "backend"));
    } else {
        Selection selection;
        if (!token(backend->second))
            return invalid(std::move(result), error(Code::InvalidField, "The backend identifier is not a bounded token", "backend"));
        if (backend->second == "legacy-lff") {
            if (identityPresent) return invalid(std::move(result), error(Code::InvalidField, "A legacy opt-out cannot carry identity fields", "backend"));
        } else if (backend->second == "harfbuzz-ot") {
            selection.backend = Backend::HarfBuzzOpenType;
            for (const auto *key : {"profile", "sha256", "metrics", "shaping", "engine"})
                if (!strings.count(key)) return invalid(std::move(result), error(Code::MissingField, "The atomic OpenType identity is incomplete", key));
            if (!integers.count("face")) return invalid(std::move(result), error(Code::MissingField, "The atomic OpenType identity requires a face", "face"));
            if (integers.at("face") < 0) return invalid(std::move(result), error(Code::InvalidField, "The font face index cannot be negative", "face"));
            selection.identity = {strings.at("profile"), strings.at("sha256").toLatin1(),
                static_cast<unsigned>(integers.at("face")), strings.at("metrics"), strings.at("shaping"), strings.at("engine")};
        } else return opaque(std::move(result), error(Code::UnknownBackend, "The unknown backend was retained without activation", "backend"));
        value.selection = selection;
    }
    if (strings.count("language")) value.language = strings.at("language").toUtf8();
    if (integers.count("direction")) {
        const int direction = integers.at("direction");
        if (direction < 0 || direction > 3) return invalid(std::move(result), error(Code::InvalidField, "The direction enum is outside its schema range", "direction"));
        value.direction = static_cast<Direction>(direction);
    }
    if (auto issue = validate(value)) return invalid(std::move(result), std::move(issue));
    result.status = DecodeStatus::Known; result.value = std::move(value);
    return result;
}

LC_TextProfile::Encoded LC_TextProfile::encode(const Value &value) {
    Encoded result;
    if (auto issue = validate(value)) { result.error = std::move(issue); return result; }
    result.block = {{1001, applicationName()}, {1070, SchemaVersion},
        {1000, value.scope == Scope::StyleDefaults ? QStringLiteral("scope=style-default") : QStringLiteral("scope=entity-override")}};
    const auto string = [&](const char *key, const QString &text) {
        const QString field = QString::fromLatin1(key) + '=' + text;
        result.block.push_back({1000, field});
    };
    const auto integer = [&](const char *key, int number) {
        result.block.push_back({1000, QString::fromLatin1(key)}); result.block.push_back({1070, number});
    };
    if (value.selection) {
        const auto &selection = *value.selection;
        string("backend", selection.backend == Backend::LegacyLff ? QStringLiteral("legacy-lff") : QStringLiteral("harfbuzz-ot"));
        if (selection.backend == Backend::HarfBuzzOpenType) {
            const auto &id = selection.identity;
            string("profile", id.profileId); string("sha256", QString::fromLatin1(id.fontSha256));
            integer("face", int(id.faceIndex)); string("metrics", id.metricPolicy);
            string("shaping", id.shapingPolicy); string("engine", id.engineIdentity);
        }
    }
    if (value.language) string("language", QString::fromLatin1(*value.language));
    if (value.direction) integer("direction", int(*value.direction));
    if (auto issue = blockBounds(result.block)) { result.block.clear(); result.error = std::move(issue); }
    return result;
}

LC_TextProfile::Inherited LC_TextProfile::inherit(const std::optional<Value> &style, const std::optional<Value> &entity,
                                                const QByteArray &fallbackLanguage, RtlMarker marker) {
    return inherit(modeled(style), modeled(entity), fallbackLanguage, marker);
}
LC_TextProfile::Inherited LC_TextProfile::inherit(const Decoded &style, const Decoded &entity,
                                                const QByteArray &fallbackLanguage, RtlMarker marker) {
    Inherited result;
    result.style = style; result.entity = entity;
    if (!language(fallbackLanguage)) { result.error = error(Code::InvalidField, "The explicit fallback language is invalid", "language"); return result; }
    if (marker != RtlMarker::None && marker != RtlMarker::LegacyReversed && marker != RtlMarker::LogicalRtl) {
        result.error = error(Code::InvalidField, "The legacy RTL marker enum is invalid"); return result;
    }
    for (const auto *decoded : {&style, &entity}) {
        if (decoded->status == DecodeStatus::Opaque || decoded->status == DecodeStatus::Invalid) {
            result.error = decoded->error;
            if (!result.error) result.error = error(Code::InvalidField, "The profile metadata cannot be safely inherited");
            return result;
        }
        if ((decoded->status == DecodeStatus::Absent && (decoded->value || decoded->error ||
              (decoded->raw && !decoded->raw->empty()))) ||
            (decoded->status != DecodeStatus::Absent && (!decoded->known() || decoded->error))) {
            result.error = error(Code::InvalidField, "The modeled profile decode state is inconsistent"); return result;
        }
        if (decoded->value) {
            if (auto issue = validate(*decoded->value)) { result.error = std::move(issue); return result; }
        }
    }
    if ((style.value && style.value->scope != Scope::StyleDefaults) ||
        (entity.value && entity.value->scope != Scope::EntityOverride)) {
        result.error = error(Code::InvalidScope, "Style defaults and entity overrides must occupy their explicit inheritance scopes"); return result;
    }
    auto &effective = result.effective;
    effective.language = fallbackLanguage;
    if (style.value) {
        effective.selection = *style.value->selection; effective.selectionOrigin = Origin::Style;
        effective.language = *style.value->language; effective.languageOrigin = Origin::Style;
        effective.direction = *style.value->direction; effective.directionOrigin = Origin::Style;
    }
    if (entity.value) {
        if (entity.value->selection) { effective.selection = *entity.value->selection; effective.selectionOrigin = Origin::Entity; }
        if (entity.value->language) { effective.language = *entity.value->language; effective.languageOrigin = Origin::Entity; }
    }
    if (entity.value && entity.value->direction && *entity.value->direction != Direction::ByStyle) {
        effective.direction = *entity.value->direction; effective.directionOrigin = Origin::Entity;
    } else if (marker == RtlMarker::LogicalRtl) {
        effective.direction = Direction::RightToLeft; effective.directionOrigin = Origin::LogicalRtlMarker;
    }
    result.status = InheritanceStatus::Resolved;
    if (marker == RtlMarker::LegacyReversed && effective.selection.backend == Backend::HarfBuzzOpenType) {
        result.status = InheritanceStatus::LegacyConflict;
        result.error = error(Code::LegacyOrderConflict, "Legacy reversed source conflicts with the stored OpenType profile; preserve both and retain legacy rendering");
    }
    return result;
}
