/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_embeddedtextmodel.h"
#include "lc_embeddedtextxdata.h"
#include "lc_textprofile.h"
#include <QCryptographicHash>
#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>

namespace {
using C = LC_EmbeddedTextModel;
using K = C::Kind;
using E = C::ErrorCode;
using Bytes = C::Bytes;
constexpr std::size_t headerSize = 64, fieldHeaderSize = 12;
static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559,
              "Embedded models require IEEE754 binary64");
enum class Schema {
    Top, Source, Imported, EntityProfile, StyleProfile, HistoricalProfile,
    EntityValue, StyleValue, Selection, Identity, CurrentStyle,
    Outer, Embedded, Legacy, Baseline, Pending, OpaqueExtra, Deformation
};
struct Spec { std::uint16_t id; K kind; bool required = true; Schema child = Schema::Top; };
struct Slice { const std::uint8_t *data = nullptr; std::size_t size = 0; };
struct View { std::uint16_t id; K kind; Slice value; };
using Views = std::vector<View>;
C::Error failure(E code, const char *message) { return {code, QString::fromLatin1(message), {}}; }
bool valid(const C::Limits &l) {
    return l.maximumBytes >= headerSize && l.maximumBytes <= 64u * 1024u * 1024u &&
        l.maximumItems <= 1000000u && l.maximumDepth >= 1 && l.maximumDepth <= 16;
}
bool add(std::size_t &total, std::size_t amount, std::size_t maximum) {
    if (total > maximum || amount > maximum - total) return false;
    total += amount; return true;
}
void put(Bytes &out, std::uint64_t value, unsigned width) {
    for (unsigned i = 0; i < width; ++i) out.push_back(std::uint8_t(value >> (8u * i)));
}
struct Reader {
    Slice input;
    std::size_t position = 0;
    bool integer(std::uint64_t &value, unsigned width) {
        if (position > input.size || width > input.size - position) return false;
        value = 0;
        for (unsigned i = 0; i < width; ++i) value |= std::uint64_t(input.data[position++]) << (8u * i);
        return true;
    }
    bool slice(Slice &out, std::uint64_t size) {
        if (position > input.size || size > input.size - position) return false;
        out = {input.data ? input.data + position : nullptr, std::size_t(size)};
        position += std::size_t(size); return true;
    }
    bool finished() const { return position == input.size; }
};
std::uint64_t number(Slice s) {
    Reader reader{s}; std::uint64_t result = 0;
    reader.integer(result, unsigned(s.size)); return result;
}
const View *get(const Views &views, std::uint16_t id) {
    const auto it = std::lower_bound(views.begin(), views.end(), id,
        [](const View &v, std::uint16_t key) { return v.id < key; });
    return it != views.end() && it->id == id ? &*it : nullptr;
}
std::uint64_t number(const Views &v, std::uint16_t id) { return number(get(v, id)->value); }
bool present(const Views &v, std::uint16_t id) { return get(v, id) != nullptr; }
bool emptyText(Slice s) { return s.size == 9; }
QByteArray nullableBytes(Slice s) {
    if (!s.data[0]) return {};
    return QByteArray(reinterpret_cast<const char *>(s.data + 9), qsizetype(s.size - 9));
}
QString text(Slice s) {
    if (!s.data[0]) return {};
    const qsizetype count = qsizetype((s.size - 9) / 2);
    QString result(count, Qt::Uninitialized);
    for (qsizetype i = 0; i < count; ++i)
        result[i] = QChar(std::uint16_t(s.data[9 + 2 * std::size_t(i)]) |
                         (std::uint16_t(s.data[10 + 2 * std::size_t(i)]) << 8));
    return result;
}

std::vector<Spec> native(bool embedded) {
    std::vector<Spec> s = {
        {1,K::I32},{2,K::Bytes},{3,K::Bytes},{4,K::U32},{5,K::I32},{6,K::I32},
        {7,K::Real},{8,K::Bool},{9,K::I32},{10,K::Bytes},{11,K::I32},{12,K::Bytes},
        {13,K::I32},{14,K::I32},{15,K::I32},{16,K::U32},{17,K::U32},{18,K::U32},
        {19,K::U32},{20,K::U32Array},{21,K::U32},{22,K::Bool},{23,K::RawTags},
        {24,K::RawTagArrays},{25,K::Bool},{26,K::U64},{27,K::Bytes},{28,K::Bytes},
        {29,K::U32},{30,K::U32},{31,K::Bool},{32,K::U32},{33,K::U32},{34,K::Bytes},
        {35,K::Bool},{36,K::Bool},{37,K::U32},{38,K::U32},{39,K::U32},
        {40,K::RecordArray,true,Schema::Pending},{41,K::RecordArray,true,Schema::Pending},
        {42,K::U8},{43,K::U8},{44,K::U64},{45,K::U8},{46,K::U8},{47,K::U64},
        {48,K::U32},{49,K::U32},{50,K::U8},{51,K::U32},{52,K::U32},{53,K::Bool},
        {54,K::U8},{55,K::I32},{56,K::U8},{57,K::U8},{58,K::U8},{59,K::U8},
        {60,K::U8},{61,K::U8},{62,K::U8},{63,K::U8},{64,K::I32},
        {100,K::Coord},{101,K::Coord},{102,K::Coord},{103,K::Real},{104,K::Real},
        {105,K::Real},{106,K::Bytes},{107,K::Real},{108,K::Real},{109,K::Real},
        {110,K::Bytes},{111,K::I32},{112,K::I32},{113,K::I32},{114,K::U8},
        {115,K::U8},{116,K::U32},{117,K::U64}
    };
    if (embedded) {
        const std::vector<Spec> extra = {
            {200,K::Real},{201,K::U16},{202,K::I32},{203,K::Real},{204,K::I32},
            {205,K::I32},{206,K::Bool},{207,K::Bool},{208,K::U16},{209,K::Bool},
            {210,K::U32},{211,K::I32},{212,K::Coord},{213,K::Coord},{214,K::Real},
            {215,K::Real},{216,K::Real},{217,K::Real},{218,K::U16},{219,K::I32},
            {220,K::Real},{221,K::Real},{222,K::Bool},{223,K::Bool},
            {224,K::RealArray},{225,K::Bytes},{226,K::U16},{227,K::U32},{228,K::Bool}
        };
        s.insert(s.end(), extra.begin(), extra.end());
    } else {
        const std::vector<Spec> extra = {
            {200,K::Bytes},{201,K::U8},{202,K::U8},{203,K::U8},{204,K::U8},
            {205,K::I32},{206,K::Bool},{207,K::Bytes,false}
        };
        s.insert(s.end(), extra.begin(), extra.end());
    }
    return s;
}
const std::vector<Spec> &specs(Schema schema) {
    static const std::vector<Spec> top = {
        {1,K::Record,true,Schema::Source},{2,K::Record,false,Schema::Imported},
        {3,K::Record,true,Schema::Outer},{4,K::Record,true,Schema::Embedded},
        {5,K::Record,true,Schema::Legacy},{6,K::Record,true,Schema::Baseline},
        {7,K::RecordArray,false,Schema::OpaqueExtra}
    };
    static const std::vector<Spec> source = {
        {1,K::Utf16},{2,K::U8},{3,K::Bool},{4,K::Bool},{5,K::NullableBytes,false},
        {6,K::U8},{7,K::U8},{8,K::Utf16},{9,K::Utf16},{10,K::U8},{11,K::U8},
        {12,K::Utf16},{13,K::Utf16},{14,K::Utf16},{15,K::Bool},
        {16,K::Record,true,Schema::EntityProfile},{17,K::Record,true,Schema::CurrentStyle},
        {18,K::Record,true,Schema::Deformation}
    };
    static const std::vector<Spec> imported = {
        {1,K::Utf16},{2,K::U8},{3,K::Bool},{4,K::Bool},{5,K::NullableBytes,false},
        {6,K::U8},{7,K::U8},{8,K::Utf16},{9,K::Utf16},{10,K::U8},{11,K::U8},
        {12,K::Utf16},{13,K::Utf16},{14,K::Utf16},
        {16,K::Record,true,Schema::HistoricalProfile},{18,K::Record,false,Schema::Deformation}
    };
    static const std::vector<Spec> entityProfile = {
        {1,K::U8},{2,K::U8},{3,K::Record,false,Schema::EntityValue},
        {4,K::RawTags,false},{5,K::U8},{6,K::Utf16}
    };
    static const std::vector<Spec> styleProfile = {
        {1,K::U8},{2,K::U8},{3,K::Record,false,Schema::StyleValue},
        {4,K::RawTags,false},{5,K::U8},{6,K::Utf16}
    };
    static const std::vector<Spec> historical = {
        {1,K::U8},{2,K::U8},{3,K::Utf16},{4,K::RawTags,false}
    };
    static const std::vector<Spec> value = {
        {1,K::Record,false,Schema::Selection},{2,K::NullableBytes,false},{3,K::U8,false}
    };
    static const std::vector<Spec> selection = {{1,K::U8},{2,K::Record,false,Schema::Identity}};
    static const std::vector<Spec> identity = {
        {1,K::Utf16},{2,K::NullableBytes},{3,K::U32},{4,K::Utf16},{5,K::Utf16},{6,K::Utf16}
    };
    static const std::vector<Spec> currentStyle = {
        {1,K::Utf16},{2,K::Record,true,Schema::StyleProfile},{3,K::RawTags,false}
    };
    static const auto outer = native(false), embedded = native(true);
    static const std::vector<Spec> legacy = {{1,K::U8},{2,K::Bool},{3,K::Utf16}};
    static const std::vector<Spec> baseline = {
        {1,K::U8},{2,K::U32},{3,K::U8},{4,K::Bool},{5,K::Bool},{6,K::Bool},
        {7,K::Record,true,Schema::Outer},{8,K::Record,false,Schema::Embedded}
    };
    static const std::vector<Spec> pending = {{1,K::U64},{2,K::U32},{3,K::U64},{4,K::Bool}};
    static const std::vector<Spec> opaque = {{1,K::Utf16},{2,K::U32},{3,K::Bytes}};
    static const std::vector<Spec> deformation = {{1,K::Real},{2,K::Real},{3,K::Real},{4,K::Real}};
    switch (schema) {
    case Schema::Top:return top; case Schema::Source:return source;
    case Schema::Imported:return imported; case Schema::EntityProfile:return entityProfile;
    case Schema::StyleProfile:return styleProfile; case Schema::HistoricalProfile:return historical;
    case Schema::EntityValue:case Schema::StyleValue:return value;
    case Schema::Selection:return selection; case Schema::Identity:return identity;
    case Schema::CurrentStyle:return currentStyle; case Schema::Outer:return outer;
    case Schema::Embedded:return embedded; case Schema::Legacy:return legacy;
    case Schema::Baseline:return baseline; case Schema::Pending:return pending;
    case Schema::OpaqueExtra:return opaque;
    case Schema::Deformation:return deformation;
    }
    return top;
}

struct Validator {
    const C::Limits &limits;
    std::size_t items = 0;
    C::Error error;
    bool fail(E code, const char *message) { error = failure(code,message); return false; }
    bool consume(std::uint64_t count) {
        if (count > limits.maximumItems || !add(items, std::size_t(count), limits.maximumItems))
            return fail(E::ResourceLimit,"Aggregate embedded model item limit exceeded");
        return true;
    }
    bool fields(Slice input, std::uint64_t count, Views &out) {
        if (count > input.size / fieldHeaderSize)
            return fail(E::Malformed,"Embedded model field count exceeds remaining bytes");
        if (!consume(count)) return false;
        out.reserve(std::size_t(count)); Reader reader{input}; std::uint16_t previous = 0;
        for (std::uint64_t index = 0; index < count; ++index) {
            std::uint64_t id, kind, flags, size; Slice payload;
            if (!reader.integer(id,2) || !reader.integer(kind,1) || !reader.integer(flags,1) ||
                !reader.integer(size,8) || !reader.slice(payload,size))
                return fail(E::Malformed,"Truncated embedded model field");
            if (!id || id <= previous || flags)
                return fail(E::InvalidField,"Embedded model IDs must be ascending unique and flags zero");
            previous = std::uint16_t(id);
            out.push_back({std::uint16_t(id),static_cast<K>(kind),payload});
        }
        if (!reader.finished()) return fail(E::Malformed,"Trailing embedded model record bytes");
        return true;
    }
    bool record(Slice s, Schema schema, unsigned depth, Views *views = nullptr) {
        if (depth > limits.maximumDepth) return fail(E::ResourceLimit,"Embedded model nesting limit exceeded");
        Reader r{s}; std::uint64_t count; Slice rest;
        if (!r.integer(count,8) || !r.slice(rest,s.size-r.position))
            return fail(E::Malformed,"Truncated embedded model record count");
        Views local;
        if (!fields(rest,count,local) || !validate(local,schema,depth)) return false;
        if (views) *views = std::move(local);
        return true;
    }
    bool raw(Slice s) {
        Reader r{s}; std::uint64_t magic,version,count;
        if (!r.integer(magic,4) || magic != 0x5843544c || !r.integer(version,1))
            return fail(E::Malformed,"Invalid retained raw-tag capsule");
        if (version != 2) return fail(E::UnknownVersion,"Unknown retained raw-tag capsule version");
        if (!r.integer(count,4)) return fail(E::Malformed,"Truncated retained raw-tag capsule count");
        if (!consume(count)) return false;
        const Bytes input(s.data,s.data+s.size);
        const auto result = LC_EmbeddedTextXData::decode(input,{std::size_t(count),limits.maximumBytes});
        if (!result) return fail(result.error.code == LC_EmbeddedTextXData::ErrorCode::ResourceLimit ?
            E::ResourceLimit:E::Malformed,"Malformed retained raw-tag capsule");
        return true;
    }
    bool field(const View &v, const Spec &spec, unsigned depth) {
        const Slice s = v.value;
        switch (v.kind) {
        case K::Bool:return s.size == 1 && s.data[0] <= 1 ? true : fail(E::InvalidField,"Invalid Boolean storage");
        case K::U8:return s.size == 1 ? true : fail(E::InvalidField,"Invalid U8 storage");
        case K::U16:return s.size == 2 ? true : fail(E::InvalidField,"Invalid U16 storage");
        case K::U32:case K::I32:return s.size == 4 ? true : fail(E::InvalidField,"Invalid 32-bit storage");
        case K::U64:case K::I64:case K::Real:return s.size == 8 ? true : fail(E::InvalidField,"Invalid 64-bit storage");
        case K::Coord:return s.size == 24 ? true : fail(E::InvalidField,"Invalid coordinate storage");
        case K::Bytes:return true;
        case K::Utf16:case K::NullableBytes: {
            Reader r{s}; std::uint64_t nullness,count;
            if (!r.integer(nullness,1) || nullness > 1 || !r.integer(count,8) ||
                (!nullness && count) || count > (s.size-r.position)/(v.kind==K::Utf16?2u:1u) ||
                count*(v.kind==K::Utf16?2u:1u) != s.size-r.position)
                return fail(E::InvalidField,"Invalid null-aware string storage");
            return true;
        }
        case K::Record:return record(s,spec.child,depth+1);
        case K::U32Array:case K::RealArray: {
            Reader r{s}; std::uint64_t count; const unsigned width = v.kind==K::U32Array?4:8;
            if (!r.integer(count,8) || count > (s.size-r.position)/width ||
                count*width != s.size-r.position)
                return fail(E::Malformed,"Invalid embedded model scalar array");
            return consume(count);
        }
        case K::RawTags:return raw(s);
        case K::RecordArray:case K::RawTagArrays: {
            Reader r{s}; std::uint64_t count;
            if (!r.integer(count,8) || count > (s.size-r.position)/8)
                return fail(E::Malformed,"Invalid embedded model record array count");
            if (!consume(count)) return false;
            if (depth+1 > limits.maximumDepth) return fail(E::ResourceLimit,"Embedded model nesting limit exceeded");
            for (std::uint64_t i=0;i<count;++i) {
                std::uint64_t size; Slice element;
                if (!r.integer(size,8) || !r.slice(element,size))
                    return fail(E::Malformed,"Truncated embedded model array element");
                if (!(v.kind==K::RecordArray ? record(element,spec.child,depth+1) : raw(element))) return false;
            }
            return r.finished() ? true : fail(E::Malformed,"Trailing embedded model array bytes");
        }
        }
        return fail(E::UnknownKind,"Unknown embedded model field kind");
    }
    bool parseChild(const Views &v, std::uint16_t id, Views &out) {
        // Previously validated; do not count fields twice or revalidate recursively.
        Reader r{get(v,id)->value}; std::uint64_t count; r.integer(count,8);
        out.reserve(std::size_t(count));
        for (std::uint64_t i=0;i<count;++i) {
            std::uint64_t fieldId,kind,flags,size; Slice payload;
            r.integer(fieldId,2);r.integer(kind,1);r.integer(flags,1);r.integer(size,8);r.slice(payload,size);
            out.push_back({std::uint16_t(fieldId),static_cast<K>(kind),payload});
        }
        return true;
    }
    bool value(const Views &v, bool style) {
        LC_TextProfile::Value result;
        result.scope = style ? LC_TextProfile::Scope::StyleDefaults:LC_TextProfile::Scope::EntityOverride;
        if (present(v,1)) {
            Views selection; parseChild(v,1,selection);
            LC_TextProfile::Selection selected;
            selected.backend = number(selection,1) ? LC_TextProfile::Backend::HarfBuzzOpenType:
                LC_TextProfile::Backend::LegacyLff;
            if (present(selection,2)) {
                Views identity; parseChild(selection,2,identity);
                selected.identity = {text(get(identity,1)->value),nullableBytes(get(identity,2)->value),
                    unsigned(number(identity,3)),text(get(identity,4)->value),
                    text(get(identity,5)->value),text(get(identity,6)->value)};
            }
            result.selection = std::move(selected);
        }
        if (present(v,2)) result.language = nullableBytes(get(v,2)->value);
        if (present(v,3)) result.direction = static_cast<LC_TextProfile::Direction>(number(v,3));
        if (LC_TextProfile::validate(result)) return fail(E::InvalidField,"Invalid materialized current profile value");
        return true;
    }
    bool semantics(const Views &v, Schema schema) {
        switch (schema) {
        case Schema::Source:case Schema::Imported: {
            if (number(v,2)>1 || number(v,6)>2 || number(v,7)>2 || number(v,10)>2 || number(v,11)>2 ||
                (!number(v,10) && number(v,11)) || (number(v,10) && !number(v,11)) ||
                (number(v,11)==2 && number(v,7)!=2))
                return fail(E::InvalidField,"Invalid source convention or marker provenance");
            if (present(v,5)) {
                const auto bytes = nullableBytes(get(v,5)->value);
                if (QString::fromUtf8(bytes.constData(),bytes.size()) != text(get(v,1)->value))
                    return fail(E::InvalidField,"Current/source UTF8 bytes do not decode to source");
            }
            return true;
        }
        case Schema::EntityProfile:case Schema::StyleProfile: {
            const auto status = number(v,1), transport = number(v,5);
            if (status>4 || number(v,2)!=(schema==Schema::StyleProfile?0u:1u) || transport>2 ||
                present(v,3)!=(status==1) || present(v,4)!=(status>=2) ||
                (status==4 ? (!transport || emptyText(get(v,6)->value)) : transport!=0))
                return fail(E::InvalidField,"Inconsistent materialized profile presence/scope/transport");
            return true;
        }
        case Schema::HistoricalProfile: {
            const auto status=number(v,1), transport=number(v,2);
            if (status>4 || transport>2 || present(v,4)!=(status!=0) ||
                (status==4 ? (!transport || emptyText(get(v,3)->value)) : transport!=0))
                return fail(E::InvalidField,"Inconsistent historical profile provenance");
            return true;
        }
        case Schema::EntityValue:return value(v,false);
        case Schema::StyleValue:return value(v,true);
        case Schema::Selection:
            return number(v,1)<=1 && present(v,2)==(number(v,1)==1) ? true :
                fail(E::InvalidField,"Backend selection and identity must be atomic");
        case Schema::Legacy:
            return number(v,1)<=2 && (!number(v,2) || (number(v,1)==2 && !emptyText(get(v,3)->value))) ? true :
                fail(E::InvalidField,"Invalid legacy embedded marker provenance");
        case Schema::Baseline:
            return number(v,1)<=1 && number(v,3)<=1 && present(v,8)==bool(number(v,6)) ? true :
                fail(E::InvalidField,"Inconsistent target baseline native-body presence");
        default:return true;
        }
    }
    bool validate(const Views &v, Schema schema, unsigned depth) {
        const auto &registry = specs(schema);
        for (const auto &entry : v) {
            const auto rawKind = static_cast<unsigned>(entry.kind);
            if (rawKind<1 || rawKind>18) return fail(E::UnknownKind,"Unknown embedded model field kind");
            const auto it=std::lower_bound(registry.begin(),registry.end(),entry.id,
                [](const Spec &s,std::uint16_t id){return s.id<id;});
            if (it==registry.end() || it->id!=entry.id) {
                error = failure(E::UnknownField,"Unknown embedded model field");error.path.push_back(entry.id);return false;
            }
            if (it->kind != entry.kind) {
                error=failure(E::InvalidField,"Embedded model field has the wrong registered kind");
                error.path.push_back(entry.id);return false;
            }
            if (!field(entry,*it,depth)) {error.path.insert(error.path.begin(),entry.id);return false;}
        }
        for (const auto &spec:registry) if (spec.required && !present(v,spec.id)) {
            error=failure(E::MissingField,"Missing required embedded model field");
            error.path.push_back(spec.id);return false;
        }
        return semantics(v,schema);
    }
};

C::Encoded pack(const C::Record &record,const C::Limits &limits,bool counted) {
    if (!valid(limits)) return {std::nullopt,failure(E::InvalidLimits,"Invalid embedded model limits")};
    if (record.size()>limits.maximumItems)
        return {std::nullopt,failure(E::ResourceLimit,"Embedded model field count exceeds limit")};
    std::size_t size=counted?8:0;
    std::uint16_t previous=0;
    for (const auto &f:record) {
        if (!f.id || f.id<=previous) return {std::nullopt,failure(E::InvalidField,"Field IDs must be ascending unique")};
        previous=f.id;
        if (!add(size,fieldHeaderSize,limits.maximumBytes) || !add(size,f.payload.size(),limits.maximumBytes))
            return {std::nullopt,failure(E::ResourceLimit,"Embedded model byte limit exceeded")};
    }
    Bytes result;result.reserve(size);if(counted)put(result,record.size(),8);
    for (const auto &f:record) {
        put(result,f.id,2);put(result,static_cast<std::uint8_t>(f.kind),1);put(result,0,1);
        put(result,f.payload.size(),8);result.insert(result.end(),f.payload.begin(),f.payload.end());
    }
    return {std::move(result),{}};
}
bool opaque(E code) { return code==E::UnknownVersion || code==E::UnknownField || code==E::UnknownKind; }
C::Decoded retained(const Bytes &input,C::Error error) {
    return {opaque(error.code)?C::Status::Opaque:C::Status::Invalid,std::nullopt,input,std::move(error)};
}
C::Error allocationFailure() { return failure(E::ResourceLimit,"Embedded model allocation unavailable; input not accepted"); }
}

bool LC_EmbeddedTextModel::Field::operator==(const Field &o) const {
    return id==o.id && kind==o.kind && payload==o.payload;
}
LC_EmbeddedTextModel::Encoded LC_EmbeddedTextModel::packRecord(const Record &input,const Limits &limits) {
    try {return pack(input,limits,true);} catch(const std::bad_alloc &) {return {std::nullopt,allocationFailure()};}
    catch(const std::length_error &) {return {std::nullopt,allocationFailure()};}
}
LC_EmbeddedTextModel::Encoded LC_EmbeddedTextModel::packRecordArray(const std::vector<Record> &input,const Limits &limits) {
    try {
        if (!valid(limits)) return {std::nullopt,failure(E::InvalidLimits,"Invalid embedded model limits")};
        if(input.size()>limits.maximumItems) return {std::nullopt,failure(E::ResourceLimit,"Record array item limit exceeded")};
        std::size_t size=8,items=input.size();std::vector<Bytes> payloads;payloads.reserve(input.size());
        for(const auto &record:input) {
            if(!add(items,record.size(),limits.maximumItems))
                return {std::nullopt,failure(E::ResourceLimit,"Record array field limit exceeded")};
            auto payload=pack(record,limits,true);if(!payload)return payload;
            if(!add(size,8,limits.maximumBytes)||!add(size,payload.bytes->size(),limits.maximumBytes))
                return {std::nullopt,failure(E::ResourceLimit,"Record array byte limit exceeded")};
            payloads.push_back(std::move(*payload.bytes));
        }
        Bytes result;result.reserve(size);put(result,input.size(),8);
        for(const auto &payload:payloads){put(result,payload.size(),8);result.insert(result.end(),payload.begin(),payload.end());}
        return {std::move(result),{}};
    } catch(const std::bad_alloc &) {return {std::nullopt,allocationFailure()};}
    catch(const std::length_error &) {return {std::nullopt,allocationFailure()};}
}
LC_EmbeddedTextModel::Encoded LC_EmbeddedTextModel::encode(const Record &input,const Limits &limits) {
    try {
        auto body=pack(input,limits,false);if(!body)return body;
        std::size_t size=headerSize;
        if(!add(size,body.bytes->size(),limits.maximumBytes))
            return {std::nullopt,failure(E::ResourceLimit,"Complete embedded model byte limit exceeded")};
        Validator validator{limits,0,{}};Views views;
        if(!validator.fields({body.bytes->data(),body.bytes->size()},input.size(),views) ||
            !validator.validate(views,Schema::Top,1)) return {std::nullopt,std::move(validator.error)};
        const auto digest=QCryptographicHash::hash(QByteArrayView(reinterpret_cast<const char *>(body.bytes->data()),
            qsizetype(body.bytes->size())),QCryptographicHash::Sha256);
        Bytes output;output.reserve(size);output.insert(output.end(),{'L','C','E','M'});
        put(output,EnvelopeVersion,2);put(output,ModelSchemaVersion,2);
        put(output,headerSize,4);put(output,body.bytes->size(),8);
        put(output,input.size(),4);output.insert(output.end(),digest.begin(),digest.end());put(output,0,8);
        output.insert(output.end(),body.bytes->begin(),body.bytes->end());return {std::move(output),{}};
    } catch(const std::bad_alloc &) {return {std::nullopt,allocationFailure()};}
    catch(const std::length_error &) {return {std::nullopt,allocationFailure()};}
}
LC_EmbeddedTextModel::Decoded LC_EmbeddedTextModel::decode(const Bytes &input,const Limits &limits) {
    try {
        if(!valid(limits)) return {Status::Refused,std::nullopt,std::nullopt,failure(E::InvalidLimits,"Invalid embedded model limits")};
        if(input.size()>limits.maximumBytes)
            return {Status::Refused,std::nullopt,std::nullopt,failure(E::ResourceLimit,"Complete embedded model byte limit exceeded")};
        Reader r{{input.data(),input.size()}};Slice magic,digest,body;
        std::uint64_t envelope,schema,header,bodyBytes,count,reserved;
        if(!r.slice(magic,4) || std::memcmp(magic.data,"LCEM",4) ||
            !r.integer(envelope,2)||!r.integer(schema,2)||!r.integer(header,4)||
            !r.integer(bodyBytes,8)||!r.integer(count,4)||!r.slice(digest,32)||!r.integer(reserved,8))
            return retained(input,failure(E::Malformed,"Truncated or invalid embedded model header"));
        if(envelope!=EnvelopeVersion || schema!=ModelSchemaVersion)
            return retained(input,failure(E::UnknownVersion,"Unsupported embedded model version; retained without migration"));
        if(header!=headerSize || reserved || !r.slice(body,bodyBytes) || !r.finished())
            return retained(input,failure(E::Malformed,"Invalid embedded model length/reserved/trailing storage"));
        const auto actual=QCryptographicHash::hash(QByteArrayView(reinterpret_cast<const char *>(body.data),qsizetype(body.size)),
            QCryptographicHash::Sha256);
        if(std::memcmp(actual.constData(),digest.data,32))
            return retained(input,failure(E::DigestMismatch,"Embedded model body digest mismatch"));
        Validator validator{limits,0,{}};Views views;
        if(!validator.fields(body,count,views)||!validator.validate(views,Schema::Top,1))
            return retained(input,std::move(validator.error));
        Record result;result.reserve(views.size());
        for(const auto &view:views) result.push_back({view.id,view.kind,Bytes(view.value.data,view.value.data+view.value.size)});
        return {Status::Known,std::move(result),std::nullopt,{}};
    } catch(const std::bad_alloc &) {return {Status::Refused,std::nullopt,std::nullopt,allocationFailure()};}
    catch(const std::length_error &) {return {Status::Refused,std::nullopt,std::nullopt,allocationFailure()};}
}
