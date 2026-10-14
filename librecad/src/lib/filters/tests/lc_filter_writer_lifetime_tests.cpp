/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_text.h"
#include "rs_settings.h"
#include "rs_debug.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <stdexcept>

namespace {
void settings() {
    static int argc = 1;
    static char name[] = "filter_writer_lifetime_tests";
    static char *argv[] = {name, nullptr};
    static QCoreApplication *app = QCoreApplication::instance() ? QCoreApplication::instance() : new QCoreApplication(argc, argv);
    static const bool initialized = [] { RS_Settings::init("LibreCAD", "LibreCAD-tests"); return true; }();
    (void)app; (void)initialized;
    RS_DEBUG->setLevel(RS_Debug::D_ERROR);
}

RS_Text *initialize(RS_FilterDXFRW &input, RS_Graphic &graphic, const QString &path) {
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly));
    const QByteArray empty = "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n0\nENDSEC\n0\nSECTION\n2\nTABLES\n0\nENDSEC\n0\nSECTION\n2\nBLOCKS\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n";
    REQUIRE(file.write(empty) == empty.size());
    file.close();
    REQUIRE(input.fileImport(graphic, path, RS2::FormatDXFRW2013));
    DRW_Text source; source.text = "writer lifetime"; source.height = 2; source.style = "standard";
    input.addText(source);
    RS_Text *text = nullptr;
    for (auto *entity : graphic) if (entity->rtti() == RS2::EntityText) {
        REQUIRE(text == nullptr);
        text = static_cast<RS_Text *>(entity);
    }
    REQUIRE(text);
    return text;
}

bool isDwg(RS2::FormatType format) {
#ifdef DWGSUPPORT
    return format == RS2::FormatDWG2004 || format == RS2::FormatDWG2013 ||
           format == RS2::FormatDWG2018;
#else
    (void)format;
    return false;
#endif
}

DRW::Version expectedVersion(RS2::FormatType format) {
    switch (format) {
    case RS2::FormatDXFRW2004: case RS2::FormatDWG2004: return DRW::AC1018;
    case RS2::FormatDXFRW2018: case RS2::FormatDWG2018: return DRW::AC1032;
    default: return DRW::AC1027;
    }
}

QByteArray versionSignature(RS2::FormatType format) {
    switch (expectedVersion(format)) {
    case DRW::AC1018: return QByteArrayLiteral("AC1018");
    case DRW::AC1032: return QByteArrayLiteral("AC1032");
    default: return QByteArrayLiteral("AC1027");
    }
}

QString outputPath(const QTemporaryDir &directory, int index, RS2::FormatType format) {
    return directory.filePath(QStringLiteral("output-%1.%2").arg(index).arg(isDwg(format) ? QStringLiteral("dwg") : QStringLiteral("dxf")));
}

void verifySaved(const QString &path, RS2::FormatType format) {
    QFile file(path); REQUIRE(file.open(QIODevice::ReadOnly));
    const auto bytes = file.readAll();
    if (isDwg(format)) CHECK(bytes.startsWith(versionSignature(format)));
    else CHECK(bytes.contains(versionSignature(format)));
    RS_Graphic reloaded; RS_FilterDXFRW reader;
    REQUIRE(reader.fileImport(reloaded, path, format));
    int count = 0;
    for (const auto *entity : reloaded) if (entity->rtti() == RS2::EntityText) {
        CHECK(static_cast<const RS_Text *>(entity)->getCanonicalText() == QStringLiteral("writer lifetime"));
        ++count;
    }
    CHECK(count == 1);
}

class ObservingFilter : public RS_FilterDXFRW {
public:
    DRW::Version observedVersion = DRW::UNKNOWNV;
    void writeHeader(DRW_Header &header) override {
        observedVersion = dimStyleTargetVersion();
        RS_FilterDXFRW::writeHeader(header);
    }
};

class ReentrantFilter final : public ObservingFilter {
public:
    RS_Graphic *graphic = nullptr;
    QString nestedPath;
    RS2::FormatType nestedFormat = RS2::FormatDXFRW2013;
    bool attempted = false;
    bool nestedSucceeded = false;
    bool activeWriterPreserved = false;
    void writeHeader(DRW_Header &header) override {
        if (!attempted) {
            attempted = true;
            const auto activeVersion = dimStyleTargetVersion();
            nestedSucceeded = fileExport(*graphic, nestedPath, nestedFormat);
            activeWriterPreserved = dimStyleTargetVersion() == activeVersion;
        }
        ObservingFilter::writeHeader(header);
    }
};

class ThrowingFilter final : public ObservingFilter {
public:
    bool throwNext = true;
    void writeHeader(DRW_Header &header) override {
        ObservingFilter::writeHeader(header);
        if (throwNext) {
            throwNext = false;
            throw std::runtime_error("injected export callback failure");
        }
    }
};
}

TEST_CASE("Filter writer observer is inactive after every successful same-instance export", "[filter-writer-lifetime][observer]") {
    settings();
    QTemporaryDir directory; REQUIRE(directory.isValid());
    RS_Graphic graphic; RS_FilterDXFRW input;
    const auto *text = initialize(input, graphic, directory.filePath(QStringLiteral("empty.dxf")));
    const auto source = text->getCanonicalText();
    ObservingFilter output;
    REQUIRE(output.dimStyleTargetVersion() == DRW::UNKNOWNV);
#ifdef DWGSUPPORT
    const auto sequence = GENERATE(std::vector<RS2::FormatType>{RS2::FormatDXFRW2013, RS2::FormatDWG2013},
                                  std::vector<RS2::FormatType>{RS2::FormatDWG2013, RS2::FormatDXFRW2013},
                                  std::vector<RS2::FormatType>{RS2::FormatDWG2013, RS2::FormatDWG2013},
                                  std::vector<RS2::FormatType>{RS2::FormatDXFRW2013, RS2::FormatDXFRW2013},
                                  std::vector<RS2::FormatType>{RS2::FormatDXFRW2004, RS2::FormatDWG2018},
                                  std::vector<RS2::FormatType>{RS2::FormatDWG2004, RS2::FormatDXFRW2018});
#else
    const std::vector<RS2::FormatType> sequence{RS2::FormatDXFRW2013, RS2::FormatDXFRW2013};
#endif
    int index = 0;
    for (const auto format : sequence) {
        INFO("export format " << int(format));
        const auto path = outputPath(directory, index++, format);
        REQUIRE(output.fileExport(graphic, path, format));
        CHECK(output.observedVersion == expectedVersion(format));
        // This public query must not dereference a freed prior writer, nor
        // report a prior format outside an active export callback.
        REQUIRE(output.dimStyleTargetVersion() == DRW::UNKNOWNV);
        verifySaved(path, format);
        CHECK(text->getCanonicalText() == source);
    }
}

TEST_CASE("Filter failed export releases its writer and permits same-instance retry", "[filter-writer-lifetime][retry]") {
    settings();
    QTemporaryDir directory; REQUIRE(directory.isValid());
#ifdef DWGSUPPORT
    const auto format = GENERATE(RS2::FormatDXFRW2013, RS2::FormatDWG2013);
#else
    const auto format = RS2::FormatDXFRW2013;
#endif
    RS_Graphic graphic; RS_FilterDXFRW input;
    auto *text = initialize(input, graphic, directory.filePath(QStringLiteral("empty.dxf")));
    RS_FilterDXFRW output;
    // Failure before a writer is allocated.
    CHECK_FALSE(output.fileExport(graphic, directory.filePath(QStringLiteral("missing/output.dxf")), format));
    REQUIRE(output.dimStyleTargetVersion() == DRW::UNKNOWNV);
    // Failure during native entity/XDATA emission, after header/table writes.
    const auto invalid = std::make_shared<DRW_Variant>(1001, std::string("bad\0appid", 9));
    text->setDrwExtData({invalid});
    CHECK_FALSE(output.fileExport(graphic, outputPath(directory, 0, format), format));
    REQUIRE(output.dimStyleTargetVersion() == DRW::UNKNOWNV);
    REQUIRE(text->getDrwExtData().size() == 1);
    CHECK(text->getDrwExtData().front() == invalid);
    text->setDrwExtData({});
    for (int index = 1; index <= 2; ++index) {
        const auto path = outputPath(directory, index, format);
        REQUIRE(output.fileExport(graphic, path, format));
        REQUIRE(output.dimStyleTargetVersion() == DRW::UNKNOWNV);
        verifySaved(path, format);
    }
}

TEST_CASE("Reentrant filter export cannot replace the active writer observer", "[filter-writer-lifetime][reentrant]") {
    settings();
    QTemporaryDir directory; REQUIRE(directory.isValid());
#ifdef DWGSUPPORT
    const auto format = GENERATE(RS2::FormatDXFRW2004, RS2::FormatDWG2018);
#else
    const auto format = RS2::FormatDXFRW2013;
#endif
    RS_Graphic graphic; RS_FilterDXFRW input;
    initialize(input, graphic, directory.filePath(QStringLiteral("empty.dxf")));
    ReentrantFilter output;
    output.graphic = &graphic;
    output.nestedPath = outputPath(directory, 1, format);
    output.nestedFormat = isDwg(format) ? RS2::FormatDXFRW2004 : RS2::FormatDWG2018;
    const auto path = outputPath(directory, 0, format);
    REQUIRE(output.fileExport(graphic, path, format));
    CHECK(output.attempted);
    CHECK_FALSE(output.nestedSucceeded);
    CHECK(output.activeWriterPreserved);
    CHECK(output.observedVersion == expectedVersion(format));
    CHECK_FALSE(QFileInfo::exists(output.nestedPath));
    REQUIRE(output.dimStyleTargetVersion() == DRW::UNKNOWNV);
    verifySaved(path, format);
}

TEST_CASE("Callback exceptions release the writer and allow same-instance cross-format retry", "[filter-writer-lifetime][exception]") {
    settings();
    QTemporaryDir directory; REQUIRE(directory.isValid());
#ifdef DWGSUPPORT
    const auto format = GENERATE(RS2::FormatDXFRW2004, RS2::FormatDWG2018);
#else
    const auto format = RS2::FormatDXFRW2004;
#endif
    RS_Graphic graphic; RS_FilterDXFRW input;
    initialize(input, graphic, directory.filePath(QStringLiteral("empty.dxf")));
    ThrowingFilter output;
    CHECK_FALSE(output.fileExport(graphic, outputPath(directory, 0, format), format));
    CHECK_FALSE(output.throwNext);
    CHECK(output.observedVersion == expectedVersion(format));
    REQUIRE(output.dimStyleTargetVersion() == DRW::UNKNOWNV);
    const auto other = isDwg(format) ? RS2::FormatDXFRW2004 :
#ifdef DWGSUPPORT
        RS2::FormatDWG2018;
#else
        RS2::FormatDXFRW2018;
#endif
    int index = 1;
    for (const auto retryFormat : {other, format}) {
        const auto path = outputPath(directory, index++, retryFormat);
        REQUIRE(output.fileExport(graphic, path, retryFormat));
        CHECK(output.observedVersion == expectedVersion(retryFormat));
        REQUIRE(output.dimStyleTargetVersion() == DRW::UNKNOWNV);
        verifySaved(path, retryFormat);
    }
}
