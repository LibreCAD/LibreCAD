/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "drw_entities.h"
#include "drw_objects.h"
#include "intern/drw_textcodec.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgreader.h"
#include "intern/dxfreader.h"
#include "lc_drwnullinterface.h"
#include "libdxfrw.h"

struct LC_DxfReadProbeTestAccess {
    static void afterProbe(dxfRW& reader, std::function<void()> hook) {
        reader.m_testAfterProbe = std::move(hook);
    }
};

namespace {
using Tags = std::vector<std::shared_ptr<DRW_Variant>>;

class Entity final : public DRW_Line {
public:
    using DRW_Line::parseCode;
};
class Object final : public DRW_Dictionary {
public:
    using DRW_Dictionary::parseCode;
};

class CommonObject final : public DRW_Layer {
public:
    bool readCommon(dwgBuffer& input) {
        return DRW_TableEntry::parseDwg(DRW::AC1027, &input, nullptr, 0);
    }
};

struct EedFixture {
    Tags tags;
    std::vector<DRW_Entity::PendingHandleRef> apps, layers;
    std::size_t text(int code, std::string value) {
        const auto index = tags.size();
        tags.push_back(std::make_shared<DRW_Variant>(code, std::move(value)));
        return index;
    }
    void app(const char* name, std::uint32_t handle) {
        apps.push_back({text(1001, name), handle});
    }
    void layer(std::uint32_t handle) {
        layers.push_back({text(1003, "Layer"), handle});
    }
    std::vector<DRW_Variant*> raw() const {
        std::vector<DRW_Variant*> result;
        for (const auto& tag : tags) result.push_back(tag.get());
        return result;
    }
};

std::vector<DwgEedChunk> decodeCommon(const EedFixture& fixture, CommonObject& object) {
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1027, false);
    const auto raw = fixture.raw();
    dwgBufferW packets(&codec);
    REQUIRE(DRW_Entity::encodeDwgEed(DRW::AC1027, raw, fixture.apps, fixture.layers, 30, packets));
    dwgBuffer packetReader(packets.data().data(), packets.data().size(), &codec);
    std::vector<DwgEedChunk> chunks;
    REQUIRE(readDwgEed(DRW::AC1027, packetReader, chunks));

    dwgBufferW body(&codec);
    body.putObjType(DRW::AC1027, DRW::DwgLayerObjectType);
    dwgHandle own;
    own.code = 0; own.ref = 0x71;
    body.putHandle(own);
    // Encode directly into the current bit cursor: copying padded packet bytes
    // would insert padding before the common flags after the EED terminator.
    REQUIRE(DRW_Entity::encodeDwgEed(DRW::AC1027, raw, fixture.apps, fixture.layers, 30, body));
    body.putBitLong(0); // reactors
    body.putBit(1);     // xdictionary missing
    body.putBit(0);     // no DataStorage
    REQUIRE(body.isGood());
    // A preexisting item makes every expected reference index nonzero-based.
    REQUIRE(object.addExtData(std::make_unique<DRW_Variant>(1070, std::int32_t{17})));
    dwgBuffer input(body.data().data(), body.data().size(), &codec);
    REQUIRE(object.readCommon(input));
    CHECK(input.isGood());
    CHECK(object.handle == 0x71);
    CHECK(object.extData.front()->code() == 1070);
    CHECK(object.extData.front()->i_val() == 17);
    return chunks;
}

void checkLayer(const CommonObject& object, std::size_t resolution,
                std::size_t index, std::uint32_t handle) {
    REQUIRE(resolution < object.pendingLayerRefResolutions.size());
    const auto& ref = object.pendingLayerRefResolutions[resolution];
    CHECK(ref.indexInExtData == index);
    CHECK(ref.handleRef == handle);
    REQUIRE(index < object.extData.size());
    CHECK(object.extData[index]->code() == 1003);
    CHECK(object.extData[index]->isLayerRef());
}

// Literal wire records, independent of the production caret encoder.
std::string record(int code, std::string_view value, bool binary) {
    if (!binary) return std::to_string(code) + "\n" + std::string(value) + "\n";
    std::string result;
    result.push_back(static_cast<char>(code & 0xff));
    result.push_back(static_cast<char>((code >> 8) & 0xff));
    if (code == 1004) {
        REQUIRE(value.size() <= 127);
        result.push_back(static_cast<char>(value.size()));
        result.append(value);
    } else {
        result.append(value);
        result.push_back('\0');
    }
    return result;
}

std::unique_ptr<dxfReader> readerFor(std::istream& stream, bool binary,
                                     DRW::Version version = DRW::AC1018) {
    std::unique_ptr<dxfReader> reader;
    if (binary) reader = std::make_unique<dxfReaderBinary>(&stream);
    else reader = std::make_unique<dxfReaderAscii>(&stream);
    std::string versionName;
    for (const auto& [name, id] : DRW::dwgVersionStrings) {
        if (id == version) {
            versionName = name;
            break;
        }
    }
    REQUIRE(!versionName.empty());
    reader->setVersion(versionName, true);
    reader->setCodePage("ANSI_1252");
    return reader;
}

std::string hex(std::string_view value) {
    const char digits[] = "0123456789ABCDEF";
    std::string result;
    for (const unsigned char c : value) {
        result.push_back(digits[c >> 4]);
        result.push_back(digits[c & 15]);
    }
    return result;
}

class OutputDirectory final {
public:
    OutputDirectory() {
        static std::atomic<unsigned> serial{0};
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() /
            ("lc-xdata-codec-" + std::to_string(stamp) + "-" + std::to_string(serial++));
        REQUIRE(std::filesystem::create_directory(path));
    }
    ~OutputDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
    OutputDirectory(const OutputDirectory&) = delete;
    OutputDirectory& operator=(const OutputDirectory&) = delete;
    std::filesystem::path path;
};

Tags payload(const std::string& foreign, const std::string& owned) {
    // Native schema spellings/types are hand-authored; this test qualifies
    // transport, not cache adoption, feature geometry or application wiring.
    return {
        std::make_shared<DRW_Variant>(1001, std::string("FOREIGN_CODEC")),
        std::make_shared<DRW_Variant>(1000, foreign),
        std::make_shared<DRW_Variant>(1003, std::string("0")),
        std::make_shared<DRW_Variant>(1004, std::vector<std::uint8_t>{0, 9, 10, 13, 94, 32, 255}),
        std::make_shared<DRW_Variant>(1005, std::string("A0")),
        std::make_shared<DRW_Variant>(1001, std::string("LibreCad")),
        std::make_shared<DRW_Variant>(1000, std::string("LC_OFFSET_CURVE")),
        std::make_shared<DRW_Variant>(1002, std::string("{")),
        std::make_shared<DRW_Variant>(1070, std::int32_t{1}),
        std::make_shared<DRW_Variant>(1000, std::string("ELLIPSE")),
        std::make_shared<DRW_Variant>(1011, DRW_Coord{0, 0, 0}),
        std::make_shared<DRW_Variant>(1012, DRW_Coord{1, 0, 0}),
        std::make_shared<DRW_Variant>(1040, 1.),
        std::make_shared<DRW_Variant>(1040, 0.),
        std::make_shared<DRW_Variant>(1040, .1),
        std::make_shared<DRW_Variant>(1070, std::int32_t{0}),
        std::make_shared<DRW_Variant>(1041, .25),
        std::make_shared<DRW_Variant>(1040, .0001),
        std::make_shared<DRW_Variant>(1000, owned),
        std::make_shared<DRW_Variant>(1002, std::string("}"))};
}

template<typename Actual>
void sameTags(const Actual& actual, const Tags& expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        CAPTURE(i);
        REQUIRE(actual[i]);
        REQUIRE(actual[i]->code() == expected[i]->code());
        if (expected[i]->code() == 1004) {
            // Both binary and ASCII DXF readers expose binary chunks as hex.
            REQUIRE(actual[i]->type() == DRW_Variant::STRING);
            const auto* bytes = expected[i]->binary();
            const std::string_view raw(reinterpret_cast<const char*>(bytes->data()), bytes->size());
            CHECK(*actual[i]->content.s == hex(raw));
            continue;
        }
        REQUIRE(actual[i]->type() == expected[i]->type());
        switch (expected[i]->type()) {
        case DRW_Variant::STRING: CHECK(*actual[i]->content.s == *expected[i]->content.s); break;
        case DRW_Variant::INTEGER: CHECK(actual[i]->i_val() == expected[i]->i_val()); break;
        case DRW_Variant::DOUBLE: CHECK(actual[i]->d_val() == expected[i]->d_val()); break;
        case DRW_Variant::COORD:
            CHECK(actual[i]->coord()->x == expected[i]->coord()->x);
            CHECK(actual[i]->coord()->y == expected[i]->coord()->y);
            CHECK(actual[i]->coord()->z == expected[i]->coord()->z);
            break;
        default: FAIL("Unexpected literal fixture type");
        }
    }
}

class FileInterface final : public lc::test::NullDrwInterface {
public:
    dxfRW* writer = nullptr;
    Tags data;
    Tags objectData;
    std::vector<std::string> applicationNames{"FOREIGN_CODEC", "LibreCad"};
    std::string layerName = "0";
    bool wroteLine = false, wroteObject = false, wroteApps = true;
    bool wroteLayer = false;
    std::vector<DRW_AppId> apps;
    std::vector<DRW_Layer> layers;
    std::vector<DRW_Line> lines;
    std::vector<DRW_DictionaryVar> objects;
    void writeHeader(DRW_Header& header) override {
        header.addStr("$DWGCODEPAGE", "ANSI_1252", 3);
    }
    void writeAppId() override {
        for (const auto& name : applicationNames) {
            DRW_AppId app;
            app.name = name;
            wroteApps = writer->writeAppId(&app) && wroteApps;
        }
    }
    void writeLayers() override {
        if (layerName == "0") return;
        DRW_Layer layer;
        layer.name = layerName;
        wroteLayer = writer->writeLayer(&layer);
    }
    void writeEntities() override {
        DRW_Line line;
        line.layer = layerName;
        line.basePoint = {0, 0, 0};
        line.secPoint = {1, 0, 0};
        line.extData = data;
        wroteLine = writer->writeLine(&line);
    }
    void writeObjects() override {
        DRW_DictionaryVar object;
        object.handle = writer->allocHandle();
        object.m_value = "ordinary^I\\P%%d";
        for (const auto& tag : objectData) {
            REQUIRE(object.addExtData(std::make_unique<DRW_Variant>(*tag)));
        }
        wroteObject = writer->writeDictionaryVar(&object);
    }
    void addAppId(const DRW_AppId& app) override { apps.push_back(app); }
    void addLayer(const DRW_Layer& layer) override { layers.push_back(layer); }
    void addLine(const DRW_Line& line) override { lines.push_back(line); }
    void addDictionaryVar(const DRW_DictionaryVar& object) override { objects.push_back(object); }
};
}

TEST_CASE("DXF XDATA caret codec has independent canonical control and literal expectations",
          "[libdxfrw][xdata][codec]") {
    CHECK(DRW_TextCodec::encodeDxfXDataString("a\tb\nc\rd^J^ ") == "a^Ib^Jc^Md^ J^  ");
    CHECK(DRW_TextCodec::decodeDxfXDataString("a^Ib^Jc^Md^ J^  ") == "a\tb\nc\rd^J^ ");
    CHECK(DRW_TextCodec::encodeDxfXDataString("^^") == "^ ^ ");
    CHECK(DRW_TextCodec::decodeDxfXDataString("^ ^ ") == "^^");
    CHECK(DRW_TextCodec::decodeDxfXDataString("^^") == std::string(1, '\x1e'));
    CHECK(DRW_TextCodec::decodeDxfXDataString("^@^A^_^ ") == std::string("\0\1\x1f^", 4));
    CHECK(DRW_TextCodec::decodeDxfXDataString("tail^ ^a^!^\x7f") == "tail^^a^!^\x7f");
    CHECK(DRW_TextCodec::encodeDxfXDataString({}).empty());
    CHECK(DRW_TextCodec::decodeDxfXDataString({}).empty());
    CHECK(DRW_TextCodec::decodeDxfXDataString("^") == "^");
}

TEST_CASE("DXF XDATA caret codec round trips every byte without text presentation semantics",
          "[libdxfrw][xdata][codec]") {
    std::string input;
    for (unsigned value = 0; value < 256; ++value) input.push_back(static_cast<char>(value));
    const auto encoded = DRW_TextCodec::encodeDxfXDataString(input);
    CHECK(encoded.size() == input.size() + 32 + 1);
    for (const unsigned char value : encoded) CHECK(value >= 0x20);
    CHECK(DRW_TextCodec::decodeDxfXDataString(encoded) == input);
    for (const auto* literal : {"\\P\\~", "{\\fArial;kept}", "%%d %%c %%p", "\\U+0041", "\\M+18140"}) {
        CHECK(DRW_TextCodec::encodeDxfXDataString(literal) == literal);
        CHECK(DRW_TextCodec::decodeDxfXDataString(literal) == literal);
    }
}

TEST_CASE("DXF XDATA reader converts source codepage before ASCII-only code 1000 carets",
          "[libdxfrw][xdata][codec]") {
    for (bool binary : {false, true}) for (auto version : {DRW::AC1018, DRW::AC1032}) {
        CAPTURE(binary, static_cast<int>(version));
        const std::string raw = version == DRW::AC1018 ? std::string("caf\xe9^I^ J") : std::string("caf\xc3\xa9^I^ J");
        const std::string unicode = "caf\xc3\xa9^I^ J";
        const auto input = record(1000, raw, binary);
        std::istringstream stream(input);
        auto reader = readerFor(stream, binary, version);
        int code = 0;
        REQUIRE(reader->readRec(&code)); REQUIRE(code == 1000);
        CHECK(reader->getRawString() == raw);
        CHECK(reader->getUtf8String() == unicode);
        CHECK(reader->getUtf8XDataString(code) == (binary ? unicode : "caf\xc3\xa9\t^J"));
    }
    // This ordering is deliberate; source Unicode escapes can reveal a caret.
    std::istringstream stream(record(1000, "\\U+005EI", false));
    auto reader = readerFor(stream, false);
    int code = 0;
    REQUIRE(reader->readRec(&code));
    CHECK(reader->getUtf8XDataString(code) == "\t");
}

TEST_CASE("DXF XDATA payload decoding is declared while symbol references follow text recovery policy",
          "[libdxfrw][xdata][codec][policy]") {
    const std::string rawText = "caf\xc3\xa9";
    for (auto version : {DRW::AC1018, DRW::AC1032}) {
        // The same UTF-8-looking bytes mean U+00C3 U+00A9 under CP1252.
        const std::string declaredText = version == DRW::AC1018
            ? "caf\xc3\x83\xc2\xa9" : rawText;
        for (auto policy : {DRW::DxfTextEncoding::Declared,
                            DRW::DxfTextEncoding::LegacyLibreCadUtf8}) {
            CAPTURE(static_cast<int>(version), static_cast<int>(policy));
            DRW_TextCodec codec;
            codec.setDxfTextEncoding(policy);
            codec.setVersion(version, true);
            codec.setCodePage("ANSI_1252", true);
            CHECK(codec.toUtf8Declared(rawText) == declaredText);
            const bool recovery = version == DRW::AC1018
                && policy == DRW::DxfTextEncoding::LegacyLibreCadUtf8;
            const std::string symbolText = recovery ? rawText : declaredText;
            CHECK(codec.toUtf8(rawText) == symbolText);

            for (bool binary : {false, true}) {
                CAPTURE(binary);
                const std::string input = record(1001, rawText + "^I", binary)
                    + record(1000, rawText + "^I^ J", binary)
                    + record(1003, rawText + "^I", binary)
                    + record(1002, "{", binary)
                    + record(1004, binary ? std::string("^\0\xc3\xa9\xff", 5)
                                          : "5E00C3A9FF", binary)
                    + record(1005, "aBcD", binary) + record(1002, "}", binary);
                const std::vector<std::pair<int, std::string>> expected{
                    {1001, symbolText + "^I"},
                    {1000, declaredText + (binary ? "^I^ J" : "\t^J")},
                    {1003, symbolText + "^I"}, {1002, "{"},
                    {1004, "5E00C3A9FF"}, {1005, "aBcD"}, {1002, "}"}};
                std::istringstream stream(input);
                auto reader = readerFor(stream, binary, version);
                reader->setDxfTextEncoding(policy);
                Entity entity;
                Object object;
                for (const auto& [expectedCode, text] : expected) {
                    int code = 0;
                    REQUIRE(reader->readRec(&code));
                    REQUIRE(code == expectedCode);
                    CHECK(reader->getUtf8XDataString(code) == text);
                    REQUIRE(entity.parseCode(code, reader));
                    REQUIRE(object.parseCode(code, reader));
                    REQUIRE(entity.extData.back()->type() == DRW_Variant::STRING);
                    REQUIRE(object.extData.back()->type() == DRW_Variant::STRING);
                    CHECK(*entity.extData.back()->content.s == text);
                    CHECK(*object.extData.back()->content.s == text);
                }
                CHECK(entity.extData.size() == expected.size());
                CHECK(object.extData.size() == expected.size());
            }
        }
    }
}

TEST_CASE("DXF XDATA application layer and opaque tags do not acquire caret or presentation decoding",
          "[libdxfrw][xdata][codec]") {
    for (bool binary : {false, true}) {
        for (int code : {1001, 1003}) {
            const std::string raw = "caf\xe9^I\\P%%d";
            std::istringstream stream(record(code, raw, binary));
            auto reader = readerFor(stream, binary);
            int actual = 0;
            REQUIRE(reader->readRec(&actual)); REQUIRE(actual == code);
            CHECK(reader->getUtf8XDataString(code) == "caf\xc3\xa9^I\\P%%d");
        }
        std::istringstream stream(record(1002, "\\U+0041^I", binary) +
            record(1004, binary ? std::string("^I\0\xff", 4) : "5E4900FF", binary) +
            record(1005, "DEADbeef", binary));
        auto reader = readerFor(stream, binary);
        int code = 0;
        REQUIRE(reader->readRec(&code)); REQUIRE(code == 1002);
        CHECK(reader->getUtf8XDataString(code) == "\\U+0041^I");
        REQUIRE(reader->readRec(&code)); REQUIRE(code == 1004);
        CHECK(reader->getUtf8XDataString(code) == "5E4900FF");
        REQUIRE(reader->readRec(&code)); REQUIRE(code == 1005);
        CHECK(reader->getUtf8XDataString(code) == "DEADbeef");
    }
}

TEST_CASE("DXF entity and object XDATA share caret conversion and preserve raw nontext tags",
          "[libdxfrw][xdata][codec]") {
    for (bool binary : {false, true}) {
        std::string input = record(1001, "APP^I", binary) + record(1000, "value^I^ J", binary) +
            record(1003, "caf\xe9^I", binary) + record(1002, "{", binary) +
            record(1004, binary ? std::string("^I", 2) : "5E49", binary) +
            record(1005, "aBcD", binary) + record(1002, "}", binary);
        std::istringstream stream(input);
        auto reader = readerFor(stream, binary);
        Entity entity;
        Object object;
        int code = 0;
        while (reader->readRec(&code)) {
            REQUIRE(entity.parseCode(code, reader));
            REQUIRE(object.parseCode(code, reader));
        }
        REQUIRE(entity.extData.size() == 7); REQUIRE(object.extData.size() == 7);
        const std::vector<std::string> expected{"APP^I", binary ? "value^I^ J" : "value\t^J",
            "caf\xc3\xa9^I", "{", "5E49", "aBcD", "}"};
        for (std::size_t i = 0; i < expected.size(); ++i) {
            CHECK(*entity.extData[i]->content.s == expected[i]);
            CHECK(*object.extData[i]->content.s == expected[i]);
        }
    }
}

TEST_CASE("DXF XDATA text admits 255 source bytes and refuses 256 before conversion or publication",
          "[libdxfrw][xdata][codec][limits]") {
    for (bool binary : {false, true}) for (int code : {1000, 1001, 1003}) {
        CAPTURE(binary, code);
        // CP1252 expands to 510 UTF-8 bytes, but the source string is legal.
        const auto input = record(code, std::string(255, '\xe9'), binary) +
                           record(code, std::string(256, '\xe9'), binary);
        std::istringstream stream(input);
        auto reader = readerFor(stream, binary);
        Entity entity;
        Object object;
        int actual = 0;
        REQUIRE(reader->readRec(&actual));
        CHECK(reader->getRawString().size() == 255);
        REQUIRE(entity.parseCode(actual, reader)); REQUIRE(object.parseCode(actual, reader));
        REQUIRE(entity.extData.size() == 1); REQUIRE(object.extData.size() == 1);
        CHECK(entity.extData.front()->content.s->size() == 510);
        CHECK(object.extData.front()->content.s->size() == 510);
        const auto* entityTag = entity.extData.front().get();
        const auto* objectTag = object.extData.front();
        REQUIRE(reader->readRec(&actual));
        CHECK(reader->getRawString().size() == 256);
        CHECK_FALSE(entity.parseCode(actual, reader)); CHECK_FALSE(object.parseCode(actual, reader));
        CHECK(entity.extData.size() == 1); CHECK(object.extData.size() == 1);
        CHECK(entity.extData.front().get() == entityTag); CHECK(object.extData.front() == objectTag);
    }
}

TEST_CASE("ASCII XDATA caret expansion is bounded by encoded rather than decoded string bytes",
          "[libdxfrw][xdata][codec][limits]") {
    const auto exact = DRW_TextCodec::encodeDxfXDataString(std::string(127, '^') + "x");
    const auto above = DRW_TextCodec::encodeDxfXDataString(std::string(128, '\t'));
    REQUIRE(exact.size() == 255); REQUIRE(above.size() == 256);
    std::istringstream stream(record(1000, exact, false) + record(1000, above, false));
    auto reader = readerFor(stream, false);
    Entity entity;
    Object object;
    int code = 0;
    REQUIRE(reader->readRec(&code));
    REQUIRE(entity.parseCode(code, reader)); REQUIRE(object.parseCode(code, reader));
    CHECK(*entity.extData.front()->content.s == std::string(127, '^') + "x");
    CHECK(*object.extData.front()->content.s == std::string(127, '^') + "x");
    REQUIRE(reader->readRec(&code));
    CHECK_FALSE(entity.parseCode(code, reader)); CHECK_FALSE(object.parseCode(code, reader));
    CHECK(entity.extData.size() == 1); CHECK(object.extData.size() == 1);
}

TEST_CASE("Actual DXF ASCII and binary writers round trip foreign and native-schema tail text",
          "[libdxfrw][xdata][codec][roundtrip]") {
    OutputDirectory directory;
    const std::string foreign = "foreign\t^I caf\xc3\xa9";
    const std::string owned = "owned\x01\t^J^^ caf\xc3\xa9";
    const auto expected = payload(foreign, owned);
    // Objects carry the same text and opaque values, not an entity definition.
    const Tags objectExpected{expected[0], expected[1], expected[2], expected[3], expected[4],
        expected[5], expected[7], expected[18], expected[19]};
    for (bool binary : {false, true}) for (auto version : {DRW::AC1018, DRW::AC1032}) {
        CAPTURE(binary, static_cast<int>(version));
        const auto path = directory.path / (std::to_string(static_cast<int>(version)) + (binary ? "-binary.dxf" : "-ascii.dxf"));
        FileInterface source;
        source.data = expected;
        source.objectData = objectExpected;
        dxfRW output(path.string().c_str());
        source.writer = &output;
        REQUIRE(output.write(&source, version, binary));
        CHECK(source.wroteApps); CHECK(source.wroteLine); CHECK(source.wroteObject);
        FileInterface captured;
        dxfRW input(path.string().c_str());
        REQUIRE(input.read(&captured, false));
        REQUIRE(captured.lines.size() == 1); REQUIRE(captured.objects.size() == 1);
        sameTags(captured.lines.front().extData, expected);
        sameTags(captured.objects.front().extData, objectExpected);
        CHECK(captured.objects.front().m_value == "ordinary^I\\P%%d");
        if (!binary) {
            std::ifstream file(path, std::ios::binary);
            REQUIRE(file.good());
            const std::string bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
            CHECK(bytes.find("foreign^I^ I caf") != std::string::npos);
            CHECK(bytes.find("owned^A^I^ J^ ^  caf") != std::string::npos);
        }
    }
}

#ifndef _WIN32
TEST_CASE("DXF detection and parsing use the same opened file", "[libdxfrw][read][source-identity]") {
    OutputDirectory directory;
    for (const bool originalBinary : {false, true}) for (const bool replacementBinary : {false, true}) {
        CAPTURE(originalBinary, replacementBinary);
        const auto sourcePath = directory.path / "source.dxf";
        const auto replacementPath = directory.path / "replacement.dxf";
        const auto savedPath = directory.path / "opened-original.dxf";
        std::error_code error;
        std::filesystem::remove(sourcePath, error); std::filesystem::remove(replacementPath, error);
        std::filesystem::remove(savedPath, error);
        const auto writeFile = [&](const std::filesystem::path& path, const char* layer, bool binary) {
            FileInterface source; source.layerName = layer;
            dxfRW writer(path.string().c_str()); source.writer = &writer;
            REQUIRE(writer.write(&source, DRW::AC1032, binary)); REQUIRE(source.wroteLine);
        };
        writeFile(sourcePath, "ORIGINAL", originalBinary);
        writeFile(replacementPath, "REPLACEMENT", replacementBinary);
        dxfRW reader(sourcePath.string().c_str()); FileInterface captured; bool swapped = false;
        LC_DxfReadProbeTestAccess::afterProbe(reader, [&] {
            std::filesystem::rename(sourcePath, savedPath, error); REQUIRE_FALSE(error);
            std::filesystem::rename(replacementPath, sourcePath, error); REQUIRE_FALSE(error);
            swapped = true;
        });
        REQUIRE(reader.read(&captured, false)); REQUIRE(swapped);
        REQUIRE(captured.lines.size() == 1); CHECK(captured.lines.front().layer == "ORIGINAL");
        CHECK(reader.isBinary() == originalBinary);
    }
}
#endif

TEST_CASE("DXF stream reader accepts ASCII and binary at a nonzero position", "[libdxfrw][read][stream]") {
    OutputDirectory directory;
    dxfRW reader("");
    for (const auto version : {DRW::AC1009, DRW::AC1032}) for (const bool binary : {false, true}) {
        CAPTURE(static_cast<int>(version), binary);
        const auto path = directory.path / "stream.dxf";
        FileInterface source; source.layerName = "STREAM_LAYER";
        dxfRW writer(path.string().c_str()); source.writer = &writer;
        REQUIRE(writer.write(&source, version, binary)); REQUIRE(source.wroteLine);
        std::ifstream file(path, std::ios::binary); REQUIRE(file.good());
        const std::string bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        const std::string prefix("prefix\0", 7);
        std::istringstream input(prefix + bytes, std::ios::in | std::ios::binary);
        input.seekg(static_cast<std::streamoff>(prefix.size())); REQUIRE(input.good());
        FileInterface captured;
        REQUIRE(reader.readStream(&captured, false, input));
        REQUIRE(captured.lines.size() == 1); CHECK(captured.lines.front().layer == "STREAM_LAYER");
        CHECK(reader.isBinary() == binary); CHECK(reader.getSourceVersion() == version);
    }
}

TEST_CASE("DXF stream reader rejects nonseekable and invalid input", "[libdxfrw][read][stream]") {
    struct ForwardOnly final : std::stringbuf {
        explicit ForwardOnly(const std::string& data) : std::stringbuf(data) {}
        pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode) override { return pos_type(off_type(-1)); }
        pos_type seekpos(pos_type, std::ios_base::openmode) override { return pos_type(off_type(-1)); }
    } buffer("0\nEOF\n");
    std::istream input(&buffer); dxfRW reader(""); FileInterface captured;
    CHECK_FALSE(reader.readStream(&captured, false, input)); CHECK(reader.getError() == DRW::BAD_OPEN);
    CHECK(captured.lines.empty());
    std::istringstream empty;
    CHECK_FALSE(reader.readStream(&captured, false, empty)); CHECK(captured.lines.empty());
    std::istringstream valid("0\nEOF\n");
    CHECK_FALSE(reader.readStream(nullptr, false, valid)); CHECK(reader.getError() == DRW::BAD_UNKNOWN);
}

TEST_CASE("DXF stream reader preserves CRLF and newline-less ASCII handling", "[libdxfrw][read][stream]") {
    const std::string text = "0\r\nSECTION\r\n2\r\nENTITIES\r\n0\r\nLINE\r\n8\r\n0\r\n"
        "10\r\n1.0\r\n20\r\n2.0\r\n11\r\n3.0\r\n21\r\n4.0\r\n0\r\nENDSEC\r\n0\r\nEOF";
    std::istringstream input(text, std::ios::in | std::ios::binary); dxfRW reader(""); FileInterface captured;
    REQUIRE(reader.readStream(&captured, false, input));
    REQUIRE(captured.lines.size() == 1); CHECK(captured.lines.front().basePoint.x == 1.0);
    CHECK(captured.lines.front().secPoint.x == 3.0); CHECK_FALSE(reader.isBinary());
}

TEST_CASE("Actual DXF XDATA symbol references resolve to decoded APPID and layer table identities",
          "[libdxfrw][xdata][codec][policy][references][roundtrip]") {
    OutputDirectory directory;
    const auto data = [](const std::string& app, const std::string& layer,
                         const std::string& text) -> Tags {
        return {std::make_shared<DRW_Variant>(1001, app),
            std::make_shared<DRW_Variant>(1000, text),
            std::make_shared<DRW_Variant>(1003, layer),
            std::make_shared<DRW_Variant>(1002, std::string("{")),
            std::make_shared<DRW_Variant>(1004, std::vector<std::uint8_t>{0, 94, 9, 194, 176, 255}),
            std::make_shared<DRW_Variant>(1005, std::string("aBcD")),
            std::make_shared<DRW_Variant>(1002, std::string("}"))};
    };
    for (auto version : {DRW::AC1018, DRW::AC1032}) for (bool binary : {false, true}) {
        CAPTURE(static_cast<int>(version), binary);
        // Both writers produce C2 B0. Under pre-R2007 CP1252 this is two
        // characters; legacy recovery instead reads the symbols as U+00B0.
        const std::string declaredSuffix = version == DRW::AC1018
            ? "\xc3\x82\xc2\xb0" : "\xc2\xb0";
        const std::string declaredApp = "APP" + declaredSuffix;
        const std::string declaredLayer = "L" + declaredSuffix;
        const std::string declaredPayload = "payload " + declaredSuffix + "\t^I";
        FileInterface source;
        source.applicationNames = {declaredApp};
        source.layerName = declaredLayer;
        source.data = data(declaredApp, declaredLayer, declaredPayload);
        source.objectData = source.data;
        const auto path = directory.path / ("references-" + std::to_string(static_cast<int>(version))
            + (binary ? "-binary.dxf" : "-ascii.dxf"));
        dxfRW output(path.string().c_str());
        source.writer = &output;
        REQUIRE(output.write(&source, version, binary));
        REQUIRE(source.wroteApps); REQUIRE(source.wroteLayer);
        REQUIRE(source.wroteLine); REQUIRE(source.wroteObject);
        {
            std::ifstream file(path, std::ios::binary);
            REQUIRE(file.good());
            const std::string bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
            CHECK(bytes.find("APP\xc2\xb0") != std::string::npos);
            CHECK(bytes.find("L\xc2\xb0") != std::string::npos);
        }
        for (auto policy : {DRW::DxfTextEncoding::Declared,
                            DRW::DxfTextEncoding::LegacyLibreCadUtf8}) {
            CAPTURE(static_cast<int>(policy));
            const bool recovery = version == DRW::AC1018
                && policy == DRW::DxfTextEncoding::LegacyLibreCadUtf8;
            const std::string expectedApp = recovery ? "APP\xc2\xb0" : declaredApp;
            const std::string expectedLayer = recovery ? "L\xc2\xb0" : declaredLayer;
            FileInterface captured;
            dxfRW input(path.string().c_str());
            input.setDxfTextEncoding(policy);
            REQUIRE(input.read(&captured, false));
            REQUIRE(captured.lines.size() == 1); REQUIRE(captured.objects.size() == 1);
            const auto appMatches = [&](const DRW_AppId& app) { return app.name == expectedApp; };
            const auto layerMatches = [&](const DRW_Layer& layer) { return layer.name == expectedLayer; };
            REQUIRE(std::count_if(captured.apps.begin(), captured.apps.end(), appMatches) == 1);
            REQUIRE(std::count_if(captured.layers.begin(), captured.layers.end(), layerMatches) == 1);
            const auto app = std::find_if(captured.apps.begin(), captured.apps.end(), appMatches);
            const auto layer = std::find_if(captured.layers.begin(), captured.layers.end(), layerMatches);
            REQUIRE(app != captured.apps.end()); REQUIRE(layer != captured.layers.end());
            REQUIRE(app->handle != 0); REQUIRE(layer->handle != 0);
            CHECK(captured.lines.front().layer == layer->name);
            const auto expected = data(expectedApp, expectedLayer, declaredPayload);
            sameTags(captured.lines.front().extData, expected);
            sameTags(captured.objects.front().extData, expected);

            // Resolve the imported references by exact table identity, not by
            // re-decoding the already logical strings or guessing a handle.
            const auto checkResolved = [&](const std::vector<DRW_Variant*>& raw) {
                REQUIRE(raw.size() == expected.size());
                REQUIRE(raw[0]->code() == 1001); REQUIRE(raw[2]->code() == 1003);
                REQUIRE(raw[0]->type() == DRW_Variant::STRING);
                REQUIRE(raw[2]->type() == DRW_Variant::STRING);
                REQUIRE(*raw[0]->content.s == app->name);
                REQUIRE(*raw[2]->content.s == layer->name);
                const std::vector<DRW_Entity::PendingHandleRef> appRefs{{0, app->handle}};
                const std::vector<DRW_Entity::PendingHandleRef> layerRefs{{2, layer->handle}};
                DRW_TextCodec codec;
                codec.setVersion(DRW::AC1027, false);
                dwgBufferW missingApp(&codec), missingLayer(&codec), encoded(&codec);
                CHECK_FALSE(DRW_Entity::encodeDwgEed(DRW::AC1027, raw, {}, layerRefs, 30, missingApp));
                CHECK_FALSE(DRW_Entity::encodeDwgEed(DRW::AC1027, raw, appRefs, {}, 30, missingLayer));
                REQUIRE(DRW_Entity::encodeDwgEed(DRW::AC1027, raw, appRefs, layerRefs, 30, encoded));
                dwgBuffer decoded(encoded.data().data(), encoded.data().size(), &codec);
                std::vector<DwgEedChunk> chunks;
                REQUIRE(readDwgEed(DRW::AC1027, decoded, chunks));
                REQUIRE(chunks.size() == 1);
                CHECK(chunks.front().appHandle == app->handle);
                REQUIRE(chunks.front().layerRefs.size() == 1);
                CHECK(chunks.front().layerRefs.front().itemIndex == 1);
                CHECK(chunks.front().layerRefs.front().handleRef == layer->handle);
                REQUIRE_FALSE(chunks.front().items.empty());
                CHECK(chunks.front().items.front().code() == 1000);
                REQUIRE(chunks.front().items.front().type() == DRW_Variant::STRING);
                CHECK(*chunks.front().items.front().content.s == declaredPayload);
            };
            std::vector<DRW_Variant*> lineTags;
            for (const auto& tag : captured.lines.front().extData) lineTags.push_back(tag.get());
            checkResolved(lineTags);
            checkResolved(captured.objects.front().extData);
        }
    }
}

TEST_CASE("XDATA caret transport does not conceal source Unicode and MIF ambiguities",
          "[libdxfrw][xdata][codec]") {
    for (const auto& literal : {std::string("\\U+0041"), std::string("\\M+18140")}) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1018, true);
        codec.setCodePage("ANSI_1252", true);
        const auto wire = codec.fromUtf8(DRW_TextCodec::encodeDxfXDataString(literal));
        std::istringstream stream(record(1000, wire, false));
        auto reader = readerFor(stream, false);
        int code = 0;
        REQUIRE(reader->readRec(&code));
        CHECK(reader->getUtf8XDataString(code) != literal);
    }
}

TEST_CASE("Common DWG objects coalesce one APPID across scalar and marker-brace packet rollovers",
          "[libdxfrw][xdata][codec][eed-packets][objects]") {
    for (bool markerBoundary : {false, true}) {
        CAPTURE(markerBoundary);
        EedFixture fixture;
        fixture.app("APP", 0x23);
        if (markerBoundary) {
            // 3+2*32747 + 3+2*15 = 65530. The marker fits in packet one;
            // its complete braced unit must begin in the next packet.
            fixture.text(1000, std::string(32747, 'x'));
            fixture.text(1000, "LC_OFFSET_CURVE");
            fixture.text(1002, "{");
            fixture.layer(0xb1);
            fixture.text(1000, "owned");
            fixture.text(1002, "}");
        } else {
            // One scalar fills all 65535 payload bytes, forcing the next
            // scalar and following layer reference into packet two.
            fixture.text(1000, std::string(32766, 'x'));
            fixture.text(1000, "after");
            fixture.layer(0xb1);
        }
        CommonObject object;
        const auto chunks = decodeCommon(fixture, object);
        REQUIRE(chunks.size() == 2);
        CHECK(chunks[0].appHandle == 0x23); CHECK(chunks[1].appHandle == 0x23);
        REQUIRE(object.pendingAppIdResolutions.size() == 1);
        CHECK(object.pendingAppIdResolutions.front().indexInExtData == 1);
        CHECK(object.pendingAppIdResolutions.front().handleRef == 0x23);
        CHECK(object.extData.size() == 1 + fixture.tags.size());
        std::size_t applications = 0;
        for (const auto* tag : object.extData) if (tag->code() == 1001) ++applications;
        CHECK(applications == 1);
        REQUIRE(object.pendingLayerRefResolutions.size() == 1);
        checkLayer(object, 0, markerBoundary ? 5 : 4, 0xb1);
        if (markerBoundary) {
            REQUIRE(object.extData.size() == 8);
            CHECK(object.extData[3]->code() == 1000);
            CHECK(*object.extData[3]->content.s == "LC_OFFSET_CURVE");
            CHECK(object.extData[4]->code() == 1002);
            CHECK(*object.extData[4]->content.s == "{");
            CHECK(*object.extData[6]->content.s == "owned");
            CHECK(*object.extData[7]->content.s == "}");
            CHECK(chunks[0].items.size() == 2); CHECK(chunks[1].items.size() == 4);
        } else {
            CHECK(*object.extData[3]->content.s == "after");
            CHECK(chunks[0].items.size() == 1); CHECK(chunks[1].items.size() == 2);
        }
    }
}

TEST_CASE("Common DWG objects coalesce adjacent equal APPID handles with corrected layer indices",
          "[libdxfrw][xdata][codec][eed-packets][objects]") {
    EedFixture fixture;
    fixture.app("APP", 0x23);
    fixture.text(1000, "first");
    fixture.layer(0xb1);
    fixture.app("APP", 0x23);
    fixture.text(1000, "second");
    fixture.layer(0xb2);
    CommonObject object;
    const auto chunks = decodeCommon(fixture, object);
    REQUIRE(chunks.size() == 2);
    CHECK(chunks[0].appHandle == chunks[1].appHandle);
    REQUIRE(object.pendingAppIdResolutions.size() == 1);
    CHECK(object.pendingAppIdResolutions.front().indexInExtData == 1);
    REQUIRE(object.extData.size() == 6); // prefix + one APPID + four values
    CHECK(*object.extData[2]->content.s == "first");
    CHECK(*object.extData[4]->content.s == "second");
    REQUIRE(object.pendingLayerRefResolutions.size() == 2);
    checkLayer(object, 0, 3, 0xb1);
    checkLayer(object, 1, 5, 0xb2);
}

TEST_CASE("Common DWG objects never merge APPIDs across a different application",
          "[libdxfrw][xdata][codec][eed-packets][objects]") {
    EedFixture fixture;
    fixture.app("APP_A", 0x23);
    fixture.text(1000, "first"); fixture.layer(0xb1);
    fixture.app("APP_B", 0x45);
    fixture.text(1000, "middle"); fixture.layer(0xb2);
    fixture.app("APP_A", 0x23);
    fixture.text(1000, "last"); fixture.layer(0xb3);
    CommonObject object;
    const auto chunks = decodeCommon(fixture, object);
    REQUIRE(chunks.size() == 3);
    REQUIRE(object.pendingAppIdResolutions.size() == 3);
    REQUIRE(object.extData.size() == 10);
    const std::vector<std::uint32_t> handles{0x23, 0x45, 0x23};
    for (std::size_t i = 0; i < handles.size(); ++i) {
        const auto& ref = object.pendingAppIdResolutions[i];
        CHECK(chunks[i].appHandle == handles[i]);
        CHECK(ref.handleRef == handles[i]);
        CHECK(ref.indexInExtData == 1 + 3*i);
        CHECK(object.extData[1 + 3*i]->code() == 1001);
        checkLayer(object, i, 3 + 3*i, static_cast<std::uint32_t>(0xb1 + i));
    }
}
