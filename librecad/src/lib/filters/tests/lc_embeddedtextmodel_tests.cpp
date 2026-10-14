/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_embeddedtextmodel.h"
#include "lc_embeddedtextxdata.h"
#include <catch2/catch_test_macros.hpp>
#include <QCryptographicHash>
#include <QFile>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace {
using C = LC_EmbeddedTextModel;
using K = C::Kind;
using B = C::Bytes;
using R = C::Record;
using F = C::Field;
void append(B &to,const B &from) {to.insert(to.end(),from.begin(),from.end());}
void integer(B &out,std::uint64_t n,unsigned width) {
    for(unsigned i=0;i<width;++i)out.push_back(std::uint8_t(n>>(8u*i)));
}
B scalar(std::uint64_t n,unsigned width) {B result;integer(result,n,width);return result;}
B hex(const char *literal) {
    const auto bytes=QByteArray::fromHex(QByteArray(literal));return B(bytes.begin(),bytes.end());
}
B nullable(const B &bytes,bool null=false) {
    B out{std::uint8_t(!null)};integer(out,bytes.size(),8);append(out,bytes);return out;
}
B utf16(const QString &s) {
    B out{std::uint8_t(!s.isNull())};integer(out,std::uint64_t(s.size()),8);
    for(const auto unit:s)integer(out,unit.unicode(),2);return out;
}
F f(std::uint16_t id,K kind,B bytes) {return {id,kind,std::move(bytes)};}
F u8(std::uint16_t id,std::uint8_t n=0) {return f(id,K::U8,{n});}
F boolean(std::uint16_t id,bool n=false) {return f(id,K::Bool,{std::uint8_t(n)});}
F str(std::uint16_t id,const QString &s=QStringLiteral("")) {return f(id,K::Utf16,utf16(s));}
// Independent field/record/array/framing builder; no production pack/encode use.
B fields(const R &r,bool counted=true) {
    B out;if(counted)integer(out,r.size(),8);
    for(const auto &field:r) {
        integer(out,field.id,2);integer(out,std::uint8_t(field.kind),1);integer(out,0,1);
        integer(out,field.payload.size(),8);append(out,field.payload);
    }
    return out;
}
F rec(std::uint16_t id,const R &r) {return f(id,K::Record,fields(r));}
B array(const std::vector<B> &input) {
    B out;integer(out,input.size(),8);
    for(const auto &element:input){integer(out,element.size(),8);append(out,element);}return out;
}
B packet(const R &r,std::uint16_t schema=2) {
    const auto body=fields(r,false);
    const auto digest=QCryptographicHash::hash(QByteArrayView(reinterpret_cast<const char *>(body.data()),
        qsizetype(body.size())),QCryptographicHash::Sha256);
    B out{'L','C','E','M',1,0};integer(out,schema,2);integer(out,64,4);
    integer(out,body.size(),8);integer(out,r.size(),4);
    out.insert(out.end(),digest.begin(),digest.end());integer(out,0,8);append(out,body);return out;
}
R unpack(const B &payload) {
    auto read=[&payload](std::size_t &position,unsigned width) {
        REQUIRE(position<=payload.size());REQUIRE(width<=payload.size()-position);
        std::uint64_t n=0;for(unsigned i=0;i<width;++i)n|=std::uint64_t(payload[position++])<<(8*i);return n;
    };
    std::size_t p=0;const auto count=read(p,8);R out;
    for(std::uint64_t i=0;i<count;++i) {
        const auto id=read(p,2),kind=read(p,1),flags=read(p,1),size=read(p,8);
        REQUIRE(flags==0);REQUIRE(size<=payload.size()-p);
        out.push_back(f(std::uint16_t(id),static_cast<K>(kind),B(payload.begin()+std::ptrdiff_t(p),payload.begin()+std::ptrdiff_t(p+size))));
        p+=std::size_t(size);
    }
    REQUIRE(p==payload.size());return out;
}
F &at(R &r,std::uint16_t id) {
    const auto it=std::find_if(r.begin(),r.end(),[id](const auto &field){return field.id==id;});
    REQUIRE(it!=r.end());return *it;
}
const F &at(const R &r,std::uint16_t id) {
    const auto it=std::find_if(r.begin(),r.end(),[id](const auto &field){return field.id==id;});
    REQUIRE(it!=r.end());return *it;
}
void putChild(R &r,std::uint16_t id,const R &child) {at(r,id).payload=fields(child);}
B emptyRaw() {return hex("4c5443580200000000");}
B literalRaw() {
    // Hand-authored capsule, including null, INVALID, signed values, NUL bytes,
    // binary and active1003 u64 provenance. The model never activates this raw.
    return hex("4c5443580207000000"
        "0000000000" "0100000000" "03f9fffffffeffffff"
        "04341200000807060504030201"
        "02e8030000030000004100420100"
        "07ec0300000300000000ff7f"
        "02eb030000010000004c0101110000001032547698badcfe");
}
B literalNativeAdditions() {
    // Independently authored counted fields, each nondefault. Deliberate
    // raw/semantic contradictions must remain inert, not normalized by codec.
    return hex("1800000000000000"
        "2a000200010000000000000005" // layer handle code
        "2b000200010000000000000008" // layer handle size
        "2c00050008000000000000001032547698badcfe" // complete layer ref64
        "2d00020001000000000000000a" // linetype handle code
        "2e000200010000000000000007" // linetype handle size
        "2f00050008000000000000000100000001000080" // complete linetype ref64
        "30000400040000000000000078563412" // source own handle
        "310004000400000000000000f0debc9a" // source parent handle
        "320002000100000000000000a5" // exact no-links byte
        "33000400040000000000000021436587" // previous source link
        "340004000400000000000000deadbeef" // next source link
        "35000100010000000000000001" // source owner-present bool
        "3600020001000000000000007e" // xdictionary flag
        "370006000400000000000000feffffff" // negative advertised reactor count
        "38000200010000000000000081" // plot presence
        "39000200010000000000000002" // linetype presence
        "3a000200010000000000000003" // material presence
        "3b0002000100000000000000c3" // full shadow byte
        "3c000200010000000000000002" // full visual style presence
        "3d000200010000000000000080" // face visual style presence
        "3e0002000100000000000000ff" // edge visual style presence
        "3f000200010000000000000080" // stored DataStorage byte
        "400006000400000000000000efbeffff" // signed retained object kind
        "750005000800000000000000efcdab8967452301"); // complete STYLE ref64
}
R absentProfile(bool style=false) {
    return {u8(1),u8(2,style?0:1),u8(5),str(6)};
}
R currentSource() {
    // Literal direction-only Known profile: never materialized as selection.
    const B directionOnly=hex(
        "0500000000000000" // 5 fields
        "01000200010000000000000001" // status Known
        "02000200010000000000000001" // scope EntityOverride
        "03000c001500000000000000" // KnownValue record 21 bytes
          "010000000000000003000200010000000000000002" // only direction RTL
        "05000200010000000000000000" // representable
        "06000b000900000000000000010000000000000000");
    return {
        str(1,QString::fromUtf16(u"A\0ب",3)),u8(2,1),boolean(3,true),boolean(4,true),
        u8(6,1),u8(7,2),str(8,QStringLiteral("EMBED_STYLE")),str(9,QStringLiteral("amiri-regular")),
        u8(10,2),u8(11,2),str(12,QStringLiteral("ORIGINAL_STYLE")),str(13),str(14),boolean(15,true),
        f(16,K::Record,directionOnly),rec(17,{str(1,QStringLiteral("EMBED_STYLE")),rec(2,absentProfile(true)),f(3,K::RawTags,literalRaw())}),
        rec(18,{f(1,K::Real,scalar(0x3ff0000000000000ULL,8)),f(2,K::Real,scalar(0,8)),
            f(3,K::Real,scalar(0,8)),f(4,K::Real,scalar(0x3ff0000000000000ULL,8))})
    };
}
F zero(std::uint16_t id,K kind) {
    switch(kind) {
    case K::Bool:case K::U8:return f(id,kind,B(1));
    case K::U16:return f(id,kind,B(2));
    case K::U32:case K::I32:return f(id,kind,B(4));
    case K::U64:case K::I64:case K::Real:return f(id,kind,B(8));
    case K::Coord:return f(id,kind,B(24));
    case K::Bytes:return f(id,kind,{});
    case K::Utf16:return str(id);
    case K::RawTags:return f(id,kind,emptyRaw());
    case K::U32Array:case K::RealArray:case K::RecordArray:case K::RawTagArrays:return f(id,kind,B(8));
    default:FAIL("Unexpected independent fixture kind");return {};
    }
}
R native(bool embedded) {
    const K common[]={K::I32,K::Bytes,K::Bytes,K::U32,K::I32,K::I32,K::Real,K::Bool,
        K::I32,K::Bytes,K::I32,K::Bytes,K::I32,K::I32,K::I32,K::U32,K::U32,K::U32,
        K::U32,K::U32Array,K::U32,K::Bool,K::RawTags,K::RawTagArrays,K::Bool,K::U64,
        K::Bytes,K::Bytes,K::U32,K::U32,K::Bool,K::U32,K::U32,K::Bytes,K::Bool,
        K::Bool,K::U32,K::U32,K::U32,K::RecordArray,K::RecordArray,
        K::U8,K::U8,K::U64,K::U8,K::U8,K::U64,K::U32,K::U32,K::U8,K::U32,K::U32,
        K::Bool,K::U8,K::I32,K::U8,K::U8,K::U8,K::U8,K::U8,K::U8,K::U8,K::U8,K::I32};
    const K shared[]={K::Coord,K::Coord,K::Coord,K::Real,K::Real,K::Real,K::Bytes,
        K::Real,K::Real,K::Real,K::Bytes,K::I32,K::I32,K::I32,K::U8,K::U8,K::U32,K::U64};
    const K outer[]={K::Bytes,K::U8,K::U8,K::U8,K::U8,K::I32,K::Bool};
    const K mtext[]={K::Real,K::U16,K::I32,K::Real,K::I32,K::I32,K::Bool,K::Bool,K::U16,
        K::Bool,K::U32,K::I32,K::Coord,K::Coord,K::Real,K::Real,K::Real,K::Real,K::U16,
        K::I32,K::Real,K::Real,K::Bool,K::Bool,K::RealArray,K::Bytes,K::U16,K::U32,K::Bool};
    R r;std::uint16_t id=1;for(const auto kind:common)r.push_back(zero(id++,kind));
    id=100;for(const auto kind:shared)r.push_back(zero(id++,kind));
    id=200;
    if(embedded)for(const auto kind:mtext)r.push_back(zero(id++,kind));
    else for(const auto kind:outer)r.push_back(zero(id++,kind));
    at(r,1).payload=scalar(0x80000000,4); // unsupported native enum kept as bits
    at(r,2).payload={0xff,0,0xfe};at(r,7).payload=scalar(0x7ff8000000000042ULL,8);
    at(r,23).payload=literalRaw();at(r,24).payload=array({emptyRaw(),literalRaw()});
    at(r,26).payload=scalar(0xfedcba9876543210ULL,8);
    at(r,40).payload=array({fields({f(1,K::U64,scalar(9,8)),f(2,K::U32,scalar(0xffffffff,4)),
        f(3,K::U64,scalar(0x8000000000000001ULL,8)),boolean(4,true)})});
    at(r,100).payload=hex("0000000000000080000000000000f07f341200000000f87f");
    at(r,106).payload={'a',0,0xff};at(r,110).payload={'S',0,'T'};
    at(r,116).payload=scalar(0xf0123456,4);
    for(const auto &field:unpack(literalNativeAdditions()))at(r,field.id).payload=field.payload;
    if(embedded) {
        at(r,224).payload=hex("02000000000000000000000000000080341200000000f87f");
        at(r,228).payload=hex("01"); // exact retained hasXAxisVec
    }
    return r;
}
R model() {
    auto source=currentSource();
    auto original=source;original.erase(std::remove_if(original.begin(),original.end(),[](const auto &field){return field.id>=15;}),original.end());
    original.push_back(rec(16,{u8(1,4),u8(2,1),str(3,QStringLiteral("historical duplicate")),f(4,K::RawTags,literalRaw())}));
    const auto outer=native(false),embedded=native(true);
    return {rec(1,source),rec(2,original),rec(3,outer),rec(4,embedded),
        rec(5,{u8(1,2),boolean(2,true),str(3,QStringLiteral("legacy ambiguous ownership"))}),
        rec(6,{u8(1,1),f(2,K::U32,scalar(17,4)),u8(3,1),boolean(4),boolean(5),boolean(6,true),
            rec(7,outer),rec(8,embedded)}),
        f(7,K::RecordArray,array({fields({str(1,QStringLiteral("unmodeled")),f(2,K::U32,scalar(9,4)),
            f(3,K::Bytes,{0,0xff,0,0xfe})})}))};
}
void rejected(const R &r,C::ErrorCode expected) {
    const auto bytes=packet(r);const auto before=bytes;
    const auto decoded=C::decode(bytes);CHECK_FALSE(decoded.known());CHECK_FALSE(decoded.model);
    CHECK(decoded.error.code==expected);REQUIRE(decoded.raw);CHECK(*decoded.raw==before);CHECK(bytes==before);
    const auto encoded=C::encode(r);CHECK_FALSE(encoded);CHECK_FALSE(encoded.bytes);
}
}

TEST_CASE("Full embedded model decodes independent field and literal fixtures exactly", "[embedded-model][headless]") {
    const auto input=model(),before=input;const auto literal=packet(input);
    const auto read=C::decode(literal);INFO(read.error.message.toStdString());REQUIRE(read.known());
    REQUIRE(read.model);CHECK(*read.model==input);CHECK_FALSE(read.raw);CHECK(input==before);
    const auto output=C::encode(input);INFO(output.error.message.toStdString());REQUIRE(output);
    CHECK(*output.bytes==literal);const auto again=C::encode(*read.model);REQUIRE(again);CHECK(*again.bytes==literal);
    const auto source=unpack(at(*read.model,1).payload);const auto profile=unpack(at(source,16).payload);
    const auto value=unpack(at(profile,3).payload);REQUIRE(value.size()==1);CHECK(value.front().id==3);CHECK(value.front().payload==B{2});
    const auto body=fields(input,false);
    CHECK(body.size()==9461);
    CHECK(QCryptographicHash::hash(QByteArrayView(reinterpret_cast<const char *>(body.data()),qsizetype(body.size())),
        QCryptographicHash::Sha256).toHex()==QByteArray("4f9f21576ca716113ae30227a79ae70704c203b31bc49508d10e158bd02a49ca"));
    // Optional independent fixture output for checksum audit; no production encoder.
    if(const char *path=std::getenv("LC_EMBEDDED_MODEL_LITERAL_OUTPUT")) {
        QFile file(QString::fromUtf8(path));REQUIRE(file.open(QIODevice::WriteOnly));
        REQUIRE(file.write(reinterpret_cast<const char *>(body.data()),qsizetype(body.size()))==qsizetype(body.size()));
    }
    const auto nested=C::packRecord(input);REQUIRE(nested);CHECK(*nested.bytes==fields(input));
    const auto records=C::packRecordArray({input,input});REQUIRE(records);
    CHECK(*records.bytes==array({fields(input),fields(input)}));
}

TEST_CASE("Schema2 retains every literal source handle link and discriminant without coercion", "[embedded-model][headless]") {
    const auto literal=literalNativeAdditions();
    CHECK(literal.size()==359);
    const auto expected=unpack(literal);REQUIRE(expected.size()==24);
    const auto input=model();const auto before=input;
    const auto wire=packet(input);const auto read=C::decode(wire);REQUIRE(read.known());
    const auto baseline=unpack(at(*read.model,6).payload);
    const std::vector<R> snapshots{unpack(at(*read.model,3).payload),unpack(at(*read.model,4).payload),
        unpack(at(baseline,7).payload),unpack(at(baseline,8).payload)};
    for(const auto &snapshot:snapshots) {
        for(const auto &field:expected) {
            INFO("exact field "<<field.id);
            CHECK(at(snapshot,field.id)==field);
            CHECK(std::any_of(field.payload.begin(),field.payload.end(),[](std::uint8_t byte){return byte!=0;}));
        }
        // All of these contradictions are representable inert history. They
        // must not be reconciled, default-filled or treated as active graph data.
        CHECK(at(snapshot,38).payload==B(4));CHECK(at(snapshot,44).payload==hex("1032547698badcfe"));
        CHECK(at(snapshot,39).payload==B(4));CHECK(at(snapshot,47).payload==hex("0100000001000080"));
        CHECK(at(snapshot,116).payload==hex("563412f0"));CHECK(at(snapshot,117).payload==hex("efcdab8967452301"));
        CHECK(at(snapshot,21).payload==B(4));CHECK(at(snapshot,54).payload==B{0x7e});
        CHECK(at(snapshot,20).payload==B(8));CHECK(at(snapshot,55).payload==hex("feffffff"));
        CHECK(at(snapshot,35).payload==B{0});CHECK(at(snapshot,63).payload==B{0x80});
    }
    const auto output=C::encode(*read.model);REQUIRE(output);CHECK(*output.bytes==wire);CHECK(input==before);
}

TEST_CASE("Schema2 retains embedded X axis precedence separately from final angle", "[embedded-model][headless]") {
    // Literal Bool TLV with a nondefault retained parser/precedence state.
    const auto flag=unpack(hex("0100000000000000e4000100010000000000000001"));
    REQUIRE(flag.size()==1);CHECK(flag.front().id==228);CHECK(flag.front().kind==K::Bool);
    for(const bool retained:{false,true}) {
        auto r=model();auto embedded=unpack(at(r,4).payload);
        at(embedded,101).payload=hex("0000000000000000000000000000f03f0000000000000000");
        at(embedded,107).payload=scalar(0,8);
        at(embedded,228).payload=retained?flag.front().payload:B{0};putChild(r,4,embedded);
        const auto read=C::decode(packet(r));REQUIRE(read.known());CHECK(*read.model==r);
        const auto body=unpack(at(*read.model,4).payload);
        CHECK(at(body,101).payload==at(embedded,101).payload);
        CHECK(at(body,107).payload==B(8));CHECK(at(body,228).payload==B{std::uint8_t(retained)});
        const auto encoded=C::encode(*read.model);REQUIRE(encoded);CHECK(*encoded.bytes==packet(r));
    }
}

TEST_CASE("Old complete schema1 and future registries remain exact opaque without migration", "[embedded-model][headless]") {
    auto old=model();
    const auto strip=[](R native) {
        native.erase(std::remove_if(native.begin(),native.end(),[](const auto &field) {
            return (field.id>=42&&field.id<=64)||field.id==117||field.id==228;
        }),native.end());
        return native;
    };
    for(const std::uint16_t parent:{3,4})putChild(old,parent,strip(unpack(at(old,parent).payload)));
    auto baseline=unpack(at(old,6).payload);
    for(const std::uint16_t parent:{7,8})putChild(baseline,parent,strip(unpack(at(baseline,parent).payload)));
    putChild(old,6,baseline);
    // Construct a complete old-registry body and its correct schema1 envelope,
    // count, length and digest independently. Do not just corrupt a new header.
    auto native=unpack(at(old,3).payload);at(native,106).payload={0x71,0,0xff,0xfe,0x72};putChild(old,3,native);
    const auto oldBody=fields(old,false),oldFrame=packet(old,1);
    REQUIRE(oldFrame.size()==64+oldBody.size());CHECK(oldFrame[6]==1);CHECK(oldFrame[7]==0);
    CHECK(B(oldFrame.begin()+64,oldFrame.end())==oldBody);
    const auto digest=QCryptographicHash::hash(QByteArrayView(reinterpret_cast<const char *>(oldBody.data()),
        qsizetype(oldBody.size())),QCryptographicHash::Sha256);
    CHECK(B(oldFrame.begin()+24,oldFrame.begin()+56)==B(digest.begin(),digest.end()));
    const auto oldRead=C::decode(oldFrame);CHECK(oldRead.status==C::Status::Opaque);
    CHECK(oldRead.error.code==C::ErrorCode::UnknownVersion);CHECK_FALSE(oldRead.model);
    REQUIRE(oldRead.raw);CHECK(*oldRead.raw==oldFrame);
    // The same old body falsely labeled schema2 cannot acquire zero-filled
    // handles/flags or become a known corrected model.
    const auto incomplete=C::decode(packet(old));CHECK(incomplete.status==C::Status::Invalid);
    CHECK(incomplete.error.code==C::ErrorCode::MissingField);CHECK_FALSE(incomplete.model);
    REQUIRE(incomplete.raw);CHECK(*incomplete.raw==packet(old));CHECK_FALSE(C::encode(old));
    const auto future=packet(model(),3);const auto futureRead=C::decode(future);
    CHECK(futureRead.status==C::Status::Opaque);REQUIRE(futureRead.raw);CHECK(*futureRead.raw==future);CHECK_FALSE(futureRead.model);
    for(const std::uint16_t parent:{3,4}) {
        auto unknown=model();auto child=unpack(at(unknown,parent).payload);
        child.insert(std::lower_bound(child.begin(),child.end(),65,[](const auto &entry,int id){return entry.id<id;}),
            f(65,K::Bytes,{0,0xff,0x80,0,0xfe}));
        putChild(unknown,parent,child);const auto wire=packet(unknown);const auto read=C::decode(wire);
        CHECK(read.status==C::Status::Opaque);CHECK(read.error.code==C::ErrorCode::UnknownField);
        REQUIRE(read.raw);CHECK(*read.raw==wire);CHECK_FALSE(read.model);CHECK_FALSE(C::encode(unknown));
    }
}

TEST_CASE("Full embedded model unknown and malformed frames retain exact raw inert", "[embedded-model][headless]") {
    // Hand-authored complete 64-byte unknown-version envelope, empty-body SHA256.
    const auto unknown=hex("4c43454d0200010040000000000000000000000000000000"
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
        "0000000000000000");
    REQUIRE(unknown.size()==64);auto decoded=C::decode(unknown);
    CHECK(decoded.status==C::Status::Opaque);REQUIRE(decoded.raw);CHECK(*decoded.raw==unknown);CHECK_FALSE(decoded.model);
    const auto good=packet(model());
    for(std::size_t size=0;size<good.size();++size) {
        const B truncated(good.begin(),good.begin()+std::ptrdiff_t(size));
        const auto result=C::decode(truncated);CHECK_FALSE(result.known());CHECK_FALSE(result.model);
        REQUIRE(result.raw);CHECK(*result.raw==truncated);CHECK(bool(result.error));
    }
    for(const auto offset:{std::size_t(0),std::size_t(8),std::size_t(12),std::size_t(20),std::size_t(24),std::size_t(56),std::size_t(64)}) {
        auto altered=good;altered[offset]^=0xff;const auto result=C::decode(altered);
        CHECK_FALSE(result.known());REQUIRE(result.raw);CHECK(*result.raw==altered);
    }
    auto trailing=good;trailing.push_back(0);CHECK_FALSE(C::decode(trailing).known());
    auto r=model();r.push_back(f(600,K::Bytes,{0,1,2}));
    rejected(r,C::ErrorCode::UnknownField);CHECK(C::decode(packet(r)).status==C::Status::Opaque);
    r=model();at(r,3).kind=static_cast<K>(255);rejected(r,C::ErrorCode::UnknownKind);
    r=model();r.insert(r.begin()+1,r.front());rejected(r,C::ErrorCode::InvalidField);
    r=model();std::swap(r[0],r[1]);rejected(r,C::ErrorCode::InvalidField);
}

TEST_CASE("Full embedded model validates every native field kind and required presence", "[embedded-model][headless]") {
    for(const std::uint16_t parent:{3,4}) {
        const auto baseline=model();const auto original=unpack(at(baseline,parent).payload);
        for(const auto &field:original) {
            INFO("parent "<<parent<<" field "<<field.id);
            auto child=original;at(child,field.id).kind=K::I64;
            auto r=baseline;putChild(r,parent,child);rejected(r,C::ErrorCode::InvalidField);
            child=original;child.erase(std::remove_if(child.begin(),child.end(),[&field](const auto &entry){return entry.id==field.id;}),child.end());
            r=baseline;putChild(r,parent,child);rejected(r,C::ErrorCode::MissingField);
        }
    }
    auto r=model();auto child=unpack(at(r,4).payload);at(child,8).payload={2};putChild(r,4,child);rejected(r,C::ErrorCode::InvalidField);
    r=model();child=unpack(at(r,3).payload);at(child,100).payload.pop_back();putChild(r,3,child);rejected(r,C::ErrorCode::InvalidField);
    r=model();child=unpack(at(r,3).payload);at(child,23).payload={0};putChild(r,3,child);rejected(r,C::ErrorCode::Malformed);
    r=model();child=unpack(at(r,3).payload);at(child,23).payload=emptyRaw();at(child,23).payload[4]=99;
    putChild(r,3,child);rejected(r,C::ErrorCode::UnknownVersion);
}

TEST_CASE("Full embedded model current profile authority is separate from raw history", "[embedded-model][headless]") {
    auto r=model();auto source=unpack(at(r,1).payload);
    putChild(source,16,absentProfile());putChild(r,1,source);REQUIRE(C::encode(r));
    for(const auto status:{0,1,2,3,4}) {
        R p{u8(1,std::uint8_t(status)),u8(2,1)};
        if(status==1)p.push_back(rec(3,{})); // Present empty override != Absent.
        if(status>=2)p.push_back(f(4,K::RawTags,literalRaw()));
        p.push_back(u8(5,status==4?2:0));p.push_back(str(6,status==4?QStringLiteral("unavailable"):QStringLiteral("")));
        auto s=source;putChild(s,16,p);auto candidate=r;putChild(candidate,1,s);REQUIRE(C::encode(candidate));
        auto wrong=p;at(wrong,2).payload={0};putChild(s,16,wrong);putChild(candidate,1,s);rejected(candidate,C::ErrorCode::InvalidField);
        wrong=p;at(wrong,5).payload={std::uint8_t(status==4?0:1)};putChild(s,16,wrong);putChild(candidate,1,s);rejected(candidate,C::ErrorCode::InvalidField);
    }
    // Complete STYLE defaults, with explicit legacy opt-out and language/direction.
    R legacySelection{u8(1)};
    R styleValue{rec(1,legacySelection),f(2,K::NullableBytes,nullable({'a','r'})),u8(3,2)};
    R styleProfile{u8(1,1),u8(2),rec(3,styleValue),u8(5),str(6)};
    auto style=unpack(at(source,17).payload);putChild(style,2,styleProfile);putChild(source,17,style);putChild(r,1,source);
    REQUIRE(C::encode(r));
    auto malformed=styleValue;at(malformed,2).payload=nullable({'a','r','_','E','G'});
    putChild(styleProfile,3,malformed);putChild(style,2,styleProfile);putChild(source,17,style);putChild(r,1,source);rejected(r,C::ErrorCode::InvalidField);
    malformed=styleValue;malformed.erase(malformed.begin());putChild(styleProfile,3,malformed);putChild(style,2,styleProfile);
    putChild(source,17,style);putChild(r,1,source);rejected(r,C::ErrorCode::InvalidField);
}

TEST_CASE("Full embedded model source presence nullness and provenance are exact", "[embedded-model][headless]") {
    const char16_t malformedUnits[]{0xd800,0,0xdfff};
    for(const auto &s:{QString(),QStringLiteral(""),QString::fromUtf16(malformedUnits,3)}) {
        auto r=model();auto source=unpack(at(r,1).payload);at(source,1).payload=utf16(s);putChild(r,1,source);
        const auto result=C::decode(packet(r));REQUIRE(result.known());CHECK(at(unpack(at(*result.model,1).payload),1).payload==utf16(s));
    }
    auto r=model();auto source=unpack(at(r,1).payload);
    source.insert(source.begin()+4,f(5,K::NullableBytes,nullable({'A',0,0xd8,0xa8})));putChild(r,1,source);REQUIRE(C::encode(r));
    at(source,5).payload=nullable({'s','t','a','l','e'});putChild(r,1,source);rejected(r,C::ErrorCode::InvalidField);
    source=unpack(at(model(),1).payload);at(source,11).payload={0};putChild(r,1,source);rejected(r,C::ErrorCode::InvalidField);
    source=unpack(at(model(),1).payload);at(source,7).payload={0};putChild(r,1,source);rejected(r,C::ErrorCode::InvalidField);
    source=unpack(at(model(),1).payload);at(source,1).payload=hex("0001000000000000000000");putChild(r,1,source);rejected(r,C::ErrorCode::InvalidField);
    r=model();auto baseline=unpack(at(r,6).payload);at(baseline,6).payload={0};putChild(r,6,baseline);rejected(r,C::ErrorCode::InvalidField);
}

TEST_CASE("Full embedded model aggregate budgets include nested fields arrays and raw", "[embedded-model][headless]") {
    const auto input=model();const auto bytes=packet(input);const auto before=input;
    C::Limits limits;limits.maximumBytes=bytes.size();REQUIRE(C::encode(input,limits));REQUIRE(C::decode(bytes,limits).known());
    --limits.maximumBytes;CHECK_FALSE(C::encode(input,limits));
    const auto refused=C::decode(bytes,limits);CHECK(refused.status==C::Status::Refused);CHECK_FALSE(refused.raw);CHECK_FALSE(refused.model);
    limits={};limits.maximumDepth=2;CHECK_FALSE(C::encode(input,limits));
    CHECK(C::decode(bytes,limits).error.code==C::ErrorCode::ResourceLimit);
    limits={};limits.maximumItems=7;CHECK_FALSE(C::encode(input,limits));
    CHECK(C::decode(bytes,limits).error.code==C::ErrorCode::ResourceLimit);
    // Discover exact aggregate ceiling; it includes raw variants and appData list
    // boundaries, not just native scalar fields or each independent capsule.
    std::size_t lo=0,hi=1000000;
    while(lo<hi) {const auto mid=lo+(hi-lo)/2;limits={};limits.maximumItems=mid;
        if(C::encode(input,limits))hi=mid;else lo=mid+1;}
    REQUIRE(lo>400);limits={};limits.maximumItems=lo;REQUIRE(C::decode(bytes,limits).known());
    --limits.maximumItems;CHECK(C::decode(bytes,limits).error.code==C::ErrorCode::ResourceLimit);
    limits={};limits.maximumItems=1000001;CHECK(C::encode(input,limits).error.code==C::ErrorCode::InvalidLimits);
    limits={};limits.maximumDepth=17;CHECK(C::decode(bytes,limits).status==C::Status::Refused);
    CHECK(input==before);
    auto child=unpack(at(input,3).payload);at(child,20).payload=scalar(std::numeric_limits<std::uint64_t>::max(),8);
    auto bad=input;putChild(bad,3,child);rejected(bad,C::ErrorCode::Malformed);
    child=unpack(at(input,3).payload);at(child,24).payload=hex("0100000000000000ffffffffffffffff");
    bad=input;putChild(bad,3,child);rejected(bad,C::ErrorCode::Malformed);
}

TEST_CASE("Full embedded model canonical deformation is required and exact not inferred", "[embedded-model][headless]") {
    auto r=model();auto source=unpack(at(r,1).payload);
    auto deformation=unpack(at(source,18).payload);
    at(deformation,1).payload=scalar(0x7ff800000000bad0ULL,8);
    at(deformation,2).payload=scalar(0x8000000000000000ULL,8);
    at(deformation,3).payload=scalar(0xfff0000000000000ULL,8);
    at(deformation,4).payload=scalar(0x0000000000000001ULL,8);
    putChild(source,18,deformation);putChild(r,1,source);
    const auto read=C::decode(packet(r));REQUIRE(read.known());CHECK(*read.model==r);
    const auto encoded=C::encode(r);REQUIRE(encoded);CHECK(*encoded.bytes==packet(r));
    auto original=unpack(at(r,2).payload);CHECK_FALSE(std::any_of(original.begin(),original.end(),[](const auto &v){return v.id==18;}));
    original.push_back(rec(18,deformation));putChild(r,2,original);REQUIRE(C::encode(r));
    for(const auto id:{1,2,3,4}) {
        auto malformed=deformation;at(malformed,std::uint16_t(id)).payload.pop_back();
        putChild(source,18,malformed);putChild(r,1,source);rejected(r,C::ErrorCode::InvalidField);
        malformed=deformation;malformed.erase(malformed.begin()+id-1);
        putChild(source,18,malformed);putChild(r,1,source);rejected(r,C::ErrorCode::MissingField);
    }
    source.erase(std::remove_if(source.begin(),source.end(),[](const auto &v){return v.id==18;}),source.end());
    putChild(r,1,source);rejected(r,C::ErrorCode::MissingField);
}

TEST_CASE("Full embedded model validates complete current OpenType identity without resolving resources", "[embedded-model][headless]") {
    R identity{str(1,QStringLiteral("lc-amiri-1.003-ot-v1")),
        f(2,K::NullableBytes,nullable(B(64,'a'))),f(3,K::U32,scalar(0,4)),
        str(4,QStringLiteral("lc-cell9-v1")),str(5,QStringLiteral("arabic-default-v1")),
        str(6,QStringLiteral("harfbuzz/12.0"))};
    const auto original=model();
    auto withIdentity=[&original](const R &id) {
        auto r=original;auto source=unpack(at(r,1).payload);auto profile=unpack(at(source,16).payload);
        putChild(profile,3,{rec(1,{u8(1,1),rec(2,id)}),f(2,K::NullableBytes,nullable({'a','r'})),u8(3,3)});
        putChild(source,16,profile);putChild(r,1,source);return r;
    };
    const auto good=withIdentity(identity);REQUIRE(C::encode(good));REQUIRE(C::decode(packet(good)).known());
    for(const std::uint16_t id:{1,2,3,4,5,6}) {
        auto bad=identity;bad.erase(std::remove_if(bad.begin(),bad.end(),[id](const auto &v){return v.id==id;}),bad.end());
        rejected(withIdentity(bad),C::ErrorCode::MissingField);
        bad=identity;
        if(id==2)at(bad,id).payload=nullable(B(64,'A'));
        else if(id==3)at(bad,id).payload=scalar(32768,4);
        else at(bad,id).payload=utf16(QStringLiteral("/absolute/path"));
        rejected(withIdentity(bad),C::ErrorCode::InvalidField);
    }
    auto bad=good;auto source=unpack(at(bad,1).payload);auto profile=unpack(at(source,16).payload);
    putChild(profile,3,{rec(1,{u8(1,1)})});putChild(source,16,profile);putChild(bad,1,source);
    rejected(bad,C::ErrorCode::InvalidField);
    putChild(profile,3,{rec(1,{u8(1),rec(2,identity)})});putChild(source,16,profile);putChild(bad,1,source);
    rejected(bad,C::ErrorCode::InvalidField);
}

TEST_CASE("Full embedded model owns accepted and inert raw bytes independently of input", "[embedded-model][headless]") {
    auto r=model();const auto snapshot=r;auto encoded=C::encode(r);REQUIRE(encoded);
    auto input=*encoded.bytes;const auto read=C::decode(input);REQUIRE(read.known());
    std::fill(input.begin(),input.end(),0);r.clear();CHECK(*read.model==snapshot);
    CHECK(*encoded.bytes==packet(snapshot));
    auto unknown=packet(snapshot);unknown[4]=99;const auto retained=C::decode(unknown);REQUIRE(retained.raw);
    const auto raw=*retained.raw;unknown.clear();CHECK(*retained.raw==raw);
    for(const bool null:{false,true}) {
        auto candidate=model();auto source=unpack(at(candidate,1).payload);at(source,1).payload=utf16(QStringLiteral(""));
        source.insert(source.begin()+4,f(5,K::NullableBytes,nullable({},null)));putChild(candidate,1,source);
        const auto decoded=C::decode(packet(candidate));REQUIRE(decoded.known());
        CHECK(at(unpack(at(*decoded.model,1).payload),5).payload==nullable({},null));
    }
    C::Limits l;l.maximumBytes=63;CHECK(C::decode({},l).status==C::Status::Refused);
    l={};l.maximumBytes=64u*1024u*1024u+1;CHECK(C::encode(snapshot,l).error.code==C::ErrorCode::InvalidLimits);
    l={};l.maximumDepth=0;CHECK(C::decode({},l).status==C::Status::Refused);
    auto packed=C::packRecord({f(2,K::Bytes,{}),f(1,K::Bytes,{})});CHECK_FALSE(packed);
    l={};l.maximumItems=0;CHECK_FALSE(C::packRecordArray({snapshot},l));
}

TEST_CASE("Embedded model framing builders expose local bounds not nested acceptance", "[embedded-model][headless]") {
    const auto input=model();C::Limits limits;
    limits.maximumItems=input.size();limits.maximumDepth=1;
    const auto local=C::packRecord(input,limits);REQUIRE(local);CHECK(*local.bytes==fields(input));
    CHECK(C::encode(input,limits).error.code==C::ErrorCode::ResourceLimit);
    CHECK(C::decode(packet(input),limits).error.code==C::ErrorCode::ResourceLimit);
    limits.maximumItems=input.size()+1;
    const auto localArray=C::packRecordArray({input},limits);REQUIRE(localArray);
    CHECK(*localArray.bytes==array({fields(input)}));
    CHECK(C::encode(input,limits).error.code==C::ErrorCode::ResourceLimit);
    --limits.maximumItems;CHECK_FALSE(C::packRecordArray({input},limits));
    limits={};limits.maximumBytes=local.bytes->size()-1;CHECK_FALSE(C::packRecord(input,limits));
    // Nested invalid native data is likewise outside the framing-only contract.
    auto r=input;auto child=unpack(at(r,3).payload);at(child,100).payload.pop_back();putChild(r,3,child);
    REQUIRE(C::packRecord(r));CHECK(C::encode(r).error.code==C::ErrorCode::InvalidField);
}
