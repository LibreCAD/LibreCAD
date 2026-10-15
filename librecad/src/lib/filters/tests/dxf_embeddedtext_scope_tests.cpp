/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "libdxfrw.h"
#include "drw_interface.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <fstream>
#include <algorithm>

namespace {
using Tags = std::vector<std::shared_ptr<DRW_Variant>>;

class ScopeInterface final : public DRW_Interface {
public:
    dxfRW *writer = nullptr;
    int mode = 0; // ATTRIB, ATTDEF, and ATTDEF through writeAttrib dispatch.
    bool lastEntityWriteSucceeded = true;
    DRW_Attrib attribute;
    DRW_Attdef definition;
    std::vector<DRW_Attrib> attributes;
    std::vector<DRW_Attdef> definitions;
    void addHeader(const DRW_Header *) override {}
    void addLType(const DRW_LType &) override {}
    void addLayer(const DRW_Layer &) override {}
    void addDimStyle(const DRW_Dimstyle &) override {}
    void addVport(const DRW_Vport &) override {}
    void addTextStyle(const DRW_Textstyle &) override {}
    void addAppId(const DRW_AppId &) override {}
    void addBlock(const DRW_Block &) override {}
    void setBlock(const int) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point &) override {}
    void addLine(const DRW_Line &) override {}
    void addRay(const DRW_Ray &) override {}
    void addXline(const DRW_Xline &) override {}
    void addCircle(const DRW_Circle &) override {}
    void addArc(const DRW_Arc &) override {}
    void addEllipse(const DRW_Ellipse &) override {}
    void addLWPolyline(const DRW_LWPolyline &) override {}
    void addPolyline(const DRW_Polyline &) override {}
    void addSpline(const DRW_Spline *) override {}
    void addKnot(const DRW_Entity &) override {}
    void addInsert(const DRW_Insert &insert) override {
        for (const auto &value : insert.attlist) if (value) attributes.push_back(*value);
    }
    void addAttDef(const DRW_Attdef &value) override { definitions.push_back(value); }
    void addTrace(const DRW_Trace &) override {}
    void add3dFace(const DRW_3Dface &) override {}
    void addSolid(const DRW_Solid &) override {}
    void addMText(const DRW_MText &) override {}
    void addText(const DRW_Text &) override {}
    void addDimAlign(const DRW_DimAligned *) override {}
    void addDimLinear(const DRW_DimLinear *) override {}
    void addDimRadial(const DRW_DimRadial *) override {}
    void addDimDiametric(const DRW_DimDiametric *) override {}
    void addDimAngular(const DRW_DimAngular *) override {}
    void addDimAngular3P(const DRW_DimAngular3p *) override {}
    void addDimArc(const DRW_DimArc *) override {}
    void addDimOrdinate(const DRW_DimOrdinate *) override {}
    void addLeader(const DRW_Leader *) override {}
    void addHatch(const DRW_Hatch *) override {}
    void addViewport(const DRW_Viewport &) override {}
    void addImage(const DRW_Image *) override {}
    void linkImage(const DRW_ImageDef *) override {}
    void addComment(const char *) override {}
    void addPlotSettings(const DRW_PlotSettings *) override {}
    void writeHeader(DRW_Header &) override {}
    void writeBlocks() override {
        DRW_Block block; block.name = "ScopeBlock"; REQUIRE(writer->writeBlock(&block));
        if (mode == 3) REQUIRE(writer->writeAttdef(&definition));
    }
    void writeBlockRecords() override { REQUIRE(writer->writeBlockRecord("ScopeBlock")); }
    void writeEntities() override {
        if (mode == 0 || mode == 3) {
            DRW_Insert insert; insert.name = "ScopeBlock";
            insert.attlist.push_back(std::make_shared<DRW_Attrib>(attribute));
            lastEntityWriteSucceeded = writer->writeInsert(&insert);
        } else if (mode == 1) lastEntityWriteSucceeded = writer->writeAttdef(&definition);
        else lastEntityWriteSucceeded = writer->writeAttrib(&definition);
    }
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {
        for (const auto *name : {"OUTER_STYLE", "EMBED_STYLE"}) {
            DRW_Textstyle style; style.name = name; style.font = "txt";
            REQUIRE(writer->writeTextstyle(&style));
        }
    }
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {}
    void writeAppId() override {
        for (const auto *name : {"OuterScope", "EmbeddedScope", "LibreCADText", "LibreCad"}) {
            DRW_AppId app; app.name = name; REQUIRE(writer->writeAppId(&app));
        }
    }
};

void configure(DRW_Attrib &value) {
    value.tag = "SOURCE"; value.text = "OUTER-FALLBACK";
    value.height = 2; value.style = "OUTER_STYLE"; value.m_attributeType = 2;
    value.extData = {std::make_shared<DRW_Variant>(1001, std::string("OuterScope")),
        std::make_shared<DRW_Variant>(1000, std::string("outer-owned")),
        std::make_shared<DRW_Variant>(1071, 17),
        std::make_shared<DRW_Variant>(1001, std::string("LibreCADText")),
        std::make_shared<DRW_Variant>(1000, std::string("opaque outer profile")),
        std::make_shared<DRW_Variant>(1070, 999)};
    value.mtext = std::make_unique<DRW_MText>();
    value.mtext->height = 2; value.mtext->widthscale = 20;
    value.mtext->style = "EMBED_STYLE";
    value.mtext->text = "embedded \\H1.5x;body\\Psecond";
    value.mtext->basePoint = DRW_Coord(5, 7, 0);
    value.mtext->textgen = 1; value.mtext->alignH = DRW_Text::HAligned;
    value.mtext->linespacingStyle = 2; value.mtext->interlin = 1.5;
}
void exact(const Tags &actual, const Tags &expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        INFO("raw tag " << index);
        REQUIRE(actual[index]); REQUIRE(expected[index]);
        CHECK(actual[index]->code() == expected[index]->code());
        REQUIRE(actual[index]->type() == expected[index]->type());
        if (expected[index]->type() == DRW_Variant::STRING)
            CHECK(*actual[index]->content.s == *expected[index]->content.s);
        else CHECK(actual[index]->i_val() == expected[index]->i_val());
    }
}

QString externalFixture() {
#ifdef LIBRECAD_TEST_DIR
    return QStringLiteral(LIBRECAD_TEST_DIR "/dxf/embedded_mtext_external_r2018.dxf");
#else
    return QFileInfo(QString::fromUtf8(__FILE__)).absolutePath()
        + QStringLiteral("/testdata/dxf/embedded_mtext_external_r2018.dxf");
#endif
}
QByteArray fixtureBytes() {
    QFile file(externalFixture()); REQUIRE(file.open(QIODevice::ReadOnly));
    // A Windows checkout holds the fixture with CRLF line ends. The records
    // that the tests replace are spelled with LF, as is the copy they write.
    QByteArray bytes = file.readAll();
    bytes.replace("\r\n", "\n");
    return bytes;
}
void writeBytes(const QString &path, const QByteArray &bytes) {
    QFile file(path); REQUIRE(file.open(QIODevice::WriteOnly));
    REQUIRE(file.write(bytes) == bytes.size());
}
void nativeAndOuter(const DRW_Attrib &actual) {
    DRW_Attrib expected; configure(expected);
    CHECK(actual.text == expected.text); CHECK(actual.style == expected.style);
    REQUIRE(actual.mtext); CHECK(actual.mtext->text == expected.mtext->text);
    CHECK(actual.mtext->style == expected.mtext->style);
    CHECK(actual.mtext->basePoint.x == 5); CHECK(actual.mtext->basePoint.y == 7);
    CHECK(actual.mtext->height == 2); CHECK(actual.mtext->widthscale == 20);
    CHECK(actual.mtext->textgen == 1); CHECK(actual.mtext->alignH == 3);
    CHECK(actual.mtext->linespacingStyle == 2); CHECK(actual.mtext->interlin == 1.5);
    CHECK(actual.mtext->extData.empty());
    CHECK(actual.embeddedDxfMarker() == DRW_Attrib::EmbeddedDxfMarker::Canonical101);
    CHECK_FALSE(actual.hasAmbiguousLegacyEmbeddedXData());
    exact(actual.extData, expected.extData);
}
}

TEST_CASE("External canonical101 ATTRIB and ATTDEF preserve native body and outer authority", "[dxf-embedded-scope][external][headless]") {
    ScopeInterface capture; const auto path = externalFixture().toStdString();
    dxfRW reader(path.c_str()); REQUIRE(reader.read(&capture, false));
    REQUIRE(capture.attributes.size() == 1); REQUIRE(capture.definitions.size() == 1);
    nativeAndOuter(capture.attributes.front()); nativeAndOuter(capture.definitions.front());
    CHECK(capture.attributes.front().tag == "TAG");
    CHECK(capture.definitions.front().tag == "DEF_TAG");
}

TEST_CASE("Native DXF writer emits canonical101 body followed by outer XDATA", "[dxf-embedded-scope][headless]") {
    QTemporaryDir temporary; REQUIRE(temporary.isValid());
    const auto mode = GENERATE(0, 1, 2);
        INFO("writer dispatch " << mode);
        const auto path = temporary.filePath(QStringLiteral("scope-%1.dxf").arg(mode)).toStdString();
        ScopeInterface source; source.mode = mode;
        configure(source.attribute); configure(source.definition); source.definition.m_attributeType = 4;
        source.definition.prompt = "independent prompt";
        dxfRW writer(path.c_str()); source.writer = &writer;
        REQUIRE(writer.write(&source, DRW::AC1032, false));
        CHECK(source.lastEntityWriteSucceeded);
        ScopeInterface capture; dxfRW reader(path.c_str());
        REQUIRE(reader.read(&capture, false));
        const DRW_Attrib *actual = nullptr;
        const DRW_Attrib *expected = mode == 0 ? &source.attribute : &source.definition;
        if (mode == 0) { REQUIRE(capture.attributes.size() == 1); actual = &capture.attributes.front(); }
        else { REQUIRE(capture.definitions.size() == 1); actual = &capture.definitions.front(); }
        nativeAndOuter(*actual); CHECK(actual->tag == expected->tag);
        if (mode != 0) CHECK(capture.definitions.front().prompt == source.definition.prompt);

        std::ifstream file(path); REQUIRE(file.good());
        std::vector<std::pair<int, std::string>> record;
        std::string code, value; bool selected = false;
        while (std::getline(file, code) && std::getline(file, value)) {
            if (!code.empty() && code.back() == '\r') code.pop_back();
            if (!value.empty() && value.back() == '\r') value.pop_back();
            const int number = std::stoi(code);
            if (number == 0) {
                if (selected) break;
                selected = value == (mode == 0 ? "ATTRIB" : "ATTDEF");
            }
            if (selected) record.emplace_back(number, value);
        }
        const auto position = [&record](int codeValue, const char *text) {
            return std::find(record.begin(), record.end(), std::make_pair(codeValue, std::string(text)));
        };
        const auto outer = position(1001, "OuterScope"), marker = position(101, "Embedded Object");
        const auto body = position(1, expected->mtext->text.c_str());
        REQUIRE(outer != record.end()); REQUIRE(marker != record.end()); REQUIRE(body != record.end());
        CHECK(marker < body); CHECK(body < outer);
        CHECK(position(100, "Embedded Object") == record.end());
}

TEST_CASE("Embedded native group3 is continuation not ATTDEF prompt", "[dxf-embedded-scope][headless]") {
    QTemporaryDir temporary; REQUIRE(temporary.isValid());
    auto bytes = fixtureBytes();
    const bool legacy = GENERATE(false, true);
    if (legacy)
        bytes.replace("101\nEmbedded Object", "100\nEmbedded Object");
    const QByteArray continuation(250, 'x');
    const QByteArray original("  1\nembedded \\H1.5x;body\\Psecond\n");
    REQUIRE(bytes.count(original) == 2);
    const QByteArray replacement = QByteArray("  3\n") + continuation + "\n  1\nfinal body\n";
    bytes.replace(original, replacement);
    const auto path = temporary.filePath("continuation.dxf"); writeBytes(path, bytes);
    ScopeInterface capture; const auto name = path.toStdString(); dxfRW reader(name.c_str());
    REQUIRE(reader.read(&capture, false)); REQUIRE(capture.attributes.size() == 1); REQUIRE(capture.definitions.size() == 1);
    for (const DRW_Attrib *actual : {static_cast<const DRW_Attrib *>(&capture.attributes.front()),
                                    static_cast<const DRW_Attrib *>(&capture.definitions.front())}) {
        REQUIRE(actual->mtext); CHECK(actual->mtext->text == continuation.toStdString() + "final body");
        CHECK(actual->text == "OUTER-FALLBACK"); CHECK(actual->mtext->extData.empty());
        CHECK(actual->extData.size() == 6);
    }
    CHECK(capture.definitions.front().prompt != continuation.toStdString());
}

TEST_CASE("Legacy100 marker is read like the canonical one and its provenance survives value operations", "[dxf-embedded-scope][headless]") {
    QTemporaryDir temporary; REQUIRE(temporary.isValid());
    auto bytes = fixtureBytes();
    const QByteArray marker("101\nEmbedded Object"); REQUIRE(bytes.count(marker) == 2);
    bytes.replace(marker, "1001\nOuterScope\n1000\npre-marker-owned\n100\nEmbedded Object");
    const auto path = temporary.filePath("legacy.dxf"); writeBytes(path, bytes);
    ScopeInterface capture; const auto name = path.toStdString(); dxfRW reader(name.c_str());
    REQUIRE(reader.read(&capture, false)); REQUIRE(capture.attributes.size() == 1); REQUIRE(capture.definitions.size() == 1);
    DRW_Attrib expected; configure(expected);
    // XDATA is the ATTRIB's with either spelling of the marker, wherever it
    // stands in the record: the embedded MTEXT has none of its own.
    Tags outer{std::make_shared<DRW_Variant>(1001, std::string("OuterScope")),
        std::make_shared<DRW_Variant>(1000, std::string("pre-marker-owned"))};
    outer.insert(outer.end(), expected.extData.begin(), expected.extData.end());
    for (const DRW_Attrib *actual : {static_cast<const DRW_Attrib *>(&capture.attributes.front()),
                                    static_cast<const DRW_Attrib *>(&capture.definitions.front())}) {
        REQUIRE(actual->mtext); CHECK(actual->text == "OUTER-FALLBACK");
        CHECK(actual->mtext->text == expected.mtext->text);
        CHECK(actual->embeddedDxfMarker() == DRW_Attrib::EmbeddedDxfMarker::Legacy100);
        CHECK(actual->mtext->extData.empty());
        CHECK_FALSE(actual->hasAmbiguousLegacyEmbeddedXData());
        exact(actual->extData, outer);
        DRW_Attrib copied(*actual); DRW_Attrib assigned; assigned = *actual;
        CHECK(copied.embeddedDxfMarker() == actual->embeddedDxfMarker());
        CHECK(assigned.embeddedDxfMarker() == actual->embeddedDxfMarker());
        exact(copied.extData, outer); exact(assigned.extData, outer);
        struct Resettable : DRW_Attrib { using DRW_Attrib::resetAttribDwgState; } resettable;
        static_cast<DRW_Attrib &>(resettable) = *actual;
        resettable.resetAttribDwgState();
        CHECK(resettable.embeddedDxfMarker() == DRW_Attrib::EmbeddedDxfMarker::None);
        CHECK_FALSE(resettable.hasAmbiguousLegacyEmbeddedXData()); CHECK_FALSE(resettable.mtext);
        // Only a value that a caller builds can hold XDATA on the embedded MTEXT.
        copied.mtext->extData = expected.extData;
        CHECK(copied.hasAmbiguousLegacyEmbeddedXData());
        DRW_Attrib moved(std::move(copied));
        CHECK(moved.embeddedDxfMarker() == DRW_Attrib::EmbeddedDxfMarker::Legacy100);
        CHECK(moved.hasAmbiguousLegacyEmbeddedXData());
    }
}

TEST_CASE("Native DXF writes the host's XDATA and nothing of an embedded MTEXT's own", "[dxf-embedded-scope][headless]") {
    QTemporaryDir temporary; REQUIRE(temporary.isValid());
    const auto mode = GENERATE(0, 1, 2);
    INFO("writer dispatch " << mode);
    ScopeInterface source; source.mode = mode; configure(source.attribute); configure(source.definition);
    source.definition.m_attributeType = 4;
    for (auto *value : {&source.attribute, static_cast<DRW_Attrib *>(&source.definition)}) {
        value->mtext->extData = {std::make_shared<DRW_Variant>(1001, std::string("EmbeddedScope")),
                                std::make_shared<DRW_Variant>(1000, std::string("embedded-only payload"))};
    }
    const auto path = temporary.filePath("embedded-xdata.dxf");
    const auto name = path.toStdString();
    dxfRW writer(name.c_str()); source.writer = &writer;
    // An embedded object has no XDATA frame in DXF. The write is not refused
    // for it: the record carries the XDATA of the ATTRIB or ATTDEF only.
    REQUIRE(writer.write(&source, DRW::AC1032, false));
    CHECK(source.lastEntityWriteSucceeded);
    CHECK(source.attribute.mtext->extData.size() == 2);
    CHECK(*source.attribute.mtext->extData.back()->content.s == "embedded-only payload");
    QFile written(path); REQUIRE(written.open(QIODevice::ReadOnly));
    CHECK_FALSE(written.readAll().contains("embedded-only payload"));
    ScopeInterface capture; dxfRW reader(name.c_str()); REQUIRE(reader.read(&capture, false));
    if (mode == 0) { REQUIRE(capture.attributes.size() == 1); nativeAndOuter(capture.attributes.front()); }
    else { REQUIRE(capture.definitions.size() == 1); nativeAndOuter(capture.definitions.front()); }
}

TEST_CASE("Native DXF paired writer fixture is independently readable", "[dxf-embedded-scope][headless]") {
    QTemporaryDir temporary; REQUIRE(temporary.isValid());
    const auto output = qEnvironmentVariable("LC_EMBEDDED_DXF_ORACLE_OUTPUT");
    const auto path = (output.isEmpty() ? temporary.filePath("paired.dxf") : output).toStdString();
    ScopeInterface source; source.mode = 3; configure(source.attribute); configure(source.definition);
    source.definition.m_attributeType = 4; source.definition.prompt = "definition prompt";
    dxfRW writer(path.c_str()); source.writer = &writer;
    REQUIRE(writer.write(&source, DRW::AC1032, false)); CHECK(source.lastEntityWriteSucceeded);
    ScopeInterface capture; dxfRW reader(path.c_str()); REQUIRE(reader.read(&capture, false));
    REQUIRE(capture.attributes.size() == 1); REQUIRE(capture.definitions.size() == 1);
    nativeAndOuter(capture.attributes.front()); nativeAndOuter(capture.definitions.front());
}
