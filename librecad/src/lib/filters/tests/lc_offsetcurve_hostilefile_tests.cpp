/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/catch_approx.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "lc_offsetcurve_persistence_fixture.h"
#include "lc_offsetimportsession.h"
#include "intern/dwg_dxf_output_transaction.h"
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QProcess>
#include <QThread>
#ifdef Q_OS_UNIX
#include <cerrno>
#include <csignal>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "rs_layer.h"
#include "rs_block.h"
#include "rs_undo.h"
#include "rs_variable.h"
#include "rs_filterdxfrw.h"
#include "rs_pattern.h"
#ifdef DWGSUPPORT

class LC_DwgReadVersionTestAccess {
public:
    static void beforeOpen(RS_FilterDXFRW& filter, std::function<void()> hook) {
        filter.m_testBeforeDwgOpen = std::move(hook);
    }
    static DRW::Version inputVersion(const RS_FilterDXFRW& filter) { return filter.m_offsetInputVersion; }
    static int numericVersion(const RS_FilterDXFRW& filter) { return filter.m_version; }
    static void prepareHeader(RS_FilterDXFRW& filter, RS_Graphic& graphic, bool dwg) {
        filter.m_graphic = &graphic; filter.m_currentContainer = &graphic;
        filter.m_offsetInputKind = dwg ? RS_FilterDXFRW::OffsetInputKind::Dwg : RS_FilterDXFRW::OffsetInputKind::Dxf;
        filter.m_offsetInputVersion = DRW::AC1015; filter.m_version = 1015;
    }
};
#endif

class LC_OffsetWorkerClientTestAccess {
public:
    static void onStarted(LC_OffsetWorkerClient& worker, std::function<void(QProcess&)> hook) {
        worker.m_testStarted = std::move(hook);
    }
    static bool hasStartHook(const LC_OffsetWorkerClient& worker) { return bool(worker.m_testStarted); }
};
class LC_OffsetImportSessionTestAccess {
public:
    static LC_OffsetWorkerClient& worker(LC_OffsetImportSession& session) { return session.m_worker; }
    static QTimer& deadline(LC_OffsetImportSession& session) { return session.m_deadline; }
    static void afterSnapshotChunk(LC_OffsetImportSession& session, std::function<void()> hook) {
        session.m_testAfterSnapshotChunk = std::move(hook);
    }
    static void beforeSourceOpen(LC_OffsetImportSession& session, std::function<void()> hook) {
        session.m_testBeforeSourceOpen = std::move(hook);
    }
    static void afterRead(LC_OffsetImportSession& session, std::function<void()> hook) {
        session.m_testAfterRead = std::move(hook);
    }
    static void afterXrefSnapshot(LC_OffsetImportSession& session, std::function<void()> hook) {
        session.m_testAfterXrefSnapshot = std::move(hook);
    }
    static void afterSnapshotParse(LC_OffsetImportSession& session,
                                   std::function<void(RS_Graphic&)> hook) {
        session.m_testAfterSnapshotParse = std::move(hook);
    }
};

namespace {
namespace f = lc::test::offset_persistence;

struct Setup {
    const bool ready{lc::test::ensureSettings()};
    lc::test::QuietLog quiet;
};

#if defined(DWGSUPPORT) && defined(Q_OS_UNIX)
class ObservedDwgVersionFilter final : public RS_FilterDXFRW {
public:
    DRW::Version observed = DRW::UNKNOWNV;
    int numeric = 0;
    unsigned splines = 0;
    void addSpline(const DRW_Spline* spline) override {
        observed = LC_DwgReadVersionTestAccess::inputVersion(*this);
        numeric = LC_DwgReadVersionTestAccess::numericVersion(*this);
        ++splines;
        RS_FilterDXFRW::addSpline(spline);
    }
};
#endif

enum class Lane { Ascii, Binary14, Binary2000, Binary18, Eed2000, Eed18 };
std::vector<Lane> lanes() {
    std::vector<Lane> result{Lane::Ascii, Lane::Binary14, Lane::Binary18};
#ifdef DWGSUPPORT
    result.insert(result.end(), {Lane::Eed2000, Lane::Eed18});
#endif
    return result;
}
bool isDwg(Lane lane) { return lane == Lane::Eed2000 || lane == Lane::Eed18; }
RS2::FormatType format(Lane lane) {
#ifdef DWGSUPPORT
    if (lane == Lane::Eed2000) return RS2::FormatDWG;
    if (lane == Lane::Eed18) return RS2::FormatDWG2018;
#endif
    if (lane == Lane::Binary2000) return RS2::FormatDXFRW2000;
    return lane == Lane::Binary14 ? RS2::FormatDXFRW14 : RS2::FormatDXFRW2018;
}
void writeInput(const std::filesystem::path& path, Lane lane, const DRW_Spline& raw) {
    switch (lane) {
    case Lane::Ascii: f::put(path, f::ascii(raw)); return;
    case Lane::Binary14: f::binaryDxf(path, raw, DRW::AC1014); return;
    case Lane::Binary2000: f::binaryDxf(path, raw, DRW::AC1015); return;
    case Lane::Binary18: f::binaryDxf(path, raw, DRW::AC1032); return;
#ifdef DWGSUPPORT
    case Lane::Eed2000: f::dwgFile(path, raw, DRW::AC1015); return;
    case Lane::Eed18: f::dwgFile(path, raw, DRW::AC1032); return;
#else
    default: FAIL("DWG fixture requested in a build without DWGSUPPORT");
#endif
    }
}

void writeXrefHost(const std::filesystem::path& path,
                   const std::vector<std::pair<std::string, std::string>>& refs) {
    auto text = f::ascii(f::raw(3));
    const auto entities = text.find("0\nSECTION\n2\nENTITIES\n");
    REQUIRE(entities != std::string::npos);
    text.resize(entities);
    text += "0\nSECTION\n2\nBLOCKS\n";
    for (std::size_t i = 0; i < refs.size(); ++i) {
        const auto blockHandle = "B" + std::to_string(i * 2 + 1);
        const auto endHandle = "B" + std::to_string(i * 2 + 2);
        text += "0\nBLOCK\n5\n" + blockHandle + "\n8\n0\n2\n" + refs[i].first +
            "\n70\n4\n1\n" + refs[i].second +
            "\n10\n0\n20\n0\n30\n0\n0\nENDBLK\n5\n" + endHandle + "\n8\n0\n";
    }
    text += "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n";
    for (std::size_t i = 0; i < refs.size(); ++i) {
        text += "0\nINSERT\n5\nC" + std::to_string(i + 1) + "\n8\n0\n2\n" + refs[i].first +
            "\n10\n0\n20\n0\n30\n0\n";
    }
    text += "0\nENDSEC\n0\nEOF\n";
    f::put(path, text);
}

LC_OffsetImportSession::Outcome stagedImport(const std::filesystem::path& path, Lane lane, bool acceptPartial = false) {
    LC_OffsetImportSession session;
    std::optional<LC_OffsetImportSession::Outcome> outcome;
    int ticks = 0; QTimer heartbeat; QObject::connect(&heartbeat, &QTimer::timeout, [&] { ++ticks; }); heartbeat.start(0);
    REQUIRE(session.start(QString::fromStdString(path.string()), format(lane), RS2::DxfTextEncoding::Declared,
        acceptPartial, [&](LC_OffsetImportSession::Outcome result) {
            CHECK(LC_OffsetImportSessionTestAccess::worker(session).healthyAndIdle()); outcome.emplace(std::move(result));
        }));
    REQUIRE(session.busy());
    QElapsedTimer elapsed; elapsed.start();
    while (!outcome) {
        REQUIRE(elapsed.elapsed() < 35000);
        QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1);
    }
    heartbeat.stop();
    if (outcome->admitted) CHECK(ticks > 0);
    CHECK_FALSE(session.busy()); return std::move(*outcome);
}
std::shared_ptr<DRW_Variant> text(int code, const std::string& value) {
    return std::make_shared<DRW_Variant>(code, value);
}
std::shared_ptr<DRW_Variant> integer(int code, std::int32_t value) {
    return std::make_shared<DRW_Variant>(code, value);
}
f::Tags withTail(const f::Tags& tail) {
    auto payload = f::owned(0x40, false);
    payload.insert(payload.end() - 1, tail.begin(), tail.end());
    return payload;
}
f::Tags repeatedItems(std::size_t count) {
    f::Tags result;
    for (std::size_t i = 0; i < count; ++i) result.push_back(integer(1071, static_cast<std::int32_t>(i)));
    return result;
}
f::Tags nestedItems(unsigned depth) {
    f::Tags result;
    for (unsigned i = 0; i < depth; ++i) result.push_back(text(1002, "{"));
    result.push_back(text(1000, "innermost retained item"));
    for (unsigned i = 0; i < depth; ++i) result.push_back(text(1002, "}"));
    return result;
}

void vectorState(std::ostream& out, const RS_Vector& p) {
    out << p.valid << ':' << f::bits(p.x) << ',' << f::bits(p.y) << ',' << f::bits(p.z) << ';';
}
void stringState(std::ostream& out, const QString& value) {
    out << value.size() << ':';
    for (const auto unit : value) out << unit.unicode() << ',';
    out << ';';
}
void cacheState(std::ostream& out, const RS_SplineData& data) {
    out << data.degree << ':' << static_cast<int>(data.type) << ':' << data.m_closedFlag << data.m_periodicFlag << ':';
    out << static_cast<int>(data.savedOpenType) << ';';
    for (const auto* points : {&data.controlPoints, &data.fitPoints}) {
        out << points->size() << ':';
        for (const auto& p : *points) vectorState(out, p);
    }
    for (const auto* numbers : {&data.knotslist, &data.weights, &data.savedOpenKnots}) {
        out << numbers->size() << ':';
        for (double value : *numbers) out << f::bits(value) << ';';
    }
}
std::string state(RS_Graphic& graphic) {
    std::ostringstream out;
    bool undo = false, redo = false;
    graphic.collectUndoState(undo, redo);
    out << graphic.count() << graphic.isModified() << undo << redo << ';';
    out << static_cast<const RS_Undo&>(graphic);
    out << static_cast<int>(graphic.getFormatType()) << ':'; stringState(out, graphic.getFilename());
    out << graphic.getVariableDictObjectRef()->isModified() << ';';
    const auto variables = graphic.getVariableDict();
    auto keys = variables.keys(); std::sort(keys.begin(), keys.end());
    for (const auto& key : keys) {
        stringState(out, key);
        const auto& value = variables[key];
        out << value.getCode() << ':' << static_cast<int>(value.getType()) << ':';
        stringState(out, value.getString()); out << value.getInt() << ':' << f::bits(value.getDouble()) << ';';
        vectorState(out, value.getVector());
    }
    REQUIRE(graphic.count() < 8);
    for (const auto* entity : graphic) {
        REQUIRE(entity);
        out << entity << ':' << entity->getParent() << ':' << entity->getLayer() << ':' << entity->getId() << ':';
        out << entity->rtti() << ':' << entity->getFlags() << ':' << entity->sourceHandle() << ':';
        out << entity->xDictHandle() << ':' << entity->lineage() << ':' << entity->ownerLineage() << ';';
        for (auto handle : entity->reactorHandles()) out << handle << ',';
        out << f::tagsState(entity->getDrwExtData()) << ';';
        for (const auto& tag : entity->getDrwExtData()) out << tag.get() << ',';
        vectorState(out, entity->getStartpoint()); vectorState(out, entity->getEndpoint());
        if (const auto* spline = dynamic_cast<const RS_Spline*>(entity)) {
            cacheState(out, spline->getData()); REQUIRE(spline->count() < 128);
            for (const auto* child : *spline) {
                REQUIRE(child); out << child << ':' << child->getParent() << ':' << child->getFlags() << ':';
                vectorState(out, child->getStartpoint()); vectorState(out, child->getEndpoint());
            }
        }
        if (const auto* curve = dynamic_cast<const LC_OffsetCurve*>(entity)) {
            const auto& d = curve->definition();
            vectorState(out, d.basis.center); vectorState(out, d.basis.majorP);
            for (double value : {d.basis.ratio, d.basis.angle1, d.basis.angle2, d.distance, d.relTolerance}) out << f::bits(value) << ';';
            out << d.basis.reversed << d.basis.isArc << d.towardCentre << d.cacheReversed << curve->isDefined() << ':';
            out << curve->unknownOwnedFlags() << ':' << f::bits(curve->checkedWorldTolerance()) << ';';
            for (const auto& item : curve->unknownOwnedTail()) {
                out << item.code << ':' << item.value.index() << ':';
                std::visit([&](const auto& value) {
                    using T = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<T, QString>) stringState(out, value);
                    else if constexpr (std::is_same_v<T, RS_Vector>) vectorState(out, value);
                    else if constexpr (std::is_same_v<T, double>) out << f::bits(value);
                    else out << value;
                }, item.value);
            }
            for (const auto& grip : curve->cachedDefinitionGrips()) vectorState(out, grip);
        }
    }
    return out.str();
}
struct Snapshot {
    RS_Graphic& graphic;
    std::string before;
    std::vector<RS_Pen> pens;
    explicit Snapshot(RS_Graphic& graphic) : graphic(graphic), before(state(graphic)) {
        for (const auto* entity : graphic) pens.push_back(entity->getPen(false));
    }
    void check() const {
        CHECK(state(graphic) == before);
        std::size_t i = 0;
        for (const auto* entity : graphic) { REQUIRE(i < pens.size()); CHECK(entity->getPen(false) == pens[i++]); }
        CHECK(i == pens.size());
    }
};

void checkTail(const LC_OffsetCurve& curve, const f::Tags& expected) {
    const auto& actual = curve.unknownOwnedTail(); REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        CAPTURE(i); REQUIRE(expected[i]); CHECK(actual[i].code == expected[i]->code());
        const auto& value = actual[i].value;
        switch (expected[i]->type()) {
        case DRW_Variant::STRING:
            REQUIRE(std::holds_alternative<QString>(value));
            CHECK(std::get<QString>(value).toUtf8().toStdString() == *expected[i]->content.s); break;
        case DRW_Variant::INTEGER:
            if (expected[i]->code() == 1070) {
                REQUIRE(std::holds_alternative<std::int16_t>(value)); CHECK(std::get<std::int16_t>(value) == expected[i]->i_val());
            } else {
                REQUIRE(std::holds_alternative<std::int32_t>(value)); CHECK(std::get<std::int32_t>(value) == expected[i]->i_val());
            }
            break;
        case DRW_Variant::DOUBLE:
            REQUIRE(std::holds_alternative<double>(value)); CHECK(f::bits(std::get<double>(value)) == f::bits(expected[i]->d_val())); break;
        case DRW_Variant::COORD: {
            REQUIRE(std::holds_alternative<RS_Vector>(value));
            const auto& p = std::get<RS_Vector>(value); const auto& q = *expected[i]->coord();
            CHECK(f::bits(p.x) == f::bits(q.x)); CHECK(f::bits(p.y) == f::bits(q.y)); CHECK(f::bits(p.z) == f::bits(q.z)); break;
        }
        default: FAIL("Expected safe tail must be a representable typed scalar");
        }
    }
}

// The raw codec read is mandatory: it proves the hostile records reached the
// adapter, rather than letting a writer's normalization satisfy the assertion.
void fileCase(Lane lane, const f::Tags& payload, bool editable,
              const f::Tags& remainder = {}, const f::Tags& tail = {},
              const f::Tags* decodedPayload = nullptr) {
    CAPTURE(static_cast<int>(lane));
    lc::test::OutputDirectory directory("offset-hostile-file-case");
    const auto input = directory.file(isDwg(lane) ? "input.dwg" : "input.dxf");
    writeInput(input, lane, f::raw(3, f::metadata(payload)));
    const auto decoded = f::readCodec(input, isDwg(lane)); REQUIRE(decoded.splines.size() == 1);
    REQUIRE(f::tagsState(decoded.splines.front().extData) == f::tagsState(f::metadata(decodedPayload ? *decodedPayload : payload)));
    RS_Graphic graphic; REQUIRE(lc::test::importDrawing(graphic, input, format(lane)));
    auto* entity = f::sole(graphic); REQUIRE(entity); f::foreign(*entity);
    CHECK(entity->isVisible()); CHECK(entity->getLength() > 0); CHECK(entity->getParent() == &graphic);
    CHECK(f::tagsState(entity->getDrwExtData()) == f::tagsState(f::metadata(remainder)));
    if (editable) {
        auto* curve = f::native(graphic, 3); CHECK(curve->unknownOwnedFlags() == 0x40); checkTail(*curve, tail);
    } else {
        CHECK(entity->rtti() == RS2::EntitySpline); CHECK(dynamic_cast<LC_OffsetCurve*>(entity) == nullptr);
    }
    Snapshot before(graphic);
    const auto output = directory.file(isDwg(lane) ? "saved.dwg" : "saved.dxf");
    REQUIRE(lc::test::exportDrawing(graphic, output, format(lane))); before.check();
    const auto saved = f::readCodec(output, isDwg(lane)); REQUIRE(saved.splines.size() == 1); CHECK(saved.ellipses == 0);
    auto expectedOwn = f::application(f::metadata(remainder), "LibreCad");
    if (editable) f::append(expectedOwn, withTail(tail));
    CHECK(f::tagsState(f::application(saved.splines.front().extData, "LibreCad")) == f::tagsState(expectedOwn));
    CHECK(f::tagsState(f::application(saved.splines.front().extData, "FOREIGN_A")) == f::tagsState(f::foreignBefore()));
    CHECK(f::tagsState(f::application(saved.splines.front().extData, "FOREIGN_B")) == f::tagsState(f::foreignAfter()));
    const auto controls = f::controls(3); REQUIRE(saved.splines.front().controllist.size() == controls.size());
    for (std::size_t i = 0; i < controls.size(); ++i) {
        const auto& actual = saved.splines.front().controllist[i]; REQUIRE(actual);
        CHECK(f::bits(actual->x) == f::bits(controls[i].x)); CHECK(f::bits(actual->y) == f::bits(controls[i].y)); CHECK(actual->z == 0);
    }
    RS_Graphic reopened; REQUIRE(lc::test::importDrawing(reopened, output, format(lane)));
    if (editable) checkTail(*f::native(reopened, 3), tail);
    else CHECK(f::sole(reopened)->rtti() == RS2::EntitySpline);
    CHECK(f::tagsState(f::sole(reopened)->getDrwExtData()) == f::tagsState(f::metadata(remainder)));
    before.check();
}

void allFiles(const f::Tags& payload, bool editable, const f::Tags& remainder = {}, const f::Tags& tail = {}) {
    for (auto lane : lanes()) fileCase(lane, payload, editable, remainder, tail);
}
void replaceUnique(std::string& bytes, const std::string& oldValue, const std::string& newValue) {
    const auto position = bytes.find(oldValue); REQUIRE(position != std::string::npos);
    REQUIRE(bytes.find(oldValue, position + oldValue.size()) == std::string::npos);
    bytes.replace(position, oldValue.size(), newValue);
}
void codecRefuses(const std::filesystem::path& path) {
    f::Capture capture;
    dxfRW reader(path.string().c_str()); CHECK_FALSE(reader.read(&capture, false)); CHECK(capture.splines.empty());
    RS_Graphic graphic; CHECK_FALSE(lc::test::importDrawing(graphic, path, RS2::FormatDXFRW2018));
    for (const auto* entity : graphic) if (entity) CHECK(entity->rtti() != RS2::EntityOffsetCurve);
}
void seedHistory(lc::test::EditableDrawing& drawing) {
    auto* marker = new RS_Line(&drawing.m_graphic, RS_Vector(-2, -3), RS_Vector(-1, -3));
    drawing.modify([&](LC_DocumentModificationBatch& batch) { batch.add(marker); });
    REQUIRE(drawing.m_graphic.undo()); REQUIRE(marker->isDeleted());
    bool undo = false, redo = false; drawing.m_graphic.collectUndoState(undo, redo); REQUIRE(redo);
    drawing.m_graphic.addVariable("$G_HOSTILE_SENTINEL", "retained variables", 1);
}
}

#if defined(DWGSUPPORT) && defined(Q_OS_UNIX)
TEST_CASE("DWG path import reads the already-open drawing after path replacement",
          "[lc-offsetcurve][import-session][dwg-version][source-identity]") {
    Setup setup; lc::test::OutputDirectory directory("offset-dwg-version-source");
    for (const auto original : {Lane::Eed2000, Lane::Eed18}) {
        const auto replacement = original == Lane::Eed2000 ? Lane::Eed18 : Lane::Eed2000;
        const std::string prefix = original == Lane::Eed2000 ? "from-2000-" : "from-2018-";
        const auto sourcePath = directory.file(prefix + "source.dwg");
        const auto replacementPath = directory.file(prefix + "replacement.dwg");
        const auto savedPath = directory.file(prefix + "saved.dwg");
        std::error_code error;
        writeInput(sourcePath, original, f::raw(3)); writeInput(replacementPath, replacement, f::raw(3));
        RS_Graphic graphic; graphic.initForNewDocument(); ObservedDwgVersionFilter filter; bool swapped = false;
        LC_DwgReadVersionTestAccess::beforeOpen(filter, [&] {
            std::filesystem::rename(sourcePath, savedPath, error); REQUIRE_FALSE(error);
            std::filesystem::rename(replacementPath, sourcePath, error); REQUIRE_FALSE(error);
            swapped = true;
        });
        CHECK_FALSE(filter.fileImport(graphic, QString::fromStdString(sourcePath.string()), RS2::FormatDWG));
        REQUIRE(swapped); REQUIRE(filter.splines > 0);
        CHECK(filter.observed == (original == Lane::Eed2000 ? DRW::AC1015 : DRW::AC1032));
        CHECK(filter.numeric == (original == Lane::Eed2000 ? 1015 : 1032));
        CHECK(filter.lastError().contains(QStringLiteral("changed while it was being read")));
    }
}

TEST_CASE("Direct host import refuses an XREF replaced after it was opened",
          "[lc-offsetcurve][import-session][xref][source-identity]") {
    Setup setup; lc::test::OutputDirectory directory("offset-direct-xref-source-change");
    const auto host = directory.file("host.dxf");
    const auto external = directory.file("external.dwg");
    const auto replacement = directory.file("replacement.dwg");
    const auto original = directory.file("opened-original.dwg");
    writeXrefHost(host, {{"REF", "external.dwg"}});
    writeInput(external, Lane::Eed18, f::raw(3));
    writeInput(replacement, Lane::Eed2000, f::raw(3));

    std::error_code error;
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_FilterDXFRW filter;
    LC_DwgReadVersionTestAccess::beforeOpen(filter, [&] {
        std::filesystem::rename(external, original, error); REQUIRE_FALSE(error);
        std::filesystem::rename(replacement, external, error); REQUIRE_FALSE(error);
    });

    CHECK_FALSE(filter.fileImport(graphic, QString::fromStdString(host.string()),
                                  RS2::FormatDXFRW2018));
    CHECK(std::filesystem::exists(original));
    CHECK(filter.lastError().contains(QStringLiteral("changed while it was being read")));
}

TEST_CASE("Direct host import embeds an unchanged XREF from its opened source",
          "[lc-offsetcurve][import-session][xref][source-identity]") {
    Setup setup; lc::test::OutputDirectory directory("offset-direct-xref-stable");
    const auto host = directory.file("host.dxf");
    const auto external = directory.file("external.dwg");
    writeXrefHost(host, {{"REF", "external.dwg"}});
    writeInput(external, Lane::Eed18, f::raw(3));

    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(host.string()),
                              RS2::FormatDXFRW2018));
    const auto *block = graphic.findBlock("REF");
    REQUIRE(block);
    REQUIRE(block->count() == 1);
    CHECK(dynamic_cast<const LC_OffsetCurve *>(block->entityAt(0)) != nullptr);
}

TEST_CASE("Direct host import revalidates earlier XREFs after later dependencies",
          "[lc-offsetcurve][import-session][xref][source-identity]") {
    Setup setup; lc::test::OutputDirectory directory("offset-direct-xref-cross-change");
    const auto host = directory.file("host.dxf");
    const auto first = directory.file("first.dwg");
    const auto second = directory.file("second.dwg");
    const auto replacement = directory.file("replacement.dwg");
    const auto openedFirst = directory.file("opened-first.dwg");
    writeXrefHost(host, {{"FIRST", "first.dwg"}, {"SECOND", "second.dwg"}});
    writeInput(first, Lane::Eed18, f::raw(3));
    writeInput(second, Lane::Eed18, f::raw(3));
    writeInput(replacement, Lane::Eed2000, f::raw(3));

    std::error_code error;
    int openedXrefs = 0;
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_FilterDXFRW filter;
    LC_DwgReadVersionTestAccess::beforeOpen(filter, [&] {
        if (++openedXrefs == 2) {
            std::filesystem::rename(first, openedFirst, error); REQUIRE_FALSE(error);
            std::filesystem::rename(replacement, first, error); REQUIRE_FALSE(error);
        }
    });

    CHECK_FALSE(filter.fileImport(graphic, QString::fromStdString(host.string()),
                                  RS2::FormatDXFRW2018));
    CHECK(openedXrefs == 2);
    CHECK(std::filesystem::exists(openedFirst));
    CHECK(filter.lastError().contains(QStringLiteral("changed while it was being read")));
}
#endif

#ifdef DWGSUPPORT
TEST_CASE("DWG source version observer precedes callbacks for files buffers and streams",
          "[lc-offsetcurve][import-session][dwg-version][observer]") {
    Setup setup; lc::test::OutputDirectory directory("offset-dwg-version-observer");
    for (const auto version : {DRW::AC1015, DRW::AC1032}) {
        const auto path = directory.file(version == DRW::AC1015 ? "r2000.dwg" : "r2018.dwg");
        f::dwgFile(path, f::raw(3), version);
        const auto bytes = lc::test::fileBytes(path); REQUIRE_FALSE(bytes.empty());
        for (const int mode : {0, 1, 2}) {
            CAPTURE(static_cast<int>(version), mode);
            struct State {
                DRW::Version observed = DRW::UNKNOWNV;
                unsigned calls = 0, headers = 0, splines = 0;
                bool throwNow = false, attemptedReentry = false, expectObserver = true;
                std::function<void()> reenter;
            } state;
            struct Interface final : lc::test::NullDrwInterface {
                explicit Interface(State& s) : state(s) {}
                State& state;
                void addHeader(const DRW_Header*) override { CHECK(state.calls == (state.expectObserver ? 1u : 0u)); ++state.headers; }
                void addSpline(const DRW_Spline*) override { CHECK(state.calls == (state.expectObserver ? 1u : 0u)); ++state.splines; }
            } sink(state);
            dwgRW reader(path.string().c_str());
            const std::string prefix("prefix\0", 7);
            std::istringstream stream(prefix + bytes, std::ios::in | std::ios::binary);
            stream.seekg(static_cast<std::streamoff>(prefix.size())); REQUIRE(stream.good());
            state.reenter = [&] {
                state.attemptedReentry = true;
                CHECK_FALSE(reader.read(&sink, false));
                CHECK_FALSE(reader.readBuffer(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size(), &sink, false));
                CHECK_FALSE(reader.readStream(stream, &sink, false));
                CHECK_FALSE(reader.getPreview()); CHECK_FALSE(reader.testReader());
                CHECK_FALSE(reader.write(&sink, version, false));
                CHECK_FALSE(reader.writeVerified(&sink, version, false, nullptr, nullptr));
            };
            reader.setReadVersionObserver([](DRW::Version value, void* context) {
                auto& target = *static_cast<State*>(context); target.observed = value; ++target.calls;
                if (target.throwNow) throw std::runtime_error("observer failure");
                if (target.reenter) target.reenter();
            }, &state);
            const bool ok = mode == 1
                ? reader.readBuffer(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size(), &sink, false)
                : mode == 2 ? reader.readStream(stream, &sink, false) : reader.read(&sink, false);
            REQUIRE(ok); CHECK(state.calls == 1); CHECK(state.observed == version); CHECK(state.attemptedReentry);
            CHECK(state.headers == 1); CHECK(state.splines > 0); CHECK(reader.getVersion() == version);
            CHECK(reader.getError() == DRW::BAD_NONE);
            state = {}; state.throwNow = true;
            CHECK_FALSE(reader.readBuffer(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size(), &sink, false));
            CHECK(state.calls == 1); CHECK(state.headers == 0); CHECK(state.splines == 0);
            CHECK(reader.getError() == DRW::BAD_READ_HEADER);
            state = {};
            CHECK_FALSE(reader.readBuffer(reinterpret_cast<const std::uint8_t*>(bytes.data()), 5, &sink, false));
            CHECK(state.calls == 0); CHECK(state.headers == 0);
            reader.setReadVersionObserver(nullptr, nullptr);
            state = {}; state.expectObserver = false;
            REQUIRE(reader.readBuffer(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size(), &sink, false));
            CHECK(state.calls == 0); CHECK(state.headers == 1); CHECK(state.splines > 0);
        }
    }
}

TEST_CASE("DWG physical version overrides a conflicting header variable",
          "[lc-offsetcurve][import-session][dwg-version][header]") {
    Setup setup;
    for (const bool dwg : {false, true}) {
        RS_Graphic graphic; graphic.initForNewDocument(); RS_FilterDXFRW filter;
        LC_DwgReadVersionTestAccess::prepareHeader(filter, graphic, dwg);
        DRW_Header header; header.addStr("$ACADVER", "AC1032", 1); filter.addHeader(&header);
        CHECK(LC_DwgReadVersionTestAccess::numericVersion(filter) == (dwg ? 1015 : 1032));
        CHECK(LC_DwgReadVersionTestAccess::inputVersion(filter) == (dwg ? DRW::AC1015 : DRW::AC1032));
    }
}

TEST_CASE("DWG stream reader rejects nonseekable and truncated input",
          "[lc-offsetcurve][import-session][dwg-version][stream]") {
    Setup setup;
    struct ForwardOnly final : std::stringbuf {
        explicit ForwardOnly(const std::string& data) : std::stringbuf(data) {}
        pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode) override { return pos_type(off_type(-1)); }
        pos_type seekpos(pos_type, std::ios_base::openmode) override { return pos_type(off_type(-1)); }
    } buffer("AC1032");
    std::istream input(&buffer); dwgRW reader(""); f::Capture sink;
    CHECK_FALSE(reader.readStream(input, &sink, false)); CHECK(reader.getError() == DRW::BAD_OPEN);
    struct NoEndSeek final : std::stringbuf {
        explicit NoEndSeek(const std::string& data) : std::stringbuf(data) {}
        pos_type seekoff(off_type offset, std::ios_base::seekdir direction, std::ios_base::openmode mode) override {
            if (direction == std::ios_base::end) return pos_type(off_type(-1));
            return std::stringbuf::seekoff(offset, direction, mode);
        }
    } noEndBuffer("AC1032");
    std::istream noEndInput(&noEndBuffer);
    CHECK(noEndInput.tellg() == std::streampos(0));
    CHECK_FALSE(reader.readStream(noEndInput, &sink, false)); CHECK(reader.getError() == DRW::BAD_OPEN);
    std::istringstream shortInput("AC10", std::ios::in | std::ios::binary);
    CHECK_FALSE(reader.readStream(shortInput, &sink, false)); CHECK(reader.getError() == DRW::BAD_VERSION);
    CHECK(sink.splines.empty());
}
#endif

TEST_CASE("G hostile malformed then valid candidate never promotes the valid suffix", "[lc-offsetcurve][g-hostile-file][duplicate]") {
    Setup setup; auto bad = f::owned(); bad[10] = std::make_shared<DRW_Variant>(1040, 1.);
    f::append(bad, f::owned()); allFiles(bad, false);
}
TEST_CASE("G hostile valid then malformed candidate never promotes the valid prefix", "[lc-offsetcurve][g-hostile-file][duplicate]") {
    Setup setup; auto bad = f::owned(); bad[3] = text(1000, "NOT_ELLIPSE");
    auto payload = f::owned(); f::append(payload, bad); allFiles(payload, false);
}
TEST_CASE("G hostile malformed duplicate strips only known candidates beside an opaque future subtree", "[lc-offsetcurve][g-hostile-file][duplicate]") {
    Setup setup; auto future = f::owned(0x40, false, 2);
    const auto nested = f::owned(); future.insert(future.end() - 1, nested.begin(), nested.end());
    auto malformed = f::owned(); malformed[2] = text(1000, "1");
    auto payload = malformed; f::append(payload, future); f::append(payload, f::owned());
    allFiles(payload, false, future);
}
TEST_CASE("G hostile marker with another string group code remains foreign-looking ordinary metadata", "[lc-offsetcurve][g-hostile-file][marker]") {
    Setup setup; auto payload = f::owned(); payload[0] = text(1003, "LC_OFFSET_CURVE");
    for (auto lane : {Lane::Ascii, Lane::Binary14, Lane::Binary18}) fileCase(lane, payload, false, payload);
}
TEST_CASE("G hostile numeric marker cannot be mistaken for the owned string marker", "[lc-offsetcurve][g-hostile-file][marker]") {
    Setup setup; auto payload = f::owned(); payload[0] = integer(1071, 1); allFiles(payload, false, payload);
}
TEST_CASE("G hostile marker spelling is case sensitive even in an owned APPID", "[lc-offsetcurve][g-hostile-file][marker]") {
    Setup setup; auto payload = f::owned(); payload[0] = text(1000, "lc_offset_curve"); allFiles(payload, false, payload);
}
TEST_CASE("G hostile brace-looking strings do not authorize an owned candidate", "[lc-offsetcurve][g-hostile-file][marker]") {
    Setup setup; auto payload = f::owned(); payload[1] = text(1000, "{"); payload.back() = text(1000, "}");
    allFiles(payload, false, payload);
}
TEST_CASE("G hostile schema INTEGER32 code cannot substitute for INTEGER16 schema one", "[lc-offsetcurve][g-hostile-file][schema]") {
    Setup setup; auto payload = f::owned(); payload[2] = integer(1071, 1); allFiles(payload, false);
}
TEST_CASE("G hostile schema DOUBLE cannot substitute for INTEGER16 schema one", "[lc-offsetcurve][g-hostile-file][schema]") {
    Setup setup; auto payload = f::owned(); payload[2] = std::make_shared<DRW_Variant>(1040, 1.); allFiles(payload, false);
}
TEST_CASE("G hostile schema decimal text cannot substitute for INTEGER16 schema one", "[lc-offsetcurve][g-hostile-file][schema]") {
    Setup setup; auto payload = f::owned(); payload[2] = text(1000, "1"); allFiles(payload, false);
}
TEST_CASE("G hostile numeric kind cannot substitute for the exact ELLIPSE string", "[lc-offsetcurve][g-hostile-file][schema]") {
    Setup setup; auto payload = f::owned(); payload[3] = integer(1070, 1); allFiles(payload, false);
}
TEST_CASE("G hostile exact tail item budget retains 64 and drops 65 items", "[lc-offsetcurve][g-hostile-file][tail][boundary]") {
    Setup setup; const unsigned count = GENERATE(64u, 65u);
    const auto tail = repeatedItems(count); allFiles(withTail(tail), true, {}, count == 64 ? tail : f::Tags{});
}
TEST_CASE("G hostile unsupported tail records still debit the raw 64 item limit", "[lc-offsetcurve][g-hostile-file][tail][boundary]") {
    Setup setup; const unsigned count = GENERATE(63u, 64u);
    const auto safe = repeatedItems(count); auto tail = safe; tail.push_back(text(1005, "1234"));
    allFiles(withTail(tail), true, {}, count == 63 ? safe : f::Tags{});
}
TEST_CASE("G hostile tail depth eight is retained and depth nine is dropped whole", "[lc-offsetcurve][g-hostile-file][tail][boundary]") {
    Setup setup; const unsigned depth = GENERATE(8u, 9u);
    const auto tail = nestedItems(depth); allFiles(withTail(tail), true, {}, depth == 8 ? tail : f::Tags{});
}
TEST_CASE("G hostile tail finite signed integer endpoints and three dimensional extension points remain exact", "[lc-offsetcurve][g-hostile-file][tail]") {
    Setup setup;
    const f::Tags tail{integer(1070, -32768), integer(1070, 32767), integer(1071, std::numeric_limits<std::int32_t>::min()),
        integer(1071, std::numeric_limits<std::int32_t>::max()), std::make_shared<DRW_Variant>(1010, DRW_Coord(1, -2, 3)),
        std::make_shared<DRW_Variant>(1042, -0.)};
    allFiles(withTail(tail), true, {}, tail);
}
TEST_CASE("G hostile unsigned INTEGER16 tail cannot be retained as a signed owned extension", "[lc-offsetcurve][g-hostile-file][tail]") {
    Setup setup; const f::Tags tail{text(1000, "valid prefix"), integer(1070, 32768), integer(1071, 9)};
    fileCase(Lane::Ascii, withTail(tail), true);
}
TEST_CASE("G hostile malformed tail brace token invalidates the known definition rather than truncating it", "[lc-offsetcurve][g-hostile-file][tail]") {
    Setup setup; const auto payload = withTail({text(1000, "before"), text(1002, "not a brace"), integer(1071, 7)});
    // DXF permits the string record; DWG's EED encoder correctly refuses it before a file exists.
    for (auto lane : {Lane::Ascii, Lane::Binary14, Lane::Binary18}) fileCase(lane, payload, false);
}
TEST_CASE("G hostile tail NUL CR and LF decoded from actual ASCII caret records drop the whole tail", "[lc-offsetcurve][g-hostile-file][tail]") {
    Setup setup;
    for (char control : {'\0', '\r', '\n'}) {
        CAPTURE(static_cast<unsigned>(control));
        const std::string escaped = std::string("bad^") + static_cast<char>(control + 0x40) + "text";
        const auto encoded = withTail({text(1000, "valid prefix"), text(1000, escaped), integer(1071, 9)});
        const auto decoded = withTail({text(1000, "valid prefix"), text(1000, std::string("bad") + control + "text"), integer(1071, 9)});
        fileCase(Lane::Ascii, encoded, true, {}, {}, &decoded);
    }
}
TEST_CASE("G hostile binary DXF CR and LF strings drop the tail without erasing the definition", "[lc-offsetcurve][g-hostile-file][tail][binary]") {
    Setup setup;
    for (auto lane : {Lane::Binary14, Lane::Binary18}) for (char control : {'\r', '\n'})
        fileCase(lane, withTail({integer(1071, 1), text(1000, std::string("bad") + control + "text"), integer(1071, 2)}), true);
}
TEST_CASE("G hostile unsupported handle and layer tail items are omitted individually in real files", "[lc-offsetcurve][g-hostile-file][tail]") {
    Setup setup;
    const f::Tags safe{text(1000, "safe before"), integer(1071, 41), text(1000, "safe after")};
    const f::Tags tail{safe[0], text(1003, "FixtureOffsets"), safe[1], text(1005, "1234"), safe[2]};
    allFiles(withTail(tail), true, {}, safe);
}
TEST_CASE("G hostile unsupported binary tail drops only that item in DXF and EED files", "[lc-offsetcurve][g-hostile-file][tail][binary]") {
    Setup setup; const f::Tags safe{text(1000, "before binary"), integer(1071, 91)};
    const auto payload = withTail({safe[0], text(1004, "00017FFF"), safe[1]});
    for (auto lane : lanes()) {
        const auto decoded = isDwg(lane)
            ? withTail({safe[0], std::make_shared<DRW_Variant>(1004, std::vector<std::uint8_t>{0, 1, 127, 255}), safe[1]})
            : payload;
        fileCase(lane, payload, true, {}, safe, &decoded);
    }
}
TEST_CASE("G hostile actual DXF nonfinite tail scalar is a codec refusal not adapter fallback", "[lc-offsetcurve][g-hostile-file][tail][codec-refusal]") {
    Setup setup; lc::test::OutputDirectory directory("offset-hostile-nonfinite-ascii");
    auto bytes = f::ascii(f::raw(3, f::metadata(withTail({std::make_shared<DRW_Variant>(1042, 17.25)}))));
    replaceUnique(bytes, "1042\n17.25\n", "1042\nnan\n");
    const auto input = directory.file("nan.dxf"); f::put(input, bytes); codecRefuses(input);
}
TEST_CASE("G hostile actual DXF nonfinite tail coordinate is a codec refusal not a sanitized extension", "[lc-offsetcurve][g-hostile-file][tail][codec-refusal]") {
    Setup setup; lc::test::OutputDirectory directory("offset-hostile-nonfinite-point");
    auto bytes = f::ascii(f::raw(3, f::metadata(withTail({std::make_shared<DRW_Variant>(1010, DRW_Coord(17.25, 18.5, 19.75))}))));
    replaceUnique(bytes, "1030\n19.75\n", "1030\ninf\n");
    const auto input = directory.file("inf.dxf"); f::put(input, bytes); codecRefuses(input);
}
TEST_CASE("G hostile actual DXF INTEGER16 overflow fails the codec before candidate admission", "[lc-offsetcurve][g-hostile-file][tail][codec-refusal]") {
    Setup setup; lc::test::OutputDirectory directory("offset-hostile-int16-overflow");
    const auto invalid = GENERATE(std::string("65536"), std::string("-32769"));
    auto bytes = f::ascii(f::raw(3, f::metadata(withTail({integer(1070, 12345)}))));
    replaceUnique(bytes, "1070\n12345\n", "1070\n" + invalid + "\n");
    const auto input = directory.file("overflow.dxf"); f::put(input, bytes); codecRefuses(input);
}
TEST_CASE("G hostile actual DXF INTEGER32 overflow fails the codec before candidate admission", "[lc-offsetcurve][g-hostile-file][tail][codec-refusal]") {
    Setup setup; lc::test::OutputDirectory directory("offset-hostile-int32-overflow");
    const auto invalid = GENERATE(std::string("2147483648"), std::string("-2147483649"));
    auto bytes = f::ascii(f::raw(3, f::metadata(withTail({integer(1071, 12345)}))));
    replaceUnique(bytes, "1071\n12345\n", "1071\n" + invalid + "\n");
    const auto input = directory.file("overflow.dxf"); f::put(input, bytes); codecRefuses(input);
}
TEST_CASE("G hostile nonfinite binary DXF tail bytes fail the real codec", "[lc-offsetcurve][g-hostile-file][tail][binary][codec-refusal]") {
    Setup setup;
    for (auto lane : {Lane::Binary14, Lane::Binary18}) {
        lc::test::OutputDirectory directory("offset-hostile-nonfinite-binary");
        const auto input = directory.file("nonfinite.dxf");
        writeInput(input, lane, f::raw(3, f::metadata(withTail({std::make_shared<DRW_Variant>(1042, 17.25)}))));
        auto bytes = lc::test::fileBytes(input);
        const auto record = [](std::uint64_t value) {
            std::string result{"\x12\x04", 2}; // Binary DXF group 1042, little endian.
            for (unsigned i = 0; i < 8; ++i) result.push_back(static_cast<char>((value >> (8*i)) & 255));
            return result;
        };
        replaceUnique(bytes, record(f::bits(17.25)), record(0x7ff0000000000000ULL));
        f::put(input, bytes); codecRefuses(input);
    }
}

#ifdef DWGSUPPORT
TEST_CASE("G hostile actual EED embedded NUL CR and LF drop the entire owned tail", "[lc-offsetcurve][g-hostile-file][tail][dwg]") {
    Setup setup;
    for (auto lane : {Lane::Eed2000, Lane::Eed18}) for (char control : {'\0', '\r', '\n'})
        fileCase(lane, withTail({integer(1071, 1), text(1000, std::string("bad") + control + "text"), integer(1071, 2)}), true);
}
TEST_CASE("G hostile actual DWG tail byte budget includes legacy or Unicode framing", "[lc-offsetcurve][g-hostile-file][tail][dwg][boundary]") {
    Setup setup; const bool unicode = GENERATE(false, true); const bool over = GENERATE(false, true);
    f::Tags tail;
    for (unsigned i = 0; i < 15; ++i) tail.push_back(text(1000, std::string(unicode ? 120 : 255, 'x')));
    tail.push_back(text(1000, std::string((unicode ? 224 : 207) + (over ? 1 : 0), 'y')));
    // Unicode: 15*243+451=4096 (then 4098); legacy: 15*259+211=4096 (then 4097).
    fileCase(unicode ? Lane::Eed18 : Lane::Eed2000, withTail(tail), true, {}, over ? f::Tags{} : tail);
}

namespace {
f::Tags eedUnit(bool unicode, bool over) {
    f::Tags result{text(1002, "{")};
    if (unicode) {
        // 2 + (3 + 2*32764) + 2 == 65535; one more UTF-16 unit gives 65537.
        result.push_back(text(1000, std::string(over ? 32765 : 32764, 'x')));
    } else {
        // 2 + 253*(4+255) + (4+0) + 2 == 65535; one extra byte gives 65536.
        for (unsigned i = 0; i < 253; ++i) result.push_back(text(1000, std::string(255, 'x')));
        result.push_back(text(1000, over ? "x" : ""));
    }
    result.push_back(text(1002, "}")); return result;
}
void addForeignUnit(LC_OffsetCurve& curve, const f::Tags& unit) {
    auto tags = curve.getDrwExtData();
    // Reuse the real input's registered APPID, not a missing-reference failure.
    const auto next = std::find_if(tags.begin() + 1, tags.end(), [](const auto& tag) { return tag->code() == 1001; });
    REQUIRE(next != tags.end()); tags.insert(next, unit.begin(), unit.end()); curve.setDrwExtData(tags);
}
}
TEST_CASE("G hostile exact 65535 byte EED braced unit publishes and reopens with native schema", "[lc-offsetcurve][g-hostile-file][dwg][boundary]") {
    Setup setup; const bool unicode = GENERATE(false, true);
    lc::test::OutputDirectory directory("offset-hostile-eed-unit-positive");
    const auto input = directory.file("input.dxf"); f::put(input, f::ascii(f::raw(3)));
    RS_Graphic graphic; REQUIRE(lc::test::importDrawing(graphic, input, RS2::FormatDXFRW2018));
    auto* curve = f::native(graphic, 3); addForeignUnit(*curve, eedUnit(unicode, false));
    const auto expected = f::tagsState(f::application(curve->getDrwExtData(), "FOREIGN_A")); Snapshot before(graphic);
    const auto target = directory.file("exact.dwg"); const auto targetFormat = unicode ? RS2::FormatDWG2018 : RS2::FormatDWG;
    REQUIRE(lc::test::exportDrawing(graphic, target, targetFormat)); before.check();
    const auto raw = f::readCodec(target, true); REQUIRE(raw.splines.size() == 1);
    CHECK(f::tagsState(f::application(raw.splines.front().extData, "FOREIGN_A")) == expected);
    CHECK(f::markerCount(raw.splines.front().extData) == 1);
    RS_Graphic reopened; REQUIRE(lc::test::importDrawing(reopened, target, targetFormat));
    auto* again = f::native(reopened, 3);
    CHECK(f::tagsState(f::application(again->getDrwExtData(), "FOREIGN_A")) == expected); before.check();
}
TEST_CASE("G hostile indivisible EED unit over 65535 never publishes and retains real redo history", "[lc-offsetcurve][g-hostile-file][dwg][save-refusal]") {
    Setup setup; const bool unicode = GENERATE(false, true);
    for (bool existing : {false, true}) {
        CAPTURE(existing, unicode); lc::test::OutputDirectory directory("offset-hostile-eed-unit-refusal");
        const auto input = directory.file("input.dxf"); f::put(input, f::ascii(f::raw(3)));
        lc::test::EditableDrawing drawing; auto& graphic = drawing.m_graphic;
        REQUIRE(lc::test::importDrawing(graphic, input, RS2::FormatDXFRW2018));
        auto* curve = f::native(graphic, 3); addForeignUnit(*curve, eedUnit(unicode, true)); seedHistory(drawing);
        const Snapshot before(graphic); const auto target = directory.file("target.dwg");
        const std::string sentinel("prior drawing\0keep every byte", 28); if (existing) f::put(target, sentinel);
        const auto entries = directory.entries(); const auto targetFormat = unicode ? RS2::FormatDWG2018 : RS2::FormatDWG;
        RS_FilterDXFRW writer; REQUIRE_FALSE(writer.fileExport(graphic, lc::test::toQString(target), targetFormat));
        before.check(); CHECK(writer.dimStyleTargetVersion() == DRW::UNKNOWNV); CHECK(directory.entries() == entries);
        if (existing) CHECK(lc::test::fileBytes(target) == sentinel); else CHECK_FALSE(std::filesystem::exists(target));
        RS_Graphic retry; REQUIRE(lc::test::importDrawing(retry, input, RS2::FormatDXFRW2018));
        const auto retryPath = directory.file("retry.dwg"); REQUIRE(writer.fileExport(retry, lc::test::toQString(retryPath), targetFormat));
        RS_Graphic reopened; REQUIRE(lc::test::importDrawing(reopened, retryPath, targetFormat)); f::native(reopened, 3); before.check();
        REQUIRE(graphic.redo()); CHECK(f::live(graphic).size() == 2); REQUIRE(graphic.undo()); before.check();
    }
}
#endif

namespace {
class ThrowAfterEntities final : public RS_FilterDXFRW {
public:
    bool throwNext = true;
    unsigned visits = 0;
    void writeEntities() override {
        RS_FilterDXFRW::writeEntities(); ++visits;
        if (throwNext) { throwNext = false; throw std::runtime_error("test failure after actual native entity serialization"); }
    }
};
struct Verification {
    unsigned calls = 0;
    static bool visit(const std::filesystem::path&, void* context) {
        ++static_cast<Verification*>(context)->calls; return true;
    }
};
}
TEST_CASE("G hostile actual native writer callback failure cannot publish a completed prefix", "[lc-offsetcurve][g-hostile-file][save-refusal][callback]") {
    Setup setup;
    std::vector<RS2::FormatType> formats{RS2::FormatDXFRW2018};
#ifdef DWGSUPPORT
    formats.push_back(RS2::FormatDWG2018);
#endif
    for (auto targetFormat : formats) for (bool existing : {false, true}) {
        CAPTURE(targetFormat, existing); lc::test::OutputDirectory directory("offset-hostile-callback-failure");
        const auto input = directory.file("input.dxf"); f::put(input, f::ascii(f::raw(3)));
        lc::test::EditableDrawing drawing; auto& graphic = drawing.m_graphic;
        REQUIRE(lc::test::importDrawing(graphic, input, RS2::FormatDXFRW2018)); f::native(graphic, 3); seedHistory(drawing);
        const Snapshot before(graphic);
        const auto suffix = targetFormat == RS2::FormatDXFRW2018 ? ".dxf" : ".dwg";
        const auto target = directory.file(std::string("target") + suffix);
        const std::string sentinel = "old target must survive"; if (existing) f::put(target, sentinel);
        const auto entries = directory.entries(); ThrowAfterEntities writer;
        REQUIRE_FALSE(writer.fileExport(graphic, lc::test::toQString(target), targetFormat)); REQUIRE(writer.visits == 1);
        CHECK(writer.dimStyleTargetVersion() == DRW::UNKNOWNV); before.check(); CHECK(directory.entries() == entries);
        if (existing) CHECK(lc::test::fileBytes(target) == sentinel); else CHECK_FALSE(std::filesystem::exists(target));
        const auto retry = directory.file(std::string("retry") + suffix); REQUIRE(writer.fileExport(graphic, lc::test::toQString(retry), targetFormat));
        CHECK(writer.visits == 2); CHECK(writer.dimStyleTargetVersion() == DRW::UNKNOWNV); before.check();
        RS_Graphic reopened; REQUIRE(lc::test::importDrawing(reopened, retry, targetFormat)); f::native(reopened, 3);
    }
}
TEST_CASE("G hostile existing publisher stream failure cannot publish real native drawing bytes", "[lc-offsetcurve][g-hostile-file][save-refusal][stream]") {
    Setup setup;
    std::vector<Lane> outputLanes{Lane::Ascii};
#ifdef DWGSUPPORT
    outputLanes.push_back(Lane::Eed18);
#endif
    for (auto lane : outputLanes) for (bool existing : {false, true}) {
        CAPTURE(static_cast<int>(lane), existing); lc::test::OutputDirectory directory("offset-hostile-stream-failure");
        const auto input = directory.file("input.dxf"); f::put(input, f::ascii(f::raw(3)));
        RS_Graphic graphic; REQUIRE(lc::test::importDrawing(graphic, input, RS2::FormatDXFRW2018)); f::native(graphic, 3);
        const auto complete = directory.file(isDwg(lane) ? "complete.dwg" : "complete.dxf");
        REQUIRE(lc::test::exportDrawing(graphic, complete, format(lane)));
        RS_Graphic reopened; REQUIRE(lc::test::importDrawing(reopened, complete, format(lane))); f::native(reopened, 3);
        const auto bytes = lc::test::fileBytes(complete); REQUIRE_FALSE(bytes.empty()); const Snapshot before(graphic);
        const auto target = directory.file("target.drawing"); const std::string sentinel = "prior drawing bytes";
        if (existing) f::put(target, sentinel); const auto entries = directory.entries();
        DwgDxfOutputTransaction output(target.string(), std::ios::binary); REQUIRE(output.open());
        output.stream().write(bytes.data(), static_cast<std::streamsize>(bytes.size())); REQUIRE(output.stream().good());
        // Existing public publisher seam, not an invented filter I/O hook or a simulated C result.
        output.stream().setstate(std::ios::badbit); output.stream().put('!'); REQUIRE(output.stream().fail());
        Verification verification;
        REQUIRE_FALSE(output.commitVerified(&Verification::visit, &verification)); CHECK(verification.calls == 0);
        CHECK_FALSE(output.isOpen()); CHECK_FALSE(output.commit()); CHECK(directory.entries() == entries); before.check();
        if (existing) CHECK(lc::test::fileBytes(target) == sentinel); else CHECK_FALSE(std::filesystem::exists(target));
    }
}

TEST_CASE("Staged offset import resolves a private drawing through the real helper",
          "[lc-offsetcurve][import-session][worker]") {
    Setup setup;
    for (const auto lane : lanes()) {
        CAPTURE(static_cast<int>(lane));
        lc::test::OutputDirectory directory("offset-staged-import");
        const auto input = directory.file(isDwg(lane) ? "input.dwg" : "input.dxf");
        writeInput(input, lane, f::raw(3));
        auto outcome = stagedImport(input, lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic); CHECK_FALSE(outcome.partial);
        CHECK(outcome.sourceStamp.valid); CHECK_FALSE(outcome.sourceUnverified);
        CHECK(outcome.admitted == 1); CHECK(outcome.downgraded == 0);
        CHECK(outcome.diagnostics.empty()); CHECK(outcome.suppressedDiagnostics == 0);
        auto* native = f::native(*outcome.graphic, 3); REQUIRE(native);
        CHECK(native->unknownOwnedFlags() == 0x40); CHECK(native->unknownOwnedTail().size() == 5);
        f::foreign(*native);
        RS_Graphic synchronous; REQUIRE(lc::test::importDrawing(synchronous, input, format(lane)));
        CHECK(outcome.graphic->getFormatType() == synchronous.getFormatType());
        auto* expected = f::native(synchronous, 3); REQUIRE(expected);
        CHECK(RS_Spline::geometryFingerprint(native->getData()) == RS_Spline::geometryFingerprint(expected->getData()));
        CHECK(f::tagsState(native->getDrwExtData()) == f::tagsState(expected->getDrwExtData()));
        CHECK(native->getPen(false) == expected->getPen(false));
        REQUIRE(native->getLayer(false)); REQUIRE(expected->getLayer(false));
        CHECK(native->getLayer(false)->getName() == expected->getLayer(false)->getName());
        CHECK(native->checkedWorldTolerance() == expected->checkedWorldTolerance());
    }
}

TEST_CASE("Staged offset import keeps ordinary fallback when cache validation refuses",
          "[lc-offsetcurve][import-session][worker][refusal]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-staged-refusal");
    const auto input = directory.file("input.dxf");
    auto raw = f::raw(3); raw.controllist[1]->x += 8;
    writeInput(input, Lane::Ascii, raw);
    auto outcome = stagedImport(input, Lane::Ascii);
    REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete); REQUIRE(outcome.graphic);
    CHECK(outcome.admitted == 0); CHECK(outcome.downgraded == 1);
    REQUIRE(outcome.diagnostics.size() == 1);
    CHECK(outcome.diagnostics.front().kind == LC_OffsetImportDiagnostic::Kind::Validation);
    auto* entity = f::sole(*outcome.graphic); REQUIRE(entity);
    CHECK(entity->rtti() == RS2::EntitySpline); CHECK(dynamic_cast<LC_OffsetCurve*>(entity) == nullptr);
    CHECK(dynamic_cast<LC_OffsetImportPlaceholder*>(entity) == nullptr); f::foreign(*entity);
}

TEST_CASE("Staged offset import reports malformed schema and raw carrier refusals",
          "[lc-offsetcurve][import-session][refusal]") {
    Setup setup;
    for (bool malformedSchema : {false, true}) {
        CAPTURE(malformedSchema); lc::test::OutputDirectory directory("offset-staged-malformed");
        const auto input = directory.file("input.dxf"); auto raw = f::raw(3);
        if (malformedSchema) { auto payload = f::owned(); f::append(payload, f::owned()); raw.extData = f::metadata(payload); }
        else raw.knotslist[1] = .125;
        writeInput(input, Lane::Ascii, raw);
        auto outcome = stagedImport(input, Lane::Ascii);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete); REQUIRE(outcome.graphic);
        CHECK(outcome.admitted == 0); CHECK(outcome.downgraded == 1);
        REQUIRE_FALSE(outcome.diagnostics.empty());
        const auto wanted = malformedSchema ? LC_OffsetImportDiagnostic::Kind::XData : LC_OffsetImportDiagnostic::Kind::Carrier;
        CHECK(std::any_of(outcome.diagnostics.begin(), outcome.diagnostics.end(),
            [&](const LC_OffsetImportDiagnostic& diagnostic) { return diagnostic.kind == wanted; }));
        auto* entity = f::sole(*outcome.graphic); REQUIRE(entity);
        CHECK(dynamic_cast<LC_OffsetImportPlaceholder*>(entity) == nullptr);
    }
}

TEST_CASE("Staged offset import cancels without delivering a private drawing",
          "[lc-offsetcurve][import-session][lifetime][asan]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-cancel");
    const auto input = directory.file("input.dxf"); writeInput(input, Lane::Ascii, f::raw(3));
    bool delivered = false;
    {
        LC_OffsetImportSession session;
        REQUIRE(session.start(QString::fromStdString(input.string()), RS2::FormatDXFRW2018,
            RS2::DxfTextEncoding::Declared, false, [&](LC_OffsetImportSession::Outcome) { delivered = true; }));
        REQUIRE(session.busy()); session.cancel(); CHECK_FALSE(session.busy());
    }
    QElapsedTimer elapsed; elapsed.start();
    while (elapsed.elapsed() < 100) { QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
    CHECK_FALSE(delivered);
}

TEST_CASE("Staged offset source capture yields cancels and detects changes",
          "[lc-offsetcurve][import-session][snapshot]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-snapshot");
    const auto path = directory.file("input.dxf");
    f::put(path, f::ascii(f::raw(3)) + std::string(600000, '\n'));
    const int scenario = GENERATE(0, 1, 2, 3); CAPTURE(scenario);
    LC_OffsetImportSession session; std::optional<LC_OffsetImportSession::Outcome> outcome;
    bool hookCalled = false, heartbeat = false;
    LC_OffsetImportSessionTestAccess::afterSnapshotChunk(session, [&] {
        hookCalled = true;
        if (scenario == 0) QTimer::singleShot(0, [&] { heartbeat = true; });
        else if (scenario == 1) session.cancel();
        else if (scenario == 2) {
            QFile file(QString::fromStdString(path.string())); REQUIRE(file.open(QIODevice::Append));
            REQUIRE(file.write("changed", 7) == 7); file.close();
        } else {
            std::filesystem::rename(path, directory.file("original.dxf"));
            f::put(path, "0\nEOF\n");
        }
    });
    REQUIRE(session.start(QString::fromStdString(path.string()), format(Lane::Ascii),
        RS2::DxfTextEncoding::Declared, false, [&](LC_OffsetImportSession::Outcome result) { outcome.emplace(std::move(result)); }));
    QElapsedTimer elapsed; elapsed.start();
    while (!hookCalled || session.busy()) {
        REQUIRE(elapsed.elapsed() < 35000);
        QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1);
    }
    REQUIRE(hookCalled);
    if (scenario == 0) {
        REQUIRE(outcome); CHECK(heartbeat); REQUIRE(outcome->status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome->graphic); CHECK(f::native(*outcome->graphic, 3) != nullptr);
    }
    else if (scenario == 1) {
        CHECK_FALSE(outcome);
        auto retry = stagedImport(path, Lane::Ascii);
        REQUIRE(retry.status == LC_OffsetImportSession::Status::Complete); REQUIRE(retry.graphic);
        CHECK(f::native(*retry.graphic, 3) != nullptr);
    }
    else {
        REQUIRE(outcome); CHECK(outcome->status == LC_OffsetImportSession::Status::SourceChanged);
        CHECK_FALSE(outcome->graphic);
    }
}

#ifdef Q_OS_WIN
TEST_CASE("Windows staged source identity binds its open handle",
          "[lc-offsetcurve][import-session][snapshot][identity]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-win-identity");
    const auto path = directory.file("input.dxf"); writeInput(path, Lane::Ascii, f::raw(3));
    const auto stamp = LC_OffsetImportSourceStamp::capture(QString::fromStdString(path.string()));
    REQUIRE(stamp.valid); REQUIRE(stamp.native.valid);
    QFile file(QString::fromStdString(path.string())); REQUIRE(file.open(QIODevice::ReadOnly));
    REQUIRE(stamp.matchesDescriptor(file.handle()));
    {
        QFile writer(QString::fromStdString(path.string())); REQUIRE(writer.open(QIODevice::Append));
        REQUIRE(writer.write("X", 1) == 1); REQUIRE(writer.flush());
    }
    CHECK_FALSE(stamp.matchesDescriptor(file.handle()));
    CHECK_FALSE(stamp.matches(QString::fromStdString(path.string())));
}

TEST_CASE("Windows staged source replacement before open is refused",
          "[lc-offsetcurve][import-session][snapshot][identity]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-win-swap");
    const auto path = directory.file("input.dxf"), previous = directory.file("previous.dxf");
    writeInput(path, Lane::Ascii, f::raw(3));
    const auto original = lc::test::fileBytes(path);
    LC_OffsetImportSession session; std::optional<LC_OffsetImportSession::Outcome> outcome;
    bool parsed = false;
    LC_OffsetImportSessionTestAccess::beforeSourceOpen(session, [&] {
        std::filesystem::rename(path, previous); f::put(path, original);
    });
    LC_OffsetImportSessionTestAccess::afterRead(session, [&] { parsed = true; });
    REQUIRE(session.start(QString::fromStdString(path.string()), format(Lane::Ascii),
        RS2::DxfTextEncoding::Declared, false, [&](LC_OffsetImportSession::Outcome result) { outcome.emplace(std::move(result)); }));
    QElapsedTimer elapsed; elapsed.start();
    while (!outcome) { REQUIRE(elapsed.elapsed() < 5000); QCoreApplication::processEvents(QEventLoop::AllEvents); }
    CHECK(outcome->status == LC_OffsetImportSession::Status::SourceChanged);
    CHECK_FALSE(outcome->graphic); CHECK_FALSE(parsed);
}
#endif

#ifdef Q_OS_UNIX
TEST_CASE("Staged source identity binds path and open descriptor",
          "[lc-offsetcurve][import-session][snapshot][identity]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-identity");
    const auto path = directory.file("input.dxf"); writeInput(path, Lane::Ascii, f::raw(3));
    const auto symlink = directory.file("symlink.dxf"), hardlink = directory.file("hardlink.dxf");
    std::filesystem::create_symlink(path, symlink);
    std::filesystem::create_hard_link(path, hardlink);
    for (const auto& linked : {path, symlink, hardlink}) {
        CAPTURE(linked.string());
        const auto stamp = LC_OffsetImportSourceStamp::capture(QString::fromStdString(linked.string()));
        REQUIRE(stamp.valid); REQUIRE(stamp.native.valid);
        QFile opened(QString::fromStdString(linked.string())); REQUIRE(opened.open(QIODevice::ReadOnly));
        CHECK(stamp.matchesDescriptor(opened.handle())); CHECK(stamp.matches(QString::fromStdString(linked.string())));
    }
    const auto stamp = LC_OffsetImportSourceStamp::capture(QString::fromStdString(path.string()));
    QFile opened(QString::fromStdString(path.string())); REQUIRE(opened.open(QIODevice::ReadOnly));
    REQUIRE(stamp.matchesDescriptor(opened.handle()));
    QFile modified(QString::fromStdString(path.string())); REQUIRE(modified.open(QIODevice::Append));
    REQUIRE(modified.write("x", 1) == 1); modified.close();
    CHECK_FALSE(stamp.matchesDescriptor(opened.handle())); CHECK_FALSE(stamp.matches(QString::fromStdString(path.string())));
}

TEST_CASE("Staged source swap before open refuses before parsing",
          "[lc-offsetcurve][import-session][snapshot][identity]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-open-swap");
    const auto path = directory.file("input.dxf"), previous = directory.file("previous.dxf");
    writeInput(path, Lane::Ascii, f::raw(3));
    const auto original = lc::test::fileBytes(path);
    LC_OffsetImportSession session; std::optional<LC_OffsetImportSession::Outcome> outcome;
    bool parsed = false;
    LC_OffsetImportSessionTestAccess::beforeSourceOpen(session, [&] {
        std::filesystem::rename(path, previous); f::put(path, original);
    });
    LC_OffsetImportSessionTestAccess::afterRead(session, [&] { parsed = true; });
    REQUIRE(session.start(QString::fromStdString(path.string()), format(Lane::Ascii),
        RS2::DxfTextEncoding::Declared, false, [&](LC_OffsetImportSession::Outcome result) { outcome.emplace(std::move(result)); }));
    QElapsedTimer elapsed; elapsed.start();
    while (!outcome) { REQUIRE(elapsed.elapsed() < 5000); QCoreApplication::processEvents(QEventLoop::AllEvents); }
    CHECK(outcome->status == LC_OffsetImportSession::Status::SourceChanged);
    CHECK_FALSE(outcome->graphic); CHECK_FALSE(parsed);
}

#if defined(Q_OS_DARWIN) || defined(Q_OS_LINUX)
TEST_CASE("Staged source descriptor detects a same-size rewrite with restored mtime",
          "[lc-offsetcurve][import-session][snapshot][identity]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-rewrite");
    const auto path = directory.file("input.dxf"); writeInput(path, Lane::Ascii, f::raw(3));
    const auto stamp = LC_OffsetImportSourceStamp::capture(QString::fromStdString(path.string()));
    REQUIRE(stamp.native.valid);
    QFile file(QString::fromStdString(path.string())); REQUIRE(file.open(QIODevice::ReadWrite));
    REQUIRE(stamp.matchesDescriptor(file.handle()));
    struct stat before{}; REQUIRE(::fstat(file.handle(), &before) == 0);
    REQUIRE(file.write("X", 1) == 1); REQUIRE(file.flush());
#if defined(Q_OS_DARWIN)
    const timespec times[] = {before.st_atimespec, before.st_mtimespec};
#else
    const timespec times[] = {before.st_atim, before.st_mtim};
#endif
    REQUIRE(::futimens(file.handle(), times) == 0);
    struct stat after{}; REQUIRE(::fstat(file.handle(), &after) == 0);
    CHECK(after.st_size == before.st_size);
    const auto current = LC_OffsetImportSourceStamp::capture(QString::fromStdString(path.string()));
    CHECK(current.native.modifiedSeconds == stamp.native.modifiedSeconds);
    CHECK(current.native.modifiedNanos == stamp.native.modifiedNanos);
    CHECK_FALSE(stamp.matchesDescriptor(file.handle())); CHECK_FALSE(stamp.matches(QString::fromStdString(path.string())));
}
#endif

TEST_CASE("Staged offset source capture refuses oversized input",
          "[lc-offsetcurve][import-session][snapshot][limit]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-snapshot-limit");
    const auto path = directory.file("large.dxf"); f::put(path, "0\nEOF\n");
    REQUIRE(::truncate(path.c_str(), 256LL * 1024 * 1024 + 1) == 0);
    LC_OffsetImportSession session; std::optional<LC_OffsetImportSession::Outcome> outcome;
    REQUIRE(session.start(QString::fromStdString(path.string()), format(Lane::Ascii),
        RS2::DxfTextEncoding::Declared, false, [&](LC_OffsetImportSession::Outcome result) { outcome.emplace(std::move(result)); }));
    QElapsedTimer elapsed; elapsed.start();
    while (!outcome) { REQUIRE(elapsed.elapsed() < 5000); QCoreApplication::processEvents(QEventLoop::AllEvents); }
    CHECK(outcome->status == LC_OffsetImportSession::Status::ResourceLimit);
    CHECK_FALSE(outcome->graphic); CHECK_FALSE(outcome->error.isEmpty());
}
#endif

TEST_CASE("Staged offset import handles a killed helper without exposing its carrier",
          "[lc-offsetcurve][import-session][worker][fault]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-worker-fault");
    const auto input = directory.file("input.dxf"); writeInput(input, Lane::Ascii, f::raw(3));
    LC_OffsetImportSession session; bool started = false;
    LC_OffsetWorkerClientTestAccess::onStarted(LC_OffsetImportSessionTestAccess::worker(session),
        [&](QProcess& process) { started = process.processId() > 0; process.kill(); });
    std::optional<LC_OffsetImportSession::Outcome> outcome;
    REQUIRE(session.start(QString::fromStdString(input.string()), RS2::FormatDXFRW2018,
        RS2::DxfTextEncoding::Declared, false, [&](LC_OffsetImportSession::Outcome result) { outcome.emplace(std::move(result)); }));
    QElapsedTimer elapsed; elapsed.start();
    while (!outcome) { REQUIRE(elapsed.elapsed() < 35000); QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
    REQUIRE(started); REQUIRE(outcome->status == LC_OffsetImportSession::Status::Complete); REQUIRE(outcome->graphic);
    CHECK(outcome->admitted == 0); CHECK(outcome->downgraded == 1);
    REQUIRE(outcome->diagnostics.size() == 1);
    CHECK(outcome->diagnostics.front().kind == LC_OffsetImportDiagnostic::Kind::Worker);
    auto* entity = f::sole(*outcome->graphic); REQUIRE(entity);
    CHECK(entity->rtti() == RS2::EntitySpline); CHECK(dynamic_cast<LC_OffsetImportPlaceholder*>(entity) == nullptr);
}

TEST_CASE("Staged offset import bounds its aggregate carrier allowance",
          "[lc-offsetcurve][import-session][quota]") {
    LC_OffsetImportBudget budget; RS_SplineData cache;
    LC_OffsetCurveXData::Definition definition;
    for (std::size_t i = 0; i < LC_OffsetImportBudget::MaximumCarriers; ++i) REQUIRE(budget.admit(cache, definition, 0));
    CHECK_FALSE(budget.admit(cache, definition, 0)); CHECK(budget.refused == 1);
    LC_OffsetImportBudget bytes; cache.controlPoints.resize(LC_OffsetImportBudget::MaximumBytes / (3 * sizeof(RS_Vector)) + 1);
    CHECK_FALSE(bytes.admit(cache, definition, 0)); CHECK(bytes.carriers == 0); CHECK(bytes.refused == 1);
    LC_OffsetImportBudget metadata; RS_SplineData small;
    CHECK_FALSE(metadata.admit(small, definition, LC_OffsetImportBudget::MaximumBytes / sizeof(std::shared_ptr<DRW_Variant>)));
    for (unsigned i = 0; i < 40; ++i) metadata.report(LC_OffsetImportDiagnostic::Kind::Carrier, int(i), i);
    CHECK(metadata.diagnostics.size() == 32); CHECK(metadata.suppressedDiagnostics == 9);
}

TEST_CASE("typed offset source survives real DXF and DWG file save reopen and second save",
          "[lc-offsetcurve][persistence][typed-source][file]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-typed-source-file-roundtrip");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;
    RS_SplineData splineSource{2, false};
    splineSource.controlPoints = {{0, 0}, {1, 2}, {3, 0}};
    splineSource.knotslist = {0, 0, 0, 1, 1, 1};
    splineSource.weights = {1, .7, 1};
    RS_SplineData nonuniformQuadratic{2, false};
    nonuniformQuadratic.controlPoints = {{0, 0}, {1, 2}, {2, -1}, {3, 2}, {4, 0}};
    nonuniformQuadratic.knotslist = {0, 0, 0, .3, .7, 1, 1, 1};
    nonuniformQuadratic.weights = {1, .6, 1.8, .8, 1};
    RS_SplineData repeatedKnotCubic{3, false};
    repeatedKnotCubic.controlPoints = {{0, 0}, {1, 2}, {2, 2}, {3, -1}, {4, -1}, {5, 1}};
    repeatedKnotCubic.knotslist = {0, 0, 0, 0, .4, .4, 1, 1, 1, 1};
    repeatedKnotCubic.weights = {1, .7, 1.4, .9, 1.6, 1};
    RS_SplineData unweightedSpline{1, false};
    unweightedSpline.controlPoints = {{0, 0}, {1, 0}, {2, 0}};
    unweightedSpline.knotslist = {0, 0, .5, 1, 1};
    RS_Spline periodic{nullptr, RS_SplineData{3, false}};
    for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30},
                                   RS_Vector{20, 50}, RS_Vector{-15, 25}})
        periodic.addControlPoint(point);
    periodic.setClosed(true);
    auto wrappedSpline = periodic.getData();
    wrappedSpline.weights.clear();
    LC_SplinePointsData splinePoints{false, false};
    splinePoints.useControlPoints = true;
    splinePoints.controlPoints = {{0, 0}, {1, 1}, {2, 0}};
    const double quarterTurn = M_PI / 2.0;
    LC_OffsetPolylineData closedArcPolyline{{2, 0}, {2, 0}, true, {
        RS_ArcData{{0, 0}, 2, 0, quarterTurn, false},
        RS_ArcData{{0, 0}, 2, quarterTurn, M_PI, false},
        RS_ArcData{{0, 0}, 2, M_PI, 3 * quarterTurn, false},
        RS_ArcData{{0, 0}, 2, 3 * quarterTurn, 2 * M_PI, false}}};
    const std::array<LC_OffsetSourceGeometry, 14> sources{
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {2, 0}, .5, 0, 1, false}},
        LC_OffsetSourceGeometry{splineSource},
        LC_OffsetSourceGeometry{nonuniformQuadratic},
        LC_OffsetSourceGeometry{repeatedKnotCubic},
        LC_OffsetSourceGeometry{unweightedSpline},
        LC_OffsetSourceGeometry{wrappedSpline}, LC_OffsetSourceGeometry{splinePoints},
        LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{0, 1}, RS_Vector{1, 0}, RS_Vector{2, 1}}}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {2, 0}, .5, -.5, .5}},
        LC_OffsetSourceGeometry{RS_LineData{{0, 0}, {2, 0}}},
        LC_OffsetSourceGeometry{RS_ArcData{{0, 0}, 2, 0, 1, false}},
        LC_OffsetSourceGeometry{RS_CircleData{{0, 0}, 2}},
        LC_OffsetSourceGeometry{LC_OffsetPolylineData{{0, 0}, {2, 0}, false,
            {RS_LineData{{0, 0}, {2, 0}}}}},
        LC_OffsetSourceGeometry{closedArcPolyline}
    };
    const auto app = std::make_shared<DRW_Variant>(1001, std::string("LibreCad"));
    for (std::size_t i = 0; i < sources.size(); ++i) {
        CAPTURE(i);
        xd::Definition definition;
        definition.source = LC_OffsetSourceDefinition{sources[i], -.75, .00025, true};
        definition.reservedFlags = 0x10;
        definition.tail = {{1000, QStringLiteral("file roundtrip tail")}, {1070, std::int16_t{23}}};

        const auto verify = [&](const std::filesystem::path &path, bool dwg, bool secondSave) {
            const auto captured = f::readCodec(path, dwg);
            REQUIRE(captured.splines.size() == 1);
            xd::Options options;
            if (dwg) {
                options.eed = true;
                options.textEncoding = xd::TextEncoding::Utf16;
            } else {
                options.asciiDxf = false;
            }
            CAPTURE(f::tagsState(captured.splines.front().extData));
            const auto parsed = xd::parseAndStrip(captured.splines.front().extData, options);
            REQUIRE(parsed.status == xd::Status::Schema2);
            REQUIRE(parsed.definition);
            REQUIRE(parsed.definition->source);
            CHECK(parsed.definition->reservedFlags == definition.reservedFlags);
            REQUIRE(parsed.definition->tail.size() == definition.tail.size());
            const auto before = values::encodeSourceDefinition(*definition.source);
            const auto after = values::encodeSourceDefinition(*parsed.definition->source);
            REQUIRE(before); REQUIRE(after);
            CHECK(QCborValue(*before).toCbor() == QCborValue(*after).toCbor());
            if (!secondSave) return;

            const auto repeat = directory.file("second-" + std::to_string(i) + (dwg ? ".dwg" : ".dxf"));
#ifdef DWGSUPPORT
            if (dwg) f::dwgFile(repeat, captured.splines.front(), DRW::AC1032);
            else f::binaryDxf(repeat, captured.splines.front(), DRW::AC1032);
#else
            REQUIRE_FALSE(dwg);
            f::binaryDxf(repeat, captured.splines.front(), DRW::AC1032);
#endif
            const auto second = f::readCodec(repeat, dwg);
            REQUIRE(second.splines.size() == 1);
            const auto repeated = xd::parseAndStrip(second.splines.front().extData, options);
            REQUIRE(repeated.status == xd::Status::Schema2);
            REQUIRE(repeated.definition);
            REQUIRE(repeated.definition->source);
            const auto repeatedSource = values::encodeSourceDefinition(*repeated.definition->source);
            REQUIRE(repeatedSource);
            CHECK(QCborValue(*before).toCbor() == QCborValue(*repeatedSource).toCbor());
        };

        for (const bool dwg : {false, true}) {
#ifndef DWGSUPPORT
            if (dwg) continue;
#endif
            xd::Options options;
            if (dwg) { options.eed = true; options.textEncoding = xd::TextEncoding::Utf16; }
            else options.asciiDxf = false;
            const auto tags = xd::forExport({app}, definition, options);
            REQUIRE(tags);
            auto entity = f::raw(1, *tags);
            const auto path = directory.file(std::to_string(i) + (dwg ? ".dwg" : ".dxf"));
#ifdef DWGSUPPORT
            if (dwg) f::dwgFile(path, entity, DRW::AC1032);
            else f::binaryDxf(path, entity, DRW::AC1032);
#else
            f::binaryDxf(path, entity, DRW::AC1032);
#endif
            verify(path, dwg, true);
        }
    }
}

TEST_CASE("typed source DXF import materializes only an operation-8 validated cache",
          "[lc-offsetcurve][import-session][typed-source][validation]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-typed-source-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{RS_LineData{{0, 0}, {4, 0}}}, .5, 1e-4, false};
    RS_Line source{nullptr, std::get<RS_LineData>(definition.source.value)};
    const auto options = LC_CurveOffset::makeDirectOptions(source, .5, definition.relativeTolerance);
    const auto generated = LC_CurveOffset::buildDirectBranches(source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, .5), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
    LC_CurveOffsetMaterializationResult materialized = LC_CurveOffset::materializeBranches(
        source, generated, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(materialized.entities.size() == 1);
    const auto* line = dynamic_cast<const RS_Line*>(materialized.entities.front().get());
    REQUIRE(line);

    for (const bool dwg : {false, true}) {
#ifndef DWGSUPPORT
        if (dwg) continue;
#endif
        xd::Definition xdata;
        xdata.source = definition;
        xd::Options encoding;
        if (dwg) { encoding.eed = true; encoding.textEncoding = xd::TextEncoding::Utf16; }
        else encoding.asciiDxf = false;
        const auto tags = xd::forExport({std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
            xdata, encoding);
        REQUIRE(tags);
        DRW_Spline cache = f::raw(1, *tags);
        cache.controllist.clear();
        cache.controllist.push_back(std::make_shared<DRW_Coord>(line->getStartpoint().x,
            line->getStartpoint().y, 0));
        cache.controllist.push_back(std::make_shared<DRW_Coord>(line->getEndpoint().x,
            line->getEndpoint().y, 0));
        cache.knotslist = {0, 0, 1, 1};
        cache.weightlist = {1, 1};
        cache.degree = 1;
        cache.flags = 8;
        cache.m_scenario = 1;
        cache.ncontrol = 2;
        cache.nknots = 4;
        cache.nfit = 0;

        const auto input = directory.file(dwg ? "validated.dwg" : "validated.dxf");
#ifdef DWGSUPPORT
        if (dwg) f::dwgFile(input, cache, DRW::AC1032);
        else f::binaryDxf(input, cache, DRW::AC1032);
#else
        f::binaryDxf(input, cache, DRW::AC1032);
#endif
        LC_OffsetImportSession session;
        std::optional<LC_OffsetImportSession::Outcome> outcome;
        const auto format = dwg ? RS2::FormatDWG2018 : RS2::FormatDXFRW2018;
        REQUIRE(session.start(QString::fromStdString(input.string()), format,
            RS2::DxfTextEncoding::Declared, false,
            [&](LC_OffsetImportSession::Outcome result) { outcome.emplace(std::move(result)); }));
        QElapsedTimer elapsed; elapsed.start();
        while (!outcome) {
            REQUIRE(elapsed.elapsed() < 35000);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QThread::msleep(1);
        }
        REQUIRE(outcome->status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome->graphic);
        CHECK(outcome->admitted == 1);
        CHECK(outcome->downgraded == 0);
        auto* imported = dynamic_cast<LC_OffsetCurve*>(f::sole(*outcome->graphic));
        REQUIRE(imported);
        REQUIRE(imported->sourceDefinition());
        const auto expected = values::encodeSourceDefinition(definition);
        const auto received = values::encodeSourceDefinition(*imported->sourceDefinition());
        REQUIRE(expected); REQUIRE(received);
        CHECK(QCborValue(*expected).toCbor() == QCborValue(*received).toCbor());
        CHECK(imported->getData().controlPoints.size() == 2);

        cache.controllist.front()->y += .125;
        const auto refusedInput = directory.file(dwg ? "modified-cache.dwg" : "modified-cache.dxf");
#ifdef DWGSUPPORT
        if (dwg) f::dwgFile(refusedInput, cache, DRW::AC1032);
        else f::binaryDxf(refusedInput, cache, DRW::AC1032);
#else
        f::binaryDxf(refusedInput, cache, DRW::AC1032);
#endif
        LC_OffsetImportSession refusedSession;
        std::optional<LC_OffsetImportSession::Outcome> refused;
        REQUIRE(refusedSession.start(QString::fromStdString(refusedInput.string()), format,
            RS2::DxfTextEncoding::Declared, false,
            [&](LC_OffsetImportSession::Outcome result) { refused.emplace(std::move(result)); }));
        elapsed.restart();
        while (!refused) {
            REQUIRE(elapsed.elapsed() < 35000);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QThread::msleep(1);
        }
        REQUIRE(refused->status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(refused->graphic);
        CHECK(refused->downgraded == 1);
        auto* ordinary = f::sole(*refused->graphic);
        CHECK(dynamic_cast<LC_OffsetCurve*>(ordinary) == nullptr);
        const auto preserved = xd::parseAndStrip(ordinary->getDrwExtData(), encoding);
        CHECK(preserved.status == xd::Status::Schema2);
        REQUIRE(preserved.definition);
        REQUIRE(preserved.definition->source);
    }
}

TEST_CASE("typed source families promote from real DXF and DWG cache carriers",
          "[lc-offsetcurve][import-session][typed-source][cache-promotion][persistence]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-conic-source-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;
    const double tau = 2.0 * M_PI;
    const std::array<LC_OffsetSourceGeometry, 7> sources{
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {4, 0}, .6, 0, tau, false}},
        LC_OffsetSourceGeometry{RS_EllipseData{{3, -2}, {3, 0}, .7, .2, 2.1, false}},
        LC_OffsetSourceGeometry{RS_ArcData{{-1, 2}, 2.5, .2, 2.4, true}},
        LC_OffsetSourceGeometry{RS_CircleData{{1, -1}, 2}},
        LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{-4, 4}, RS_Vector{0, -4}, RS_Vector{4, 4}}}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false}},
        LC_OffsetSourceGeometry{RS_LineData{{0, 0}, {2, 0}}}
    };
    struct Format {
        Lane lane;
        bool dwg;
        DRW::Version version;
        const char* suffix;
    };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (std::size_t sourceIndex = 0; sourceIndex < sources.size(); ++sourceIndex) {
        CAPTURE(sourceIndex, static_cast<int>(sources[sourceIndex].kind()));
        for (const double signedDistance : {-.4, -.1, .1, .4}) {
            CAPTURE(signedDistance);
            const LC_OffsetSourceDefinition definition{
                sources[sourceIndex], signedDistance, 1e-4, false};
            auto source = definition.source.makeEntity();
            REQUIRE(source);
            const double magnitude = std::abs(signedDistance);
            const auto options = LC_CurveOffset::makeDirectOptions(*source, magnitude,
                definition.relativeTolerance);
            const auto generated = LC_CurveOffset::buildDirectBranches(*source,
                LC_CurveOffset::makeSideRequest(
                    signedDistance < 0 ? LC_CurveOffsetSide::Right : LC_CurveOffsetSide::Left,
                    magnitude), options, LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
            auto materialized = LC_CurveOffset::materializeBranches(*source, generated,
                options, LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
            REQUIRE(materialized.entities.size() == 1);

            RS_SplineData expectedCache;
            if (const auto* spline = dynamic_cast<const RS_Spline*>(materialized.entities.front().get())) {
                expectedCache = spline->getData();
                REQUIRE(spline->validate());
            } else if (const auto* line = dynamic_cast<const RS_Line*>(materialized.entities.front().get())) {
                expectedCache.degree = 1;
                expectedCache.controlPoints = {line->getStartpoint(), line->getEndpoint()};
                expectedCache.knotslist = {0, 0, 1, 1};
                expectedCache.weights = {1, 1};
            } else {
                FAIL("conic offset cache must be a spline or exact line");
            }

            for (const auto& fileFormat : formats) {
                const auto [lane, dwg, version, formatSuffix] = fileFormat;
#ifndef DWGSUPPORT
                if (dwg) continue;
#endif
                xd::Definition xdata;
                xdata.source = definition;
                xd::Options encoding;
                if (dwg) {
                    encoding.eed = true;
                    encoding.textEncoding = xd::TextEncoding::Utf16;
                } else {
                    encoding.asciiDxf = false;
                }
                const auto tags = xd::forExport(
                    {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
                    xdata, encoding);
                REQUIRE(tags);
                DRW_Spline cache = f::raw(1, *tags);
                cache.controllist.clear();
                for (const auto& point : expectedCache.controlPoints)
                    cache.controllist.push_back(
                        std::make_shared<DRW_Coord>(point.x, point.y, 0));
                cache.knotslist = expectedCache.knotslist;
                cache.weightlist = expectedCache.weights;
                cache.degree = expectedCache.degree;
                cache.flags = 8;
                cache.m_scenario = 1;
                cache.ncontrol = cache.controllist.size();
                cache.nknots = cache.knotslist.size();
                cache.nfit = 0;

                const auto suffix = std::to_string(sourceIndex) +
                    (signedDistance < 0 ? "-right" : "-left") +
                    "-" + formatSuffix;
                const auto input = directory.file(suffix);
#ifdef DWGSUPPORT
                if (dwg) f::dwgFile(input, cache, version);
                else f::binaryDxf(input, cache, version);
#else
                f::binaryDxf(input, cache, version);
#endif
                const auto outcome = stagedImport(input, lane);
                REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
                REQUIRE(outcome.graphic);
                CHECK(outcome.admitted == 1);
                CHECK(outcome.downgraded == 0);
                const auto* imported = dynamic_cast<const LC_OffsetCurve*>(
                    f::sole(*outcome.graphic));
                REQUIRE(imported);
                REQUIRE(imported->sourceDefinition());
                const auto expectedSource = values::encodeSourceDefinition(definition);
                const auto importedSource = values::encodeSourceDefinition(
                    *imported->sourceDefinition());
                REQUIRE(expectedSource);
                REQUIRE(importedSource);
                CHECK(QCborValue(*expectedSource).toCbor() ==
                      QCborValue(*importedSource).toCbor());
                CHECK(RS_Spline::geometryFingerprint(imported->getData()) ==
                      RS_Spline::geometryFingerprint(expectedCache));
            }
        }
    }
}

TEST_CASE("control-point spline-points source promotes through DXF and DWG import",
          "[lc-offsetcurve][import-session][typed-source][control-points][persistence]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-control-points-source-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    LC_SplinePointsData controlPoints{false, false};
    controlPoints.useControlPoints = true;
    controlPoints.controlPoints = {{0, 0}, {1, 1}, {2, 0}};
    struct Format {
        Lane lane;
        bool dwg;
        DRW::Version version;
        const char* suffix;
    };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const double signedDistance : {-.4, -.1, .1, .4}) {
      CAPTURE(signedDistance);
      const LC_OffsetSourceDefinition definition{
          LC_OffsetSourceGeometry{controlPoints}, signedDistance, 1e-4, false};
      auto source = definition.source.makeEntity();
      REQUIRE(source);
      const double magnitude = std::abs(signedDistance);
      const auto options = LC_CurveOffset::makeDirectOptions(*source, magnitude,
          definition.relativeTolerance);
      const auto side = signedDistance < 0 ? LC_CurveOffsetSide::Right
                                           : LC_CurveOffsetSide::Left;
      const auto generated = LC_CurveOffset::buildDirectBranches(*source,
          LC_CurveOffset::makeSideRequest(side, magnitude), options,
          LC_CurveOffset::makeDirectSourceBudget());
      REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
      auto materialized = LC_CurveOffset::materializeBranches(*source, generated, options,
          LC_CurveOffset::makeDirectSourceBudget());
      REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
      REQUIRE(materialized.entities.size() == 1);
      const auto* expectedSpline = dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
      REQUIRE(expectedSpline);
      const RS_SplineData expectedCache = expectedSpline->getData();
      REQUIRE(expectedSpline->validate());

      for (const auto& fileFormat : formats) {
        const auto [lane, dwg, version, formatSuffix] = fileFormat;
#ifndef DWGSUPPORT
        if (dwg) continue;
#endif
        xd::Definition carrier;
        carrier.source = definition;
        xd::Options encoding;
        if (dwg) { encoding.eed = true; encoding.textEncoding = xd::TextEncoding::Utf16; }
        else encoding.asciiDxf = false;
        const auto tags = xd::forExport(
            {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))}, carrier, encoding);
        REQUIRE(tags);
        DRW_Spline cache = f::raw(1, *tags);
        cache.controllist.clear();
        for (const auto& point : expectedCache.controlPoints)
            cache.controllist.push_back(std::make_shared<DRW_Coord>(point.x, point.y, 0));
        cache.knotslist = expectedCache.knotslist;
        cache.weightlist = expectedCache.weights;
        cache.degree = expectedCache.degree;
        cache.flags = 8;
        cache.m_scenario = 1;
        cache.ncontrol = cache.controllist.size();
        cache.nknots = cache.knotslist.size();
        cache.nfit = 0;

        const auto input = directory.file(std::string("control-points-") +
            (signedDistance < 0 ? "right-" : "left-") +
            std::to_string(magnitude) + "-" + formatSuffix);
#ifdef DWGSUPPORT
        if (dwg) f::dwgFile(input, cache, version);
        else f::binaryDxf(input, cache, version);
#else
        f::binaryDxf(input, cache, version);
#endif
        const auto outcome = stagedImport(input, lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        CHECK(outcome.admitted == 1);
        CHECK(outcome.downgraded == 0);
        const auto* imported = dynamic_cast<const LC_OffsetCurve*>(f::sole(*outcome.graphic));
        REQUIRE(imported);
        REQUIRE(imported->sourceDefinition());
        const auto expectedSource = values::encodeSourceDefinition(definition);
        const auto actualSource = values::encodeSourceDefinition(*imported->sourceDefinition());
        REQUIRE(expectedSource);
        REQUIRE(actualSource);
        CHECK(QCborValue(*expectedSource).toCbor() == QCborValue(*actualSource).toCbor());
        const auto* received = std::get_if<LC_SplinePointsData>(
            &imported->sourceDefinition()->source.value);
        REQUIRE(received);
        CHECK(received->useControlPoints);
        CHECK(RS_Spline::geometryFingerprint(imported->getData()) ==
              RS_Spline::geometryFingerprint(expectedCache));
      }
    }
}

TEST_CASE("bulged LWPOLYLINE import preserves signed arc geometry for offset source capture",
          "[lc-offsetcurve][import-session][typed-source][polyline][bulge]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-bulged-polyline-source-import");
    const std::array<double, 4> bulges{
        std::tan(M_PI / 16.0), -std::tan(M_PI / 8.0), 1.0, -1.5};
    DRW_LWPolyline raw;
    raw.vertexnum = 6;
    raw.addVertex(DRW_Vertex2D(0, 0, bulges[0]));
    raw.addVertex(DRW_Vertex2D(2, 0, bulges[1]));
    raw.addVertex(DRW_Vertex2D(4, 0, bulges[2]));
    raw.addVertex(DRW_Vertex2D(6, 0, bulges[3]));
    raw.addVertex(DRW_Vertex2D(8, 0, 0));
    raw.addVertex(DRW_Vertex2D(10, 0, 0));

    RS_Polyline expected;
    expected.addVertex({0, 0}, bulges[0]);
    expected.addVertex({2, 0}, bulges[1]);
    expected.addVertex({4, 0}, bulges[2]);
    expected.addVertex({6, 0}, bulges[3]);
    expected.addVertex({8, 0});
    expected.addVertex({10, 0});
    const auto expectedSource = LC_OffsetSourceGeometry::capture(expected);
    REQUIRE(expectedSource);

    struct Format {
        Lane lane;
        bool dwg;
        DRW::Version version;
        const char* suffix;
    };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& format : formats) {
        const auto [lane, dwg, version, suffix] = format;
#ifndef DWGSUPPORT
        if (dwg) continue;
#endif
        const auto path = directory.file(std::string("signed-bulges-") + suffix);
#ifdef DWGSUPPORT
        if (dwg) f::dwgPolylineFile(path, raw, version);
        else f::binaryDxfPolyline(path, raw, version);
#else
        f::binaryDxfPolyline(path, raw, version);
#endif
        auto outcome = stagedImport(path, lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        auto* imported = dynamic_cast<RS_Polyline*>(f::sole(*outcome.graphic));
        REQUIRE(imported);
        const auto actualSource = LC_OffsetSourceGeometry::capture(*imported);
        REQUIRE(actualSource);
        const auto& expectedData = std::get<LC_OffsetPolylineData>(expectedSource->value);
        const auto& actualData = std::get<LC_OffsetPolylineData>(actualSource->value);
        REQUIRE(actualData.segments.size() == expectedData.segments.size());
        CHECK(actualData.closed == expectedData.closed);
        CHECK(actualData.startpoint.x == Catch::Approx(expectedData.startpoint.x).margin(1e-12));
        CHECK(actualData.startpoint.y == Catch::Approx(expectedData.startpoint.y).margin(1e-12));
        CHECK(actualData.endpoint.x == Catch::Approx(expectedData.endpoint.x).margin(1e-12));
        CHECK(actualData.endpoint.y == Catch::Approx(expectedData.endpoint.y).margin(1e-12));
        for (std::size_t i = 0; i < expectedData.segments.size(); ++i) {
            CAPTURE(dwg, version, i);
            REQUIRE(actualData.segments[i].index() == expectedData.segments[i].index());
            if (const auto* expectedArc = std::get_if<RS_ArcData>(&expectedData.segments[i])) {
                const auto& actualArc = std::get<RS_ArcData>(actualData.segments[i]);
                CHECK(actualArc.center.x == Catch::Approx(expectedArc->center.x).margin(1e-12));
                CHECK(actualArc.center.y == Catch::Approx(expectedArc->center.y).margin(1e-12));
                CHECK(actualArc.radius == Catch::Approx(expectedArc->radius).margin(1e-12));
                CHECK(actualArc.angle1 == Catch::Approx(expectedArc->angle1).margin(1e-12));
                CHECK(actualArc.angle2 == Catch::Approx(expectedArc->angle2).margin(1e-12));
                CHECK(actualArc.reversed == expectedArc->reversed);
            }
        }
    }
}

TEST_CASE("imported duplicate LWPOLYLINE vertices remain ordinary and are not offset sources",
          "[lc-offsetcurve][import-session][typed-source][polyline][malformed]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-duplicate-polyline-source-import");
    DRW_LWPolyline raw;
    raw.vertexnum = 4;
    raw.addVertex(DRW_Vertex2D(0, 0, 0));
    raw.addVertex(DRW_Vertex2D(2, 0, 0));
    raw.addVertex(DRW_Vertex2D(2, 0, 0));
    raw.addVertex(DRW_Vertex2D(4, 1, 0));

    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& format : formats) {
#ifndef DWGSUPPORT
        if (format.dwg) continue;
#endif
        const auto path = directory.file(std::string("duplicate-vertices-") + format.suffix);
#ifdef DWGSUPPORT
        if (format.dwg) f::dwgPolylineFile(path, raw, format.version);
        else f::binaryDxfPolyline(path, raw, format.version);
#else
        f::binaryDxfPolyline(path, raw, format.version);
#endif

        auto outcome = stagedImport(path, format.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        std::vector<const RS_Polyline*> polylines;
        for (const RS_Entity* entity : *outcome.graphic) {
            if (const auto* polyline = dynamic_cast<const RS_Polyline*>(entity))
                polylines.push_back(polyline);
        }
        REQUIRE(polylines.size() == 1);
        const RS_Polyline& imported = *polylines.front();
        const auto vertices = imported.getVertexes();
        REQUIRE(vertices.size() == 4);
        CHECK(vertices[1] == vertices[2]);
        REQUIRE(imported.size() == 3);
        CHECK(imported.entityAt(1)->getStartpoint() == imported.entityAt(1)->getEndpoint());
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(imported));
    }
}

TEST_CASE("imported non-consecutive repeated LWPOLYLINE vertices are not offset sources",
          "[lc-offsetcurve][import-session][typed-source][polyline][malformed][topology]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-nonconsecutive-repeat-polyline-import");
    DRW_LWPolyline raw;
    raw.vertexnum = 5;
    raw.addVertex(DRW_Vertex2D(0, 0, 0));
    raw.addVertex(DRW_Vertex2D(2, 0, 0));
    raw.addVertex(DRW_Vertex2D(2, 2, 0));
    raw.addVertex(DRW_Vertex2D(0, 0, 0));
    raw.addVertex(DRW_Vertex2D(0, 2, 0));

    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& format : formats) {
#ifndef DWGSUPPORT
        if (format.dwg) continue;
#endif
        const auto path = directory.file(std::string("repeated-vertex-") + format.suffix);
#ifdef DWGSUPPORT
        if (format.dwg) f::dwgPolylineFile(path, raw, format.version);
        else f::binaryDxfPolyline(path, raw, format.version);
#else
        f::binaryDxfPolyline(path, raw, format.version);
#endif

        auto outcome = stagedImport(path, format.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        std::vector<const RS_Polyline*> polylines;
        for (const RS_Entity* entity : *outcome.graphic) {
            if (const auto* polyline = dynamic_cast<const RS_Polyline*>(entity))
                polylines.push_back(polyline);
        }
        REQUIRE(polylines.size() == 1);
        const RS_Polyline& imported = *polylines.front();
        const auto vertices = imported.getVertexes();
        REQUIRE(vertices.size() == 5);
        CHECK(vertices[0] == vertices[3]);
        CHECK(imported.size() == 4);
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(imported));
    }
}

TEST_CASE("imported self-crossing LWPOLYLINE remains ordinary but is not an offset source",
          "[lc-offsetcurve][import-session][typed-source][polyline][topology]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-crossing-polyline-import");
    DRW_LWPolyline raw;
    raw.vertexnum = 4;
    raw.addVertex(DRW_Vertex2D(0, 0, 0));
    raw.addVertex(DRW_Vertex2D(2, 2, 0));
    raw.addVertex(DRW_Vertex2D(0, 2, 0));
    raw.addVertex(DRW_Vertex2D(2, 0, 0));

    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 3> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& format : formats) {
#ifndef DWGSUPPORT
        if (format.dwg) continue;
#endif
        const auto path = directory.file(std::string("crossing-") + format.suffix);
#ifdef DWGSUPPORT
        if (format.dwg) f::dwgPolylineFile(path, raw, format.version);
        else f::binaryDxfPolyline(path, raw, format.version);
#else
        f::binaryDxfPolyline(path, raw, format.version);
#endif
        auto outcome = stagedImport(path, format.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        std::vector<const RS_Polyline*> polylines;
        for (const RS_Entity* entity : *outcome.graphic) {
            if (const auto* polyline = dynamic_cast<const RS_Polyline*>(entity))
                polylines.push_back(polyline);
        }
        REQUIRE(polylines.size() == 1);
        CHECK(polylines.front()->size() == 3);
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*polylines.front()));
    }
}

TEST_CASE("imported closed self-crossing LWPOLYLINE is not an offset source",
          "[lc-offsetcurve][import-session][typed-source][polyline][topology][closed]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-closed-crossing-polyline-import");
    DRW_LWPolyline raw;
    raw.flags = 1;
    raw.vertexnum = 4;
    raw.addVertex(DRW_Vertex2D(0, 0, 0));
    raw.addVertex(DRW_Vertex2D(2, 2, 0));
    raw.addVertex(DRW_Vertex2D(0, 2, 0));
    raw.addVertex(DRW_Vertex2D(2, 0, 0));

    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 3> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& format : formats) {
#ifndef DWGSUPPORT
        if (format.dwg) continue;
#endif
        const auto path = directory.file(std::string("closed-crossing-") + format.suffix);
#ifdef DWGSUPPORT
        if (format.dwg) f::dwgPolylineFile(path, raw, format.version);
        else f::binaryDxfPolyline(path, raw, format.version);
#else
        f::binaryDxfPolyline(path, raw, format.version);
#endif
        auto outcome = stagedImport(path, format.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        const RS_Polyline* imported = nullptr;
        for (const RS_Entity* entity : *outcome.graphic) {
            if (const auto* polyline = dynamic_cast<const RS_Polyline*>(entity)) {
                REQUIRE(imported == nullptr);
                imported = polyline;
            }
        }
        REQUIRE(imported);
        CHECK(imported->isClosed());
        CHECK(imported->size() == 4);
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*imported));
    }
}

TEST_CASE("imported adjacent bulge arc and line crossing is not an offset source",
          "[lc-offsetcurve][import-session][typed-source][polyline][topology][bulge]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-crossing-bulge-polyline-import");
    DRW_LWPolyline raw;
    raw.vertexnum = 3;
    raw.addVertex(DRW_Vertex2D(1, 0, 1));
    raw.addVertex(DRW_Vertex2D(-1, 0, 0));
    raw.addVertex(DRW_Vertex2D(2, 0, 0));

    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 3> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& format : formats) {
#ifndef DWGSUPPORT
        if (format.dwg) continue;
#endif
        const auto path = directory.file(std::string("arc-line-crossing-") + format.suffix);
#ifdef DWGSUPPORT
        if (format.dwg) f::dwgPolylineFile(path, raw, format.version);
        else f::binaryDxfPolyline(path, raw, format.version);
#else
        f::binaryDxfPolyline(path, raw, format.version);
#endif
        auto outcome = stagedImport(path, format.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        const RS_Polyline* imported = nullptr;
        for (const RS_Entity* entity : *outcome.graphic) {
            if (const auto* polyline = dynamic_cast<const RS_Polyline*>(entity)) {
                REQUIRE(imported == nullptr);
                imported = polyline;
            }
        }
        REQUIRE(imported);
        REQUIRE(imported->size() == 2);
        CHECK(imported->entityAt(0)->rtti() == RS2::EntityArc);
        CHECK(imported->entityAt(1)->rtti() == RS2::EntityLine);
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*imported));
    }
}

TEST_CASE("imported adjacent bulge arcs crossing twice are not an offset source",
          "[lc-offsetcurve][import-session][typed-source][polyline][topology][bulge]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-crossing-bulge-arcs-import");
    DRW_LWPolyline raw;
    raw.vertexnum = 3;
    raw.addVertex(DRW_Vertex2D(1, 0, std::tan(-5.0 * M_PI / 12.0)));
    raw.addVertex(DRW_Vertex2D(0.5, std::sqrt(3.0) / 2.0,
                                std::tan(M_PI / 3.0)));
    raw.addVertex(DRW_Vertex2D(2, 0, 0));

    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 3> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& format : formats) {
#ifndef DWGSUPPORT
        if (format.dwg) continue;
#endif
        const auto path = directory.file(std::string("arc-arc-crossing-") + format.suffix);
#ifdef DWGSUPPORT
        if (format.dwg) f::dwgPolylineFile(path, raw, format.version);
        else f::binaryDxfPolyline(path, raw, format.version);
#else
        f::binaryDxfPolyline(path, raw, format.version);
#endif
        auto outcome = stagedImport(path, format.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        const RS_Polyline* imported = nullptr;
        for (const RS_Entity* entity : *outcome.graphic) {
            if (const auto* polyline = dynamic_cast<const RS_Polyline*>(entity)) {
                REQUIRE(imported == nullptr);
                imported = polyline;
            }
        }
        REQUIRE(imported);
        REQUIRE(imported->size() == 2);
        CHECK(imported->entityAt(0)->rtti() == RS2::EntityArc);
        CHECK(imported->entityAt(1)->rtti() == RS2::EntityArc);
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*imported));
    }
}

TEST_CASE("imported overlapping coincident bulge arcs are not an offset source",
          "[lc-offsetcurve][import-session][typed-source][polyline][topology][bulge]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-overlapping-coincident-bulge-arcs-import");
    DRW_LWPolyline raw;
    raw.vertexnum = 3;
    raw.addVertex(DRW_Vertex2D(1, 0, std::tan(M_PI / 6.0)));
    raw.addVertex(DRW_Vertex2D(-0.5, std::sqrt(3.0) / 2.0,
                                std::tan(5.0 * M_PI / 12.0)));
    raw.addVertex(DRW_Vertex2D(0.5, std::sqrt(3.0) / 2.0, 0));

    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 3> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& format : formats) {
#ifndef DWGSUPPORT
        if (format.dwg) continue;
#endif
        const auto path = directory.file(std::string("overlapping-arcs-") + format.suffix);
#ifdef DWGSUPPORT
        if (format.dwg) f::dwgPolylineFile(path, raw, format.version);
        else f::binaryDxfPolyline(path, raw, format.version);
#else
        f::binaryDxfPolyline(path, raw, format.version);
#endif
        auto outcome = stagedImport(path, format.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        const RS_Polyline* imported = nullptr;
        for (const RS_Entity* entity : *outcome.graphic) {
            if (const auto* polyline = dynamic_cast<const RS_Polyline*>(entity)) {
                REQUIRE(imported == nullptr);
                imported = polyline;
            }
        }
        REQUIRE(imported);
        REQUIRE(imported->size() == 2);
        CHECK(imported->entityAt(0)->rtti() == RS2::EntityArc);
        CHECK(imported->entityAt(1)->rtti() == RS2::EntityArc);
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*imported));
    }
}

TEST_CASE("imported near-concentric bulge arcs crossing twice are not an offset source",
          "[lc-offsetcurve][import-session][typed-source][polyline][topology][bulge]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-near-concentric-bulge-arcs-import");
    constexpr double centerShift = 1.0e-8;
    const double joinAngle = 3.0 * M_PI / 2.0 - std::atan(centerShift);
    const double radius = std::hypot(centerShift, 1.0);
    DRW_LWPolyline raw;
    raw.vertexnum = 3;
    raw.addVertex(DRW_Vertex2D(1, 0, std::tan(3.0 * M_PI / 8.0)));
    raw.addVertex(DRW_Vertex2D(0, -1, -std::tan(joinAngle / 4.0)));
    raw.addVertex(DRW_Vertex2D(centerShift + radius, 0, 0));

    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& format : formats) {
#ifndef DWGSUPPORT
        if (format.dwg) continue;
#endif
        const auto path = directory.file(std::string("near-concentric-arcs-") + format.suffix);
#ifdef DWGSUPPORT
        if (format.dwg) f::dwgPolylineFile(path, raw, format.version);
        else f::binaryDxfPolyline(path, raw, format.version);
#else
        f::binaryDxfPolyline(path, raw, format.version);
#endif
        auto outcome = stagedImport(path, format.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        const RS_Polyline* imported = nullptr;
        for (const RS_Entity* entity : *outcome.graphic) {
            if (const auto* polyline = dynamic_cast<const RS_Polyline*>(entity)) {
                REQUIRE(imported == nullptr);
                imported = polyline;
            }
        }
        REQUIRE(imported);
        REQUIRE(imported->size() == 2);
        CHECK(imported->entityAt(0)->rtti() == RS2::EntityArc);
        CHECK(imported->entityAt(1)->rtti() == RS2::EntityArc);
        const auto* firstArc = dynamic_cast<const RS_Arc*>(imported->entityAt(0));
        const auto* secondArc = dynamic_cast<const RS_Arc*>(imported->entityAt(1));
        REQUIRE(firstArc);
        REQUIRE(secondArc);
        CHECK(firstArc->isPointOnEntity({0, 1}, 1.0e-9));
        CHECK(secondArc->isPointOnEntity({0, 1}, 1.0e-9));
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*imported));
    }
}

TEST_CASE("imported near-tangent line and arc crossing is not an offset source",
          "[lc-offsetcurve][import-session][typed-source][polyline][topology][bulge]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-near-tangent-line-arc-import");
    constexpr double radialOffset = 1.0e-10;
    constexpr double lineHalfLength = 1.0e-6;
    const double y = 1.0 - radialOffset;
    const double x = std::sqrt(1.0 - y * y);
    const double joinAngle = std::atan2(y, x);
    const double arcStart = joinAngle - 1.0e-6;
    const double arcEnd = joinAngle + 1.0e-3;
    const double arcStartX = std::cos(arcStart);
    const double arcStartY = std::sin(arcStart);
    const double arcEndX = std::cos(arcEnd);
    const double arcEndY = std::sin(arcEnd);
    DRW_LWPolyline raw;
    raw.vertexnum = 4;
    raw.addVertex(DRW_Vertex2D(x - lineHalfLength, y, 0.0));
    raw.addVertex(DRW_Vertex2D(x + lineHalfLength, y, 0.0));
    raw.addVertex(DRW_Vertex2D(arcStartX, arcStartY,
                               std::tan((arcEnd - arcStart) / 4.0)));
    raw.addVertex(DRW_Vertex2D(arcEndX, arcEndY, 0.0));

    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& format : formats) {
#ifndef DWGSUPPORT
        if (format.dwg) continue;
#endif
        const auto path = directory.file(std::string("near-tangent-line-arc-") + format.suffix);
#ifdef DWGSUPPORT
        if (format.dwg) f::dwgPolylineFile(path, raw, format.version);
        else f::binaryDxfPolyline(path, raw, format.version);
#else
        f::binaryDxfPolyline(path, raw, format.version);
#endif
        auto outcome = stagedImport(path, format.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        const RS_Polyline* imported = nullptr;
        for (const RS_Entity* entity : *outcome.graphic) {
            if (const auto* polyline = dynamic_cast<const RS_Polyline*>(entity)) {
                REQUIRE(imported == nullptr);
                imported = polyline;
            }
        }
        REQUIRE(imported);
        REQUIRE(imported->size() == 3);
        CHECK(imported->entityAt(0)->rtti() == RS2::EntityLine);
        CHECK(imported->entityAt(1)->rtti() == RS2::EntityLine);
        CHECK(imported->entityAt(2)->rtti() == RS2::EntityArc);
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*imported));
    }
}

TEST_CASE("imported near-parallel line crossing is not an offset source",
          "[lc-offsetcurve][import-session][typed-source][polyline][topology]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-near-parallel-line-crossing-import");
    constexpr double offset = 1.0e-10;
    DRW_LWPolyline raw;
    raw.vertexnum = 4;
    raw.addVertex(DRW_Vertex2D(0.0, 0.0, 0.0));
    raw.addVertex(DRW_Vertex2D(3.0, 0.0, 0.0));
    raw.addVertex(DRW_Vertex2D(2.0, offset, 0.0));
    raw.addVertex(DRW_Vertex2D(1.0, -offset, 0.0));

    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& format : formats) {
#ifndef DWGSUPPORT
        if (format.dwg) continue;
#endif
        const auto path = directory.file(std::string("near-parallel-lines-") + format.suffix);
#ifdef DWGSUPPORT
        if (format.dwg) f::dwgPolylineFile(path, raw, format.version);
        else f::binaryDxfPolyline(path, raw, format.version);
#else
        f::binaryDxfPolyline(path, raw, format.version);
#endif
        auto outcome = stagedImport(path, format.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        const RS_Polyline* imported = nullptr;
        for (const RS_Entity* entity : *outcome.graphic) {
            if (const auto* polyline = dynamic_cast<const RS_Polyline*>(entity)) {
                REQUIRE(imported == nullptr);
                imported = polyline;
            }
        }
        REQUIRE(imported);
        REQUIRE(imported->size() == 3);
        CHECK(imported->entityAt(0)->rtti() == RS2::EntityLine);
        CHECK(imported->entityAt(1)->rtti() == RS2::EntityLine);
        CHECK(imported->entityAt(2)->rtti() == RS2::EntityLine);
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*imported));
    }
}

TEST_CASE("imported repeated and near-coincident polyline vertices are refused as offset sources",
          "[lc-offsetcurve][import-session][typed-source][polyline][malformed][topology]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-polyline-repeated-vertex-topology-import");
    struct Shape { DRW_LWPolyline polyline; bool closed; const char* name; };
    std::array<Shape, 4> shapes{};
    shapes[0].closed = false;
    shapes[0].name = "open-return";
    shapes[0].polyline.vertexnum = 3;
    shapes[0].polyline.addVertex(DRW_Vertex2D(0, 0, 0));
    shapes[0].polyline.addVertex(DRW_Vertex2D(2, 0, 0));
    shapes[0].polyline.addVertex(DRW_Vertex2D(0, 0, 0));
    shapes[1].closed = true;
    shapes[1].name = "closed-interior-repeat";
    shapes[1].polyline.flags = 1;
    shapes[1].polyline.vertexnum = 5;
    shapes[1].polyline.addVertex(DRW_Vertex2D(0, 0, 0));
    shapes[1].polyline.addVertex(DRW_Vertex2D(2, 0, 0));
    shapes[1].polyline.addVertex(DRW_Vertex2D(2, 2, 0));
    shapes[1].polyline.addVertex(DRW_Vertex2D(0, 0, 0));
    shapes[1].polyline.addVertex(DRW_Vertex2D(0, 2, 0));
    shapes[2].closed = false;
    shapes[2].name = "near-coincident-nonconsecutive";
    shapes[2].polyline.vertexnum = 5;
    shapes[2].polyline.addVertex(DRW_Vertex2D(0, 0, 0));
    shapes[2].polyline.addVertex(DRW_Vertex2D(2, 0, 0));
    shapes[2].polyline.addVertex(DRW_Vertex2D(2, 2, 0));
    shapes[2].polyline.addVertex(DRW_Vertex2D(1.0e-15, 1.0e-15, 0));
    shapes[2].polyline.addVertex(DRW_Vertex2D(0, 2, 0));
    shapes[3].closed = false;
    shapes[3].name = "near-coincident-exponent-boundary";
    shapes[3].polyline.vertexnum = 5;
    shapes[3].polyline.addVertex(DRW_Vertex2D(2, 2, 0));
    shapes[3].polyline.addVertex(DRW_Vertex2D(4, 2, 0));
    shapes[3].polyline.addVertex(DRW_Vertex2D(4, 4, 0));
    shapes[3].polyline.addVertex(DRW_Vertex2D(std::nextafter(2.0, 0.0), 2, 0));
    shapes[3].polyline.addVertex(DRW_Vertex2D(2, 4, 0));

    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 3> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& shape : shapes) {
        for (const auto& format : formats) {
#ifndef DWGSUPPORT
            if (format.dwg) continue;
#endif
            const auto path = directory.file(std::string(shape.name) + "-" + format.suffix);
#ifdef DWGSUPPORT
            if (format.dwg) f::dwgPolylineFile(path, shape.polyline, format.version);
            else f::binaryDxfPolyline(path, shape.polyline, format.version);
#else
            f::binaryDxfPolyline(path, shape.polyline, format.version);
#endif

            auto outcome = stagedImport(path, format.lane);
            REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
            REQUIRE(outcome.graphic);
            std::vector<const RS_Polyline*> polylines;
            for (const RS_Entity* entity : *outcome.graphic) {
                if (const auto* polyline = dynamic_cast<const RS_Polyline*>(entity))
                    polylines.push_back(polyline);
            }
            REQUIRE(polylines.size() == 1);
            const RS_Polyline& imported = *polylines.front();
            CHECK(imported.isClosed() == shape.closed);
            const auto vertices = imported.getVertexes();
            CHECK(vertices.size() == shape.polyline.vertlist.size() + (shape.closed ? 1U : 0U));
            if (shape.name == std::string("near-coincident-nonconsecutive"))
                CHECK(vertices[0].distanceTo(vertices[3]) < 1.0e-14);
            if (shape.name == std::string("near-coincident-exponent-boundary"))
                CHECK(vertices[0].distanceTo(vertices[3]) < 1.0e-14);
            CHECK_FALSE(LC_OffsetSourceGeometry::capture(imported));
        }
    }
}

TEST_CASE("legacy 2D POLYLINE import preserves signed bulge source geometry",
          "[lc-offsetcurve][import-session][typed-source][polyline][legacy]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-legacy-polyline-source-import");
    const double positiveBulge = std::tan(M_PI / 16.0);
    const double negativeBulge = -std::tan(M_PI / 8.0);
    DRW_Polyline raw;
    raw.addVertex(DRW_Vertex(0, 0, 0, positiveBulge));
    raw.addVertex(DRW_Vertex(2, 0, 0, negativeBulge));
    raw.addVertex(DRW_Vertex(4, 0, 0, 0));
    raw.addVertex(DRW_Vertex(6, 1, 0, 0));

    RS_Polyline expected;
    expected.addVertex({0, 0}, positiveBulge);
    expected.addVertex({2, 0}, negativeBulge);
    expected.addVertex({4, 0});
    expected.addVertex({6, 1});
    const auto expectedSource = LC_OffsetSourceGeometry::capture(expected);
    REQUIRE(expectedSource);
    const auto& expectedData = std::get<LC_OffsetPolylineData>(expectedSource->value);

    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 4> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& format : formats) {
#ifndef DWGSUPPORT
        if (format.dwg) continue;
#endif
        const auto path = directory.file(std::string("legacy-polyline-") + format.suffix);
#ifdef DWGSUPPORT
        if (format.dwg) f::dwg2dPolylineFile(path, raw, format.version);
        else f::binaryDxf2dPolyline(path, raw, format.version);
#else
        f::binaryDxf2dPolyline(path, raw, format.version);
#endif

        auto outcome = stagedImport(path, format.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        std::vector<const RS_Polyline*> polylines;
        for (const RS_Entity* entity : *outcome.graphic) {
            if (const auto* polyline = dynamic_cast<const RS_Polyline*>(entity))
                polylines.push_back(polyline);
        }
        REQUIRE(polylines.size() == 1);
        const auto actualSource = LC_OffsetSourceGeometry::capture(*polylines.front());
        REQUIRE(actualSource);
        REQUIRE(actualSource->kind() == LC_OffsetSourceGeometry::Kind::Polyline);
        const auto& actualData = std::get<LC_OffsetPolylineData>(actualSource->value);
        CHECK(actualData.closed == expectedData.closed);
        CHECK(actualData.startpoint.distanceTo(expectedData.startpoint) < 1.0e-12);
        CHECK(actualData.endpoint.distanceTo(expectedData.endpoint) < 1.0e-12);
        REQUIRE(actualData.segments.size() == expectedData.segments.size());
        for (std::size_t i = 0; i < expectedData.segments.size(); ++i) {
            REQUIRE(actualData.segments[i].index() == expectedData.segments[i].index());
            if (std::holds_alternative<RS_LineData>(expectedData.segments[i])) {
                const auto& actual = std::get<RS_LineData>(actualData.segments[i]);
                const auto& wanted = std::get<RS_LineData>(expectedData.segments[i]);
                CHECK(actual.startpoint.distanceTo(wanted.startpoint) < 1.0e-12);
                CHECK(actual.endpoint.distanceTo(wanted.endpoint) < 1.0e-12);
            } else {
                const auto& actual = std::get<RS_ArcData>(actualData.segments[i]);
                const auto& wanted = std::get<RS_ArcData>(expectedData.segments[i]);
                CHECK(actual.center.distanceTo(wanted.center) < 1.0e-12);
                CHECK(actual.radius == Catch::Approx(wanted.radius).margin(1.0e-12));
                CHECK(actual.angle1 == Catch::Approx(wanted.angle1).margin(1.0e-12));
                CHECK(actual.angle2 == Catch::Approx(wanted.angle2).margin(1.0e-12));
                CHECK(actual.reversed == wanted.reversed);
            }
        }
    }
}

TEST_CASE("legacy 3D POLYLINE projections remain visible but are not offset sources",
          "[lc-offsetcurve][import-session][typed-source][polyline][legacy][3d]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-legacy-3d-polyline-provenance");
    DRW_Polyline raw;
    raw.flags = 8;
    raw.addVertex(DRW_Vertex(0, 0, 5, 0));
    raw.addVertex(DRW_Vertex(2, 0, 7, 0));
    raw.addVertex(DRW_Vertex(3, 2, 9, 0));
    raw.extData.push_back(std::make_shared<DRW_Variant>(1001, "LibreCad"));
    raw.extData.push_back(
        std::make_shared<DRW_Variant>(1000, "input-before-projection"));

    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 4> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};

    std::unique_ptr<RS_Graphic> roundTripSource;
    const auto hasProjectionMarker = [](const RS_Polyline& polyline) {
        return std::any_of(polyline.getDrwExtData().begin(),
                           polyline.getDrwExtData().end(), [](const auto& value) {
            return value && value->code() == 1001 &&
                   value->type() == DRW_Variant::STRING &&
                   std::string{value->c_str()} == "LibreCAD_POLYLINE_3D";
        });
    };
    const auto hasInputXData = [](const RS_Polyline& polyline) {
        const auto& values = polyline.getDrwExtData();
        bool app = false;
        bool payload = false;
        for (const auto& value : values) {
            if (!value || value->type() != DRW_Variant::STRING) continue;
            if (value->code() == 1001 && std::string{value->c_str()} == "LibreCad")
                app = true;
            if (value->code() == 1000 &&
                std::string{value->c_str()} == "input-before-projection")
                payload = true;
        }
        return app && payload;
    };

    for (const auto& input : formats) {
#ifndef DWGSUPPORT
        if (input.dwg) continue;
#endif
        const auto path = directory.file(std::string("legacy-3d-") + input.suffix);
#ifdef DWGSUPPORT
        if (input.dwg) f::dwg2dPolylineFile(path, raw, input.version);
        else f::binaryDxf2dPolyline(path, raw, input.version);
#else
        f::binaryDxf2dPolyline(path, raw, input.version);
#endif
        auto outcome = stagedImport(path, input.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        std::vector<RS_Polyline*> polylines;
        for (RS_Entity* entity : *outcome.graphic) {
            if (auto* polyline = dynamic_cast<RS_Polyline*>(entity))
                polylines.push_back(polyline);
        }
        REQUIRE(polylines.size() == 1);
        CHECK(polylines.front()->getVertexes().size() == 3);
        CHECK(polylines.front()->getVertexes()[0].z == 0.0);
        CHECK(hasProjectionMarker(*polylines.front()));
        CHECK(hasInputXData(*polylines.front()));
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*polylines.front()));
        if (input.lane == Lane::Binary18)
            roundTripSource = std::move(outcome.graphic);
    }

    REQUIRE(roundTripSource);
    const auto dxf = directory.file("roundtrip.dxf");
    REQUIRE(lc::test::exportDrawing(*roundTripSource, dxf, RS2::FormatDXFRW2018));
    const auto appRecords = lc::test::dxfRecords(dxf, "APPID");
    CHECK(std::any_of(appRecords.begin(), appRecords.end(), [](const auto& record) {
        return record.own(2) == std::vector<std::string>{"LibreCAD_POLYLINE_3D"};
    }));
    const auto dxfRecords = lc::test::dxfRecords(dxf);
    CHECK(std::any_of(dxfRecords.begin(), dxfRecords.end(), [](const auto& record) {
        const auto markers = record.own(1001);
        return std::find(markers.begin(), markers.end(),
                         "LibreCAD_POLYLINE_3D") != markers.end();
    }));

    RS_Graphic reopenedDxf;
    REQUIRE(lc::test::importDrawing(reopenedDxf, dxf, RS2::FormatDXFRW2018));
    std::size_t visibleDxfPolylines = 0;
    for (RS_Entity* entity : reopenedDxf) {
        if (auto* polyline = dynamic_cast<RS_Polyline*>(entity)) {
            ++visibleDxfPolylines;
            CHECK(hasProjectionMarker(*polyline));
            CHECK_FALSE(LC_OffsetSourceGeometry::capture(*polyline));
        }
    }
    CHECK(visibleDxfPolylines == 1);

#ifdef DWGSUPPORT
    const RS_Polyline* importedProjection = nullptr;
    for (const RS_Entity* entity : *roundTripSource) {
        if (const auto* polyline = dynamic_cast<const RS_Polyline*>(entity))
            importedProjection = polyline;
    }
    REQUIRE(importedProjection);
    REQUIRE(hasProjectionMarker(*importedProjection));
    RS_Graphic cleanDwg;
    cleanDwg.initForNewDocument();
    cleanDwg.addEntity(new RS_Line(&cleanDwg, RS_LineData({0.0, 0.0}, {1.0, 1.0})));
    auto dwgPolyline = std::make_unique<RS_Polyline>(
        &cleanDwg, RS_PolylineData{RS_Vector{}, RS_Vector{}, false});
    dwgPolyline->addVertex({0.0, 0.0});
    dwgPolyline->addVertex({2.0, 0.0});
    dwgPolyline->setDrwExtData(importedProjection->getDrwExtData());
    cleanDwg.addEntity(dwgPolyline.release());
    const auto dwg = directory.file("roundtrip.dwg");
    RS_FilterDXFRW dwgWriter;
    const bool dwgExported = dwgWriter.fileExport(
        cleanDwg, lc::test::toQString(dwg), RS2::FormatDWG2018);
    INFO(dwgWriter.dwgExportAdmissionReport().toStdString());
    INFO(dwgWriter.exportReport().toStdString());
    REQUIRE(dwgExported);
    RS_Graphic reopenedDwg;
    REQUIRE(lc::test::importDrawing(reopenedDwg, dwg, RS2::FormatDWG2018));
    CHECK(reopenedDwg.dwgAdvancedMetadata().findAppIdTableEntryByName(
        "LibreCAD_POLYLINE_3D") != nullptr);
    std::size_t visibleDwgPolylines = 0;
    for (RS_Entity* entity : reopenedDwg) {
        if (auto* polyline = dynamic_cast<RS_Polyline*>(entity)) {
            ++visibleDwgPolylines;
            CHECK(hasProjectionMarker(*polyline));
            CHECK_FALSE(LC_OffsetSourceGeometry::capture(*polyline));
        }
    }
    CHECK(visibleDwgPolylines == 1);
#endif
}

TEST_CASE("non-axial LWPOLYLINEs remain visible as marked XY projections",
          "[lc-offsetcurve][import-session][typed-source][polyline][ocs]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-non-axial-lwpolyline-projection");
    DRW_LWPolyline raw;
    raw.vertexnum = 3;
    raw.extPoint = {0.6, 0.0, 0.8};
    raw.elevation = 3.0;
    raw.addVertex(DRW_Vertex2D(0.0, 0.0, 0.0));
    raw.addVertex(DRW_Vertex2D(2.0, 0.0, 0.0));
    raw.addVertex(DRW_Vertex2D(2.0, 2.0, 0.0));

    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 3> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};

    std::unique_ptr<RS_Graphic> sourceDrawing;
    for (const auto& format : formats) {
#ifndef DWGSUPPORT
        if (format.dwg) continue;
#endif
        const auto input = directory.file(format.suffix);
#ifdef DWGSUPPORT
        if (format.dwg) f::dwgPolylineFile(input, raw, format.version);
        else f::binaryDxfPolyline(input, raw, format.version);
#else
        f::binaryDxfPolyline(input, raw, format.version);
#endif
        auto outcome = stagedImport(input, format.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        auto* projected = dynamic_cast<RS_Polyline*>(f::sole(*outcome.graphic));
        REQUIRE(projected);
        const auto points = projected->getVertexes();
        REQUIRE(points.size() == 3);
        CHECK(points[0].x == Catch::Approx(1.8));
        CHECK(points[0].y == Catch::Approx(0.0));
        CHECK(points[1].x == Catch::Approx(1.8));
        CHECK(points[1].y == Catch::Approx(2.0));
        CHECK(points[2].x == Catch::Approx(0.2));
        CHECK(points[2].y == Catch::Approx(2.0));
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*projected));

        const auto& ext = projected->getDrwExtData();
        const auto app = std::find_if(ext.begin(), ext.end(), [](const auto& value) {
            return value && value->code() == 1001 &&
                   value->type() == DRW_Variant::STRING &&
                   std::string{value->c_str()} == "LibreCAD_POLYLINE_OCS";
        });
        REQUIRE(app != ext.end());
        const auto normal = std::find_if(std::next(app), ext.end(), [](const auto& value) {
            return value && value->code() == 1010 &&
                   value->type() == DRW_Variant::COORD;
        });
        REQUIRE(normal != ext.end());
        REQUIRE((*normal)->coord() != nullptr);
        CHECK((*normal)->coord()->x == Catch::Approx(0.6));
        CHECK((*normal)->coord()->y == Catch::Approx(0.0));
        CHECK((*normal)->coord()->z == Catch::Approx(0.8));
        if (!format.dwg) sourceDrawing = std::move(outcome.graphic);
    }

    REQUIRE(sourceDrawing);
    const auto *straightSource =
        dynamic_cast<const RS_Polyline *>(f::sole(*sourceDrawing));
    REQUIRE(straightSource);
    const auto straightPoints = straightSource->getVertexes();
    REQUIRE(straightPoints.size() == 3);

    DRW_LWPolyline bulged = raw;
    bulged.vertlist.front()->bulge = std::tan(M_PI / 8.0);
    bulged.extData.push_back(std::make_shared<DRW_Variant>(
        1001, std::string("FixtureSourceData")));
    bulged.extData.push_back(std::make_shared<DRW_Variant>(
        1000, std::string("preserve-this-xdata")));
    const auto unsupportedArc = directory.file("non-axial-bulge.dxf");
    f::binaryDxfPolyline(unsupportedArc, bulged, DRW::AC1032);
    const auto unsupportedOutcome = stagedImport(unsupportedArc, Lane::Binary18);
    REQUIRE(unsupportedOutcome.status == LC_OffsetImportSession::Status::Complete);
    REQUIRE(unsupportedOutcome.graphic);
    REQUIRE(std::distance(unsupportedOutcome.graphic->begin(),
                          unsupportedOutcome.graphic->end()) == 2);
    const RS_Ellipse *projectedArc = nullptr;
    const RS_Line *projectedLine = nullptr;
    for (const RS_Entity *entity : *unsupportedOutcome.graphic) {
      if (const auto *ellipse = dynamic_cast<const RS_Ellipse *>(entity))
        projectedArc = ellipse;
      if (const auto *line = dynamic_cast<const RS_Line *>(entity))
        projectedLine = line;
    }
    REQUIRE(projectedArc);
    REQUIRE(projectedLine);
    CHECK(std::isfinite(projectedArc->getCenter().x));
    CHECK(std::isfinite(projectedArc->getCenter().y));
    CHECK(projectedArc->getMajorRadius() == Catch::Approx(std::sqrt(2.0)));
    CHECK(projectedArc->getRatio() == Catch::Approx(0.8));
    const auto samePoint = [](const RS_Vector &lhs, const RS_Vector &rhs) {
      return std::hypot(lhs.x - rhs.x, lhs.y - rhs.y) < 1e-8;
    };
    CHECK(samePoint(projectedArc->getStartpoint(), RS_Vector{1.8, 0.0}));
    CHECK(samePoint(projectedArc->getEndpoint(), RS_Vector{1.8, 2.0}));
    CHECK(samePoint(projectedLine->getStartpoint(), RS_Vector{1.8, 2.0}));
    CHECK(samePoint(projectedLine->getEndpoint(), RS_Vector{0.2, 2.0}));
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(*projectedArc));
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(*projectedLine));
    const auto hasFragmentXData = [](const RS_Entity &entity) {
      const auto &ext = entity.getDrwExtData();
      const auto app = std::find_if(ext.begin(), ext.end(), [](const auto &value) {
        return value && value->code() == 1001 &&
               value->type() == DRW_Variant::STRING &&
               std::string{value->c_str()} == "FixtureSourceData";
      });
      return app != ext.end() && std::next(app) != ext.end() &&
             (*std::next(app))->code() == 1000 &&
             (*std::next(app))->type() == DRW_Variant::STRING &&
             std::string{(*std::next(app))->c_str()} == "preserve-this-xdata";
    };
    const auto hasProjectionApp = [](const RS_Entity &entity,
                                     const std::string &appName) {
      const auto &ext = entity.getDrwExtData();
      return std::any_of(ext.begin(), ext.end(), [&appName](const auto &value) {
        return value && value->code() == 1001 &&
               value->type() == DRW_Variant::STRING &&
               std::string{value->c_str()} == appName;
      });
    };
    CHECK(hasFragmentXData(*projectedArc));
    CHECK(hasFragmentXData(*projectedLine));

    const auto bulgedDwgInput = directory.file("bulged-input.dwg");
    f::dwgPolylineFile(bulgedDwgInput, bulged, DRW::AC1032);
    auto importedBulgedDwg = stagedImport(bulgedDwgInput, Lane::Eed18);
    REQUIRE(importedBulgedDwg.status == LC_OffsetImportSession::Status::Complete);
    REQUIRE(importedBulgedDwg.graphic);
    REQUIRE(std::distance(importedBulgedDwg.graphic->begin(),
                          importedBulgedDwg.graphic->end()) == 2);
    for (const RS_Entity *entity : *importedBulgedDwg.graphic) {
      CHECK(hasFragmentXData(*entity));
      CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
    }
    const auto checkDwgOriginProjection = [&](const RS_Graphic &drawing) {
      REQUIRE(std::distance(drawing.begin(), drawing.end()) == 2);
      const RS_Ellipse *ellipse = nullptr;
      const RS_Line *line = nullptr;
      for (const RS_Entity *entity : drawing) {
        CHECK(hasFragmentXData(*entity));
        CHECK(hasProjectionApp(*entity, "LibreCAD_POLYLINE_OCS"));
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
        if (const auto *candidate = dynamic_cast<const RS_Ellipse *>(entity)) {
          ellipse = candidate;
          CHECK(hasProjectionApp(*entity, "LibreCAD_CONIC_OCS"));
        }
        if (const auto *candidate = dynamic_cast<const RS_Line *>(entity))
          line = candidate;
      }
      REQUIRE(ellipse);
      REQUIRE(line);
      CHECK(ellipse->getMajorRadius() == Catch::Approx(std::sqrt(2.0)));
      CHECK(ellipse->getRatio() == Catch::Approx(0.8));
      CHECK(samePoint(ellipse->getStartpoint(), RS_Vector{1.8, 0.0}));
      CHECK(samePoint(ellipse->getEndpoint(), RS_Vector{1.8, 2.0}));
      CHECK(samePoint(line->getStartpoint(), RS_Vector{1.8, 2.0}));
      CHECK(samePoint(line->getEndpoint(), RS_Vector{0.2, 2.0}));
    };

    struct BulgedFormat {
      Lane lane;
      bool dwg;
      DRW::Version version;
      const char *suffix;
    };
    const std::array<BulgedFormat, 5> bulgedFormats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};
    for (const auto &format : bulgedFormats) {
#ifndef DWGSUPPORT
      if (format.dwg)
        continue;
#endif
      const auto path = directory.file(std::string("bulged-") + format.suffix);
#ifdef DWGSUPPORT
      if (format.dwg)
        f::dwgPolylineFile(path, bulged, format.version);
      else
        f::binaryDxfPolyline(path, bulged, format.version);
#else
      f::binaryDxfPolyline(path, bulged, format.version);
#endif
      const auto imported = stagedImport(path, format.lane);
      REQUIRE(imported.status == LC_OffsetImportSession::Status::Complete);
      REQUIRE(imported.graphic);
      REQUIRE(std::distance(imported.graphic->begin(), imported.graphic->end()) == 2);
      const RS_Ellipse *ellipse = nullptr;
      const RS_Line *line = nullptr;
      for (const RS_Entity *entity : *imported.graphic) {
        CHECK(hasFragmentXData(*entity));
        CHECK(hasProjectionApp(*entity, "LibreCAD_POLYLINE_OCS"));
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
        if (const auto *candidate = dynamic_cast<const RS_Ellipse *>(entity)) {
          ellipse = candidate;
          CHECK(hasProjectionApp(*entity, "LibreCAD_CONIC_OCS"));
        }
        if (const auto *candidate = dynamic_cast<const RS_Line *>(entity))
          line = candidate;
      }
      REQUIRE(ellipse);
      REQUIRE(line);
      CHECK(ellipse->getMajorRadius() == Catch::Approx(std::sqrt(2.0)));
      CHECK(ellipse->getRatio() == Catch::Approx(0.8));
      CHECK(samePoint(ellipse->getStartpoint(), RS_Vector{1.8, 0.0}));
      CHECK(samePoint(ellipse->getEndpoint(), RS_Vector{1.8, 2.0}));
      CHECK(samePoint(line->getStartpoint(), RS_Vector{1.8, 2.0}));
      CHECK(samePoint(line->getEndpoint(), RS_Vector{0.2, 2.0}));
    }

    const std::array<std::pair<const char *, double>, 2> ocsBulgeVariants{{
        {"negative-minor", -std::tan(M_PI / 8.0)},
        {"positive-major", 2.0}}};
    for (const auto &[variantName, bulge] : ocsBulgeVariants) {
      for (const auto &format : bulgedFormats) {
#ifndef DWGSUPPORT
        if (format.dwg)
          continue;
#endif
        DRW_LWPolyline fixture = bulged;
        fixture.vertlist.front()->bulge = bulge;
        const auto path = directory.file(std::string("ocs-") + variantName +
                                         "-" + format.suffix);
#ifdef DWGSUPPORT
        if (format.dwg)
          f::dwgPolylineFile(path, fixture, format.version);
        else
          f::binaryDxfPolyline(path, fixture, format.version);
#else
        f::binaryDxfPolyline(path, fixture, format.version);
#endif
        const auto imported = stagedImport(path, format.lane);
        INFO(variantName << " " << format.suffix);
        REQUIRE(imported.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(imported.graphic);
        REQUIRE(std::distance(imported.graphic->begin(),
                              imported.graphic->end()) == 2);
        const RS_Ellipse *ellipse = nullptr;
        const RS_Line *line = nullptr;
        for (const RS_Entity *entity : *imported.graphic) {
          CHECK(hasFragmentXData(*entity));
          CHECK(hasProjectionApp(*entity, "LibreCAD_POLYLINE_OCS"));
          CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
          if (const auto *candidate = dynamic_cast<const RS_Ellipse *>(entity)) {
            ellipse = candidate;
            CHECK(hasProjectionApp(*entity, "LibreCAD_CONIC_OCS"));
          }
          if (const auto *candidate = dynamic_cast<const RS_Line *>(entity))
            line = candidate;
        }
        REQUIRE(ellipse);
        REQUIRE(line);
        CHECK(ellipse->getAngleLength() ==
              Catch::Approx(std::abs(4.0 * std::atan(bulge))));
        CHECK(std::isfinite(ellipse->getMajorRadius()));
        CHECK(ellipse->getMajorRadius() > 0.0);
        CHECK(ellipse->getRatio() == Catch::Approx(0.8));
        CHECK(samePoint(ellipse->getStartpoint(), RS_Vector{1.8, 0.0}));
        CHECK(samePoint(ellipse->getEndpoint(), RS_Vector{1.8, 2.0}));
        CHECK(samePoint(line->getStartpoint(), RS_Vector{1.8, 2.0}));
        CHECK(samePoint(line->getEndpoint(), RS_Vector{0.2, 2.0}));
      }
    }

    const auto projectedDxf = directory.file("bulged-roundtrip.dxf");
    REQUIRE(lc::test::exportDrawing(*unsupportedOutcome.graphic, projectedDxf,
                                    RS2::FormatDXFRW2018));
    const auto projectedAppIds = lc::test::dxfRecords(projectedDxf, "APPID");
    CHECK(std::any_of(projectedAppIds.begin(), projectedAppIds.end(),
                      [](const auto &record) {
      const auto names = record.own(2);
      return names.size() == 1 && names.front() == "LibreCAD_POLYLINE_OCS";
    }));
    RS_Graphic reopenedBulged;
    REQUIRE(lc::test::importDrawing(reopenedBulged, projectedDxf,
                                    RS2::FormatDXFRW2018));
    REQUIRE(std::distance(reopenedBulged.begin(), reopenedBulged.end()) == 2);
    for (const RS_Entity *entity : reopenedBulged)
      CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
    for (const RS_Entity *entity : reopenedBulged)
      CHECK(hasFragmentXData(*entity));
#ifdef DWGSUPPORT
    const auto projectedDwg = directory.file("bulged-roundtrip.dwg");
    RS_FilterDXFRW projectedWriter;
    REQUIRE(projectedWriter.fileExport(*unsupportedOutcome.graphic,
                                       lc::test::toQString(projectedDwg),
                                       RS2::FormatDWG2018));
    RS_Graphic reopenedBulgedDwg;
    REQUIRE(lc::test::importDrawing(reopenedBulgedDwg, projectedDwg,
                                    RS2::FormatDWG2018));
    REQUIRE(std::distance(reopenedBulgedDwg.begin(),
                          reopenedBulgedDwg.end()) == 2);
    for (const RS_Entity *entity : reopenedBulgedDwg)
      CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
    for (const RS_Entity *entity : reopenedBulgedDwg)
      CHECK(hasFragmentXData(*entity));
    CHECK(reopenedBulgedDwg.dwgAdvancedMetadata().findAppIdTableEntryByName(
              "LibreCAD_POLYLINE_OCS") != nullptr);

    const auto fromDwgDxf = directory.file("bulged-from-dwg.dxf");
    REQUIRE(lc::test::exportDrawing(*importedBulgedDwg.graphic, fromDwgDxf,
                                    RS2::FormatDXFRW2018));
    RS_Graphic reopenedFromDwgDxf;
    REQUIRE(lc::test::importDrawing(reopenedFromDwgDxf, fromDwgDxf,
                                    RS2::FormatDXFRW2018));
    checkDwgOriginProjection(reopenedFromDwgDxf);

    const auto fromDwgDwg = directory.file("bulged-from-dwg.dwg");
    RS_FilterDXFRW fromDwgWriter;
    REQUIRE(fromDwgWriter.fileExport(*importedBulgedDwg.graphic,
                                     lc::test::toQString(fromDwgDwg),
                                     RS2::FormatDWG2018));
    RS_Graphic reopenedFromDwgDwg;
    REQUIRE(lc::test::importDrawing(reopenedFromDwgDwg, fromDwgDwg,
                                    RS2::FormatDWG2018));
    checkDwgOriginProjection(reopenedFromDwgDwg);
    CHECK(reopenedFromDwgDwg.dwgAdvancedMetadata().findAppIdTableEntryByName(
              "LibreCAD_POLYLINE_OCS") != nullptr);
    CHECK(reopenedFromDwgDwg.dwgAdvancedMetadata().findAppIdTableEntryByName(
              "LibreCAD_CONIC_OCS") != nullptr);
#endif

    const auto checkBulgeVariant = [&](double bulge, bool closed,
                                       const char *name) {
      DRW_LWPolyline variant = raw;
      variant.flags = closed ? 1 : 0;
      variant.vertlist.front()->bulge = bulge;
      const auto path = directory.file(name);
      f::binaryDxfPolyline(path, variant, DRW::AC1032);
      auto imported = stagedImport(path, Lane::Binary18);
      REQUIRE(imported.status == LC_OffsetImportSession::Status::Complete);
      REQUIRE(imported.graphic);
      REQUIRE(std::distance(imported.graphic->begin(),
                            imported.graphic->end()) == (closed ? 3 : 2));
      const RS_Ellipse *ellipse = nullptr;
      std::size_t lineCount = 0;
      for (const RS_Entity *entity : *imported.graphic) {
        if (const auto *candidate = dynamic_cast<const RS_Ellipse *>(entity))
          ellipse = candidate;
        if (dynamic_cast<const RS_Line *>(entity)) ++lineCount;
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
      }
      REQUIRE(ellipse);
      CHECK(lineCount == (closed ? 2 : 1));
      CHECK(ellipse->getAngleLength() ==
            Catch::Approx(std::abs(4.0 * std::atan(bulge))));
      CHECK(samePoint(ellipse->getStartpoint(), RS_Vector{1.8, 0.0}));
      CHECK(samePoint(ellipse->getEndpoint(), RS_Vector{1.8, 2.0}));
    };
    checkBulgeVariant(-std::tan(M_PI / 8.0), false, "negative-bulge.dxf");
    checkBulgeVariant(2.0, true, "closed-major-bulge.dxf");

    DRW_LWPolyline reflectedBulge = raw;
    reflectedBulge.extPoint = {0.6, 0.0, -0.8};
    reflectedBulge.vertlist.front()->bulge = std::tan(M_PI / 8.0);
    const auto reflectedBulgePath = directory.file("reflected-bulge.dxf");
    f::binaryDxfPolyline(reflectedBulgePath, reflectedBulge, DRW::AC1032);
    const auto reflectedBulgeImport =
        stagedImport(reflectedBulgePath, Lane::Binary18);
    REQUIRE(reflectedBulgeImport.status ==
            LC_OffsetImportSession::Status::Complete);
    REQUIRE(reflectedBulgeImport.graphic);
    REQUIRE(std::distance(reflectedBulgeImport.graphic->begin(),
                          reflectedBulgeImport.graphic->end()) == 2);
    const RS_Ellipse *reflectedEllipse = nullptr;
    const RS_Line *reflectedLine = nullptr;
    for (const RS_Entity *entity : *reflectedBulgeImport.graphic) {
      if (const auto *ellipse = dynamic_cast<const RS_Ellipse *>(entity))
        reflectedEllipse = ellipse;
      if (const auto *line = dynamic_cast<const RS_Line *>(entity))
        reflectedLine = line;
      CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
    }
    REQUIRE(reflectedEllipse);
    REQUIRE(reflectedLine);
    CHECK(reflectedEllipse->isReversed());
    CHECK(reflectedEllipse->getAngleLength() == Catch::Approx(M_PI / 2.0));
    CHECK(std::hypot(reflectedEllipse->getEndpoint().x -
                     reflectedLine->getStartpoint().x,
                     reflectedEllipse->getEndpoint().y -
                     reflectedLine->getStartpoint().y) < 1e-8);

    struct BulgeFormat {
      Lane lane;
      bool dwg;
      DRW::Version version;
      const char *suffix;
    };
    const std::array<BulgeFormat, 5> bulgeFormats{{
        {Lane::Binary14, false, DRW::AC1014, "bulge-r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "bulge-r2000.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "bulge-r2018.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "bulge-r2000.dwg"},
        {Lane::Eed18, true, DRW::AC1032, "bulge-r2018.dwg"}}};
    for (const auto &format : bulgeFormats) {
#ifndef DWGSUPPORT
      if (format.dwg) continue;
#endif
      DRW_LWPolyline fixture = raw;
      fixture.vertlist.front()->bulge = std::tan(M_PI / 8.0);
      const auto path = directory.file(format.suffix);
#ifdef DWGSUPPORT
      if (format.dwg) f::dwgPolylineFile(path, fixture, format.version);
      else f::binaryDxfPolyline(path, fixture, format.version);
#else
      f::binaryDxfPolyline(path, fixture, format.version);
#endif
      auto imported = stagedImport(path, format.lane);
      REQUIRE(imported.status == LC_OffsetImportSession::Status::Complete);
      REQUIRE(imported.graphic);
      REQUIRE(std::distance(imported.graphic->begin(),
                            imported.graphic->end()) == 2);
      const RS_Ellipse *ellipse = nullptr;
      const RS_Line *line = nullptr;
      for (const RS_Entity *entity : *imported.graphic) {
        if (const auto *candidate = dynamic_cast<const RS_Ellipse *>(entity))
          ellipse = candidate;
        if (const auto *candidate = dynamic_cast<const RS_Line *>(entity))
          line = candidate;
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
      }
      REQUIRE(ellipse);
      REQUIRE(line);
      CHECK(samePoint(ellipse->getStartpoint(), RS_Vector{1.8, 0.0}));
      CHECK(samePoint(ellipse->getEndpoint(), RS_Vector{1.8, 2.0}));
      CHECK(samePoint(line->getStartpoint(), RS_Vector{1.8, 2.0}));
      CHECK(samePoint(line->getEndpoint(), RS_Vector{0.2, 2.0}));
    }

    DRW_LWPolyline rankDeficient = raw;
    rankDeficient.extPoint = {1.0, 0.0, 0.0};
    rankDeficient.vertlist.front()->bulge = std::tan(M_PI / 8.0);
    const auto rankDeficientPath = directory.file("rank-deficient-bulge.dxf");
    f::binaryDxfPolyline(rankDeficientPath, rankDeficient, DRW::AC1032);
    const auto rankDeficientImport =
        stagedImport(rankDeficientPath, Lane::Binary18);
    REQUIRE(rankDeficientImport.status ==
            LC_OffsetImportSession::Status::Complete);
    REQUIRE(rankDeficientImport.graphic);
    CHECK(rankDeficientImport.graphic->begin() ==
          rankDeficientImport.graphic->end());

    DRW_LWPolyline zeroNormalBulge = raw;
    zeroNormalBulge.extPoint = {0.0, 0.0, 0.0};
    zeroNormalBulge.vertlist.front()->bulge = std::tan(M_PI / 8.0);
    const auto zeroNormalBulgePath = directory.file("zero-normal-bulge.dxf");
    f::binaryDxfPolyline(zeroNormalBulgePath, zeroNormalBulge, DRW::AC1032);
    const auto zeroNormalBulgeImport =
        stagedImport(zeroNormalBulgePath, Lane::Binary18);
    REQUIRE(zeroNormalBulgeImport.status ==
            LC_OffsetImportSession::Status::Complete);
    REQUIRE(zeroNormalBulgeImport.graphic);
    CHECK(zeroNormalBulgeImport.graphic->begin() ==
          zeroNormalBulgeImport.graphic->end());

    REQUIRE(sourceDrawing);
    const auto dxf = directory.file("roundtrip.dxf");
    REQUIRE(lc::test::exportDrawing(*sourceDrawing, dxf, RS2::FormatDXFRW2018));
    const auto appids = lc::test::dxfRecords(dxf, "APPID");
    CHECK(std::any_of(appids.begin(), appids.end(), [](const auto& record) {
        return record.own(2) == std::vector<std::string>{"LibreCAD_POLYLINE_OCS"};
    }));
    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, dxf, RS2::FormatDXFRW2018));
    const auto* reopenedPolyline =
        dynamic_cast<const RS_Polyline*>(f::sole(reopened));
    REQUIRE(reopenedPolyline);
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(*reopenedPolyline));

#ifdef DWGSUPPORT
    const auto dwg = directory.file("roundtrip.dwg");
    RS_FilterDXFRW writer;
    REQUIRE(writer.fileExport(*sourceDrawing, lc::test::toQString(dwg),
                              RS2::FormatDWG2018));
    RS_Graphic reopenedDwg;
    REQUIRE(lc::test::importDrawing(reopenedDwg, dwg, RS2::FormatDWG2018));
    CHECK(reopenedDwg.dwgAdvancedMetadata().findAppIdTableEntryByName(
              "LibreCAD_POLYLINE_OCS") != nullptr);
    const auto* reopenedDwgPolyline =
        dynamic_cast<const RS_Polyline*>(f::sole(reopenedDwg));
    REQUIRE(reopenedDwgPolyline);
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(*reopenedDwgPolyline));
#endif
}

TEST_CASE("non-axial legacy 2D POLYLINEs project straight segments safely",
          "[lc-offsetcurve][import-session][typed-source][polyline][ocs][legacy]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-non-axial-legacy-polyline");
    DRW_Polyline raw;
    raw.basePoint = {0.0, 0.0, 3.0};
    raw.extPoint = {0.6, 0.0, 0.8};
    raw.addVertex(DRW_Vertex(0.0, 0.0, 0.0, 0.0));
    raw.addVertex(DRW_Vertex(2.0, 0.0, 0.0, 0.0));
    raw.addVertex(DRW_Vertex(2.0, 2.0, 0.0, 0.0));
    raw.extData.push_back(std::make_shared<DRW_Variant>(
        1001, std::string("FixtureSourceData")));
    raw.extData.push_back(std::make_shared<DRW_Variant>(
        1000, std::string("preserve-this-xdata")));

    const auto hasFixtureXData = [](const RS_Entity& entity) {
        const auto& ext = entity.getDrwExtData();
        const auto app = std::find_if(ext.begin(), ext.end(), [](const auto& value) {
            return value && value->code() == 1001 &&
                   value->type() == DRW_Variant::STRING &&
                   std::string{value->c_str()} == "FixtureSourceData";
        });
        return app != ext.end() && std::next(app) != ext.end() &&
               (*std::next(app))->code() == 1000 &&
               (*std::next(app))->type() == DRW_Variant::STRING &&
               std::string{(*std::next(app))->c_str()} == "preserve-this-xdata";
    };

    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};

    std::unique_ptr<RS_Graphic> sourceDrawing;
    for (const auto& format : formats) {
#ifndef DWGSUPPORT
        if (format.dwg) continue;
#endif
        const auto input = directory.file(format.suffix);
        DRW_Polyline formatPolyline = raw;
        for (auto& vertex : formatPolyline.vertlist) {
            vertex = std::make_shared<DRW_Vertex>(*vertex);
        }
#ifdef DWGSUPPORT
        if (format.dwg) f::dwg2dPolylineFile(input, formatPolyline, format.version);
        else f::binaryDxf2dPolyline(input, formatPolyline, format.version);
#else
        f::binaryDxf2dPolyline(input, formatPolyline, format.version);
#endif
        auto outcome = stagedImport(input, format.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        auto* projected = dynamic_cast<RS_Polyline*>(f::sole(*outcome.graphic));
        REQUIRE(projected);
        const auto points = projected->getVertexes();
        REQUIRE(points.size() == 3);
        CHECK(points[0].x == Catch::Approx(1.8));
        CHECK(points[0].y == Catch::Approx(0.0));
        CHECK(points[1].x == Catch::Approx(1.8));
        CHECK(points[1].y == Catch::Approx(2.0));
        CHECK(points[2].x == Catch::Approx(0.2));
        CHECK(points[2].y == Catch::Approx(2.0));
        const auto& ext = projected->getDrwExtData();
        const auto app = std::find_if(ext.begin(), ext.end(), [](const auto& value) {
            return value && value->code() == 1001 &&
                   value->type() == DRW_Variant::STRING &&
                   std::string{value->c_str()} == "LibreCAD_POLYLINE_OCS";
        });
        REQUIRE(app != ext.end());
        const auto normal = std::find_if(
            std::next(app), ext.end(), [](const auto& value) {
                return value && value->code() == 1010 &&
                       value->type() == DRW_Variant::COORD;
            });
        REQUIRE(normal != ext.end());
        REQUIRE((*normal)->coord() != nullptr);
        CHECK((*normal)->coord()->x == Catch::Approx(0.6));
        CHECK((*normal)->coord()->y == Catch::Approx(0.0));
        CHECK((*normal)->coord()->z == Catch::Approx(0.8));
        const auto elevation = std::find_if(
            std::next(normal), ext.end(), [](const auto& value) {
                return value && value->code() == 1040;
            });
        REQUIRE(elevation != ext.end());
        CHECK((*elevation)->content.d == Catch::Approx(3.0));
        CHECK(hasFixtureXData(*projected));
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*projected));
        if (!format.dwg) sourceDrawing = std::move(outcome.graphic);
    }

    REQUIRE(sourceDrawing);
    const auto *straightSource =
        dynamic_cast<const RS_Polyline *>(f::sole(*sourceDrawing));
    REQUIRE(straightSource);
    const auto straightPoints = straightSource->getVertexes();
    REQUIRE(straightPoints.size() == 3);

    DRW_Polyline bulged = raw;
    for (auto& vertex : bulged.vertlist) {
        vertex = std::make_shared<DRW_Vertex>(*vertex);
    }
    bulged.vertlist.front()->bulge = std::tan(M_PI / 8.0);
    const auto unsupportedArc = directory.file("non-axial-bulge.dxf");
    f::binaryDxf2dPolyline(unsupportedArc, bulged, DRW::AC1032);
    const auto unsupportedOutcome = stagedImport(unsupportedArc, Lane::Binary18);
    REQUIRE(unsupportedOutcome.status == LC_OffsetImportSession::Status::Complete);
    REQUIRE(unsupportedOutcome.graphic);
    REQUIRE(std::distance(unsupportedOutcome.graphic->begin(),
                          unsupportedOutcome.graphic->end()) == 2);
    const RS_Ellipse *projectedArc = nullptr;
    const RS_Line *projectedLine = nullptr;
    for (const RS_Entity *entity : *unsupportedOutcome.graphic) {
      if (const auto *ellipse = dynamic_cast<const RS_Ellipse *>(entity))
        projectedArc = ellipse;
      if (const auto *line = dynamic_cast<const RS_Line *>(entity))
        projectedLine = line;
    }
    REQUIRE(projectedArc);
    REQUIRE(projectedLine);
    CHECK(std::isfinite(projectedArc->getCenter().x));
    CHECK(std::isfinite(projectedArc->getCenter().y));
    CHECK(projectedArc->getMajorRadius() == Catch::Approx(std::sqrt(2.0)));
    CHECK(projectedArc->getRatio() == Catch::Approx(0.8));
    const auto samePoint = [](const RS_Vector &lhs, const RS_Vector &rhs) {
      return std::hypot(lhs.x - rhs.x, lhs.y - rhs.y) < 1e-8;
    };
    CHECK(samePoint(projectedArc->getStartpoint(), straightPoints[0]));
    CHECK(samePoint(projectedArc->getEndpoint(), straightPoints[1]));
    CHECK(samePoint(projectedLine->getStartpoint(), straightPoints[1]));
    CHECK(samePoint(projectedLine->getEndpoint(), straightPoints[2]));
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(*projectedArc));
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(*projectedLine));
    CHECK(hasFixtureXData(*projectedArc));
    CHECK(hasFixtureXData(*projectedLine));

    const auto hasProjectionApp = [](const RS_Entity &entity,
                                     const std::string &appName) {
      const auto &ext = entity.getDrwExtData();
      return std::any_of(ext.begin(), ext.end(), [&appName](const auto &value) {
        return value && value->code() == 1001 &&
               value->type() == DRW_Variant::STRING &&
               std::string{value->c_str()} == appName;
      });
    };
    const auto checkBulgedOcsFormat = [&](double bulge, const char *variant) {
    for (const auto &format : formats) {
#ifndef DWGSUPPORT
      if (format.dwg)
        continue;
#endif
      DRW_Polyline fixture = bulged;
      for (auto &vertex : fixture.vertlist)
        vertex = std::make_shared<DRW_Vertex>(*vertex);
      fixture.vertlist.front()->bulge = bulge;
      const auto path = directory.file(std::string("bulged-") + format.suffix);
#ifdef DWGSUPPORT
      if (format.dwg)
        f::dwg2dPolylineFile(path, fixture, format.version);
      else
        f::binaryDxf2dPolyline(path, fixture, format.version);
#else
      f::binaryDxf2dPolyline(path, fixture, format.version);
#endif
      const auto imported = stagedImport(path, format.lane);
      INFO(variant << " " << format.suffix);
      REQUIRE(imported.status == LC_OffsetImportSession::Status::Complete);
      REQUIRE(imported.graphic);
      REQUIRE(std::distance(imported.graphic->begin(),
                            imported.graphic->end()) == 2);
      const RS_Ellipse *ellipse = nullptr;
      const RS_Line *line = nullptr;
      for (const RS_Entity *entity : *imported.graphic) {
        CHECK(hasFixtureXData(*entity));
        CHECK(hasProjectionApp(*entity, "LibreCAD_POLYLINE_OCS"));
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
        if (const auto *candidate = dynamic_cast<const RS_Ellipse *>(entity)) {
          ellipse = candidate;
          CHECK(hasProjectionApp(*entity, "LibreCAD_CONIC_OCS"));
        }
        if (const auto *candidate = dynamic_cast<const RS_Line *>(entity))
          line = candidate;
      }
      REQUIRE(ellipse);
      REQUIRE(line);
      CHECK(ellipse->getAngleLength() ==
            Catch::Approx(std::abs(4.0 * std::atan(bulge))));
      CHECK(std::isfinite(ellipse->getMajorRadius()));
      CHECK(ellipse->getMajorRadius() > 0.0);
      CHECK(ellipse->getRatio() == Catch::Approx(0.8));
      CHECK(samePoint(ellipse->getStartpoint(), straightPoints[0]));
      CHECK(samePoint(ellipse->getEndpoint(), straightPoints[1]));
      CHECK(samePoint(line->getStartpoint(), straightPoints[1]));
      CHECK(samePoint(line->getEndpoint(), straightPoints[2]));
    }
    };
    checkBulgedOcsFormat(std::tan(M_PI / 8.0), "positive-minor");
    checkBulgedOcsFormat(-std::tan(M_PI / 8.0), "negative-minor");
    checkBulgedOcsFormat(2.0, "positive-major");

    const auto projectedDxf = directory.file("bulged-roundtrip.dxf");
    REQUIRE(lc::test::exportDrawing(*unsupportedOutcome.graphic, projectedDxf,
                                    RS2::FormatDXFRW2018));
    const auto projectedAppIds = lc::test::dxfRecords(projectedDxf, "APPID");
    CHECK(std::any_of(projectedAppIds.begin(), projectedAppIds.end(),
                      [](const auto &record) {
      const auto names = record.own(2);
      return names.size() == 1 && names.front() == "LibreCAD_POLYLINE_OCS";
    }));
    RS_Graphic reopenedBulged;
    REQUIRE(lc::test::importDrawing(reopenedBulged, projectedDxf,
                                    RS2::FormatDXFRW2018));
    REQUIRE(std::distance(reopenedBulged.begin(), reopenedBulged.end()) == 2);
    for (const RS_Entity *entity : reopenedBulged)
      CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
    for (const RS_Entity *entity : reopenedBulged)
      CHECK(hasFixtureXData(*entity));
#ifdef DWGSUPPORT
    const auto projectedDwg = directory.file("bulged-roundtrip.dwg");
    RS_FilterDXFRW projectedWriter;
    REQUIRE(projectedWriter.fileExport(*unsupportedOutcome.graphic,
                                       lc::test::toQString(projectedDwg),
                                       RS2::FormatDWG2018));
    RS_Graphic reopenedBulgedDwg;
    REQUIRE(lc::test::importDrawing(reopenedBulgedDwg, projectedDwg,
                                    RS2::FormatDWG2018));
    REQUIRE(std::distance(reopenedBulgedDwg.begin(),
                          reopenedBulgedDwg.end()) == 2);
    for (const RS_Entity *entity : reopenedBulgedDwg)
      CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
    for (const RS_Entity *entity : reopenedBulgedDwg)
      CHECK(hasFixtureXData(*entity));
    CHECK(reopenedBulgedDwg.dwgAdvancedMetadata().findAppIdTableEntryByName(
              "LibreCAD_POLYLINE_OCS") != nullptr);
#endif

    const auto checkBulgeVariant = [&](double bulge, bool closed,
                                       const char *name) {
      DRW_Polyline variant = raw;
      for (auto &vertex : variant.vertlist)
        vertex = std::make_shared<DRW_Vertex>(*vertex);
      variant.flags = closed ? 1 : 0;
      variant.vertlist.front()->bulge = bulge;
      const auto path = directory.file(name);
      f::binaryDxf2dPolyline(path, variant, DRW::AC1032);
      auto imported = stagedImport(path, Lane::Binary18);
      REQUIRE(imported.status == LC_OffsetImportSession::Status::Complete);
      REQUIRE(imported.graphic);
      REQUIRE(std::distance(imported.graphic->begin(),
                            imported.graphic->end()) == (closed ? 3 : 2));
      const RS_Ellipse *ellipse = nullptr;
      std::size_t lineCount = 0;
      for (const RS_Entity *entity : *imported.graphic) {
        if (const auto *candidate = dynamic_cast<const RS_Ellipse *>(entity))
          ellipse = candidate;
        if (dynamic_cast<const RS_Line *>(entity)) ++lineCount;
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
      }
      REQUIRE(ellipse);
      CHECK(lineCount == (closed ? 2 : 1));
      CHECK(ellipse->getAngleLength() ==
            Catch::Approx(std::abs(4.0 * std::atan(bulge))));
      CHECK(samePoint(ellipse->getStartpoint(), straightPoints[0]));
      CHECK(samePoint(ellipse->getEndpoint(), straightPoints[1]));
    };
    checkBulgeVariant(-std::tan(M_PI / 8.0), false, "negative-bulge.dxf");
    checkBulgeVariant(2.0, true, "closed-major-bulge.dxf");

    struct BulgeFormat {
      Lane lane;
      bool dwg;
      DRW::Version version;
      const char *suffix;
    };
    const std::array<BulgeFormat, 5> bulgeFormats{{
        {Lane::Binary14, false, DRW::AC1014, "bulge-r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "bulge-r2000.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "bulge-r2018.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "bulge-r2000.dwg"},
        {Lane::Eed18, true, DRW::AC1032, "bulge-r2018.dwg"}}};
    for (const auto &format : bulgeFormats) {
#ifndef DWGSUPPORT
      if (format.dwg) continue;
#endif
      DRW_Polyline fixture = raw;
      for (auto &vertex : fixture.vertlist)
        vertex = std::make_shared<DRW_Vertex>(*vertex);
      fixture.vertlist.front()->bulge = std::tan(M_PI / 8.0);
      fixture.extData.clear();
      const auto path = directory.file(format.suffix);
#ifdef DWGSUPPORT
      if (format.dwg) f::dwg2dPolylineFile(path, fixture, format.version);
      else f::binaryDxf2dPolyline(path, fixture, format.version);
#else
      f::binaryDxf2dPolyline(path, fixture, format.version);
#endif
      auto imported = stagedImport(path, format.lane);
      REQUIRE(imported.status == LC_OffsetImportSession::Status::Complete);
      REQUIRE(imported.graphic);
      REQUIRE(std::distance(imported.graphic->begin(),
                            imported.graphic->end()) == 2);
      const RS_Ellipse *ellipse = nullptr;
      const RS_Line *line = nullptr;
      for (const RS_Entity *entity : *imported.graphic) {
        if (const auto *candidate = dynamic_cast<const RS_Ellipse *>(entity))
          ellipse = candidate;
        if (const auto *candidate = dynamic_cast<const RS_Line *>(entity))
          line = candidate;
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
      }
      REQUIRE(ellipse);
      REQUIRE(line);
      CHECK(samePoint(ellipse->getStartpoint(), straightPoints[0]));
      CHECK(samePoint(ellipse->getEndpoint(), straightPoints[1]));
      CHECK(samePoint(line->getStartpoint(), straightPoints[1]));
      CHECK(samePoint(line->getEndpoint(), straightPoints[2]));
    }

    DRW_Polyline rankDeficientBulge = raw;
    for (auto &vertex : rankDeficientBulge.vertlist)
      vertex = std::make_shared<DRW_Vertex>(*vertex);
    rankDeficientBulge.extPoint = {1.0, 0.0, 0.0};
    rankDeficientBulge.vertlist.front()->bulge = std::tan(M_PI / 8.0);
    const auto rankDeficientBulgePath =
        directory.file("rank-deficient-bulge.dxf");
    f::binaryDxf2dPolyline(rankDeficientBulgePath, rankDeficientBulge,
                           DRW::AC1032);
    const auto rankDeficientBulgeImport =
        stagedImport(rankDeficientBulgePath, Lane::Binary18);
    REQUIRE(rankDeficientBulgeImport.status ==
            LC_OffsetImportSession::Status::Complete);
    REQUIRE(rankDeficientBulgeImport.graphic);
    CHECK(rankDeficientBulgeImport.graphic->begin() ==
          rankDeficientBulgeImport.graphic->end());

    DRW_Polyline zeroNormal = raw;
    zeroNormal.extPoint = {0.0, 0.0, 0.0};
    const auto zeroNormalFile = directory.file("zero-normal.dxf");
    f::binaryDxf2dPolyline(zeroNormalFile, zeroNormal, DRW::AC1032);
    const auto zeroNormalOutcome = stagedImport(zeroNormalFile, Lane::Binary18);
    REQUIRE(zeroNormalOutcome.status == LC_OffsetImportSession::Status::Complete);
    REQUIRE(zeroNormalOutcome.graphic);
    CHECK(std::none_of(
        zeroNormalOutcome.graphic->begin(), zeroNormalOutcome.graphic->end(),
        [](const RS_Entity* entity) {
            return dynamic_cast<const RS_Polyline*>(entity) != nullptr;
        }));

    REQUIRE(sourceDrawing);
    const auto dxf = directory.file("roundtrip.dxf");
    REQUIRE(lc::test::exportDrawing(*sourceDrawing, dxf, RS2::FormatDXFRW2018));
    const auto appids = lc::test::dxfRecords(dxf, "APPID");
    CHECK(std::any_of(appids.begin(), appids.end(), [](const auto& record) {
        return record.own(2) == std::vector<std::string>{"LibreCAD_POLYLINE_OCS"};
    }));
    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, dxf, RS2::FormatDXFRW2018));
    const auto* reopenedPolyline = dynamic_cast<const RS_Polyline*>(f::sole(reopened));
    REQUIRE(reopenedPolyline);
    CHECK(hasFixtureXData(*reopenedPolyline));
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(*reopenedPolyline));

#ifdef DWGSUPPORT
    const auto dwg = directory.file("roundtrip.dwg");
    RS_FilterDXFRW writer;
    REQUIRE(writer.fileExport(*sourceDrawing, lc::test::toQString(dwg),
                              RS2::FormatDWG2018));
    RS_Graphic reopenedDwg;
    REQUIRE(lc::test::importDrawing(reopenedDwg, dwg, RS2::FormatDWG2018));
    CHECK(reopenedDwg.dwgAdvancedMetadata().findAppIdTableEntryByName(
              "LibreCAD_POLYLINE_OCS") != nullptr);
    const auto* reopenedDwgPolyline =
        dynamic_cast<const RS_Polyline*>(f::sole(reopenedDwg));
    REQUIRE(reopenedDwgPolyline);
    CHECK(hasFixtureXData(*reopenedDwgPolyline));
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(*reopenedDwgPolyline));
#endif
}

TEST_CASE("non-axial CIRCLE ARC and ELLIPSE import as marked XY conics",
          "[lc-offsetcurve][import-session][typed-source][conic][ocs]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-non-axial-conic-projection");
    const DRW_Coord normal{0.6, 0.0, 0.8};
    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};
    const std::array<f::ConicKind, 3> kinds{
        f::ConicKind::Circle, f::ConicKind::Arc, f::ConicKind::Ellipse};

    for (const auto kind : kinds) {
        std::unique_ptr<RS_Graphic> roundTripSource;
        for (const auto& format : formats) {
#ifndef DWGSUPPORT
            if (format.dwg) continue;
#endif
            const auto input = directory.file(
                std::string(format.suffix) + "-" + std::to_string(static_cast<int>(kind)));
#ifdef DWGSUPPORT
            if (format.dwg) f::dwgConicFile(input, kind, normal, format.version);
            else f::binaryDxfConic(input, kind, normal, format.version);
#else
            f::binaryDxfConic(input, kind, normal, format.version);
#endif
            auto outcome = stagedImport(input, format.lane);
            REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
            REQUIRE(outcome.graphic);
            auto* ellipse = dynamic_cast<RS_Ellipse*>(f::sole(*outcome.graphic));
            REQUIRE(ellipse);
            CHECK_FALSE(LC_OffsetSourceGeometry::capture(*ellipse));
            const auto& data = ellipse->getData();
            const auto& ext = ellipse->getDrwExtData();
            const auto marker = std::find_if(ext.begin(), ext.end(), [](const auto& value) {
                return value && value->code() == 1001 &&
                       value->type() == DRW_Variant::STRING &&
                       std::string{value->c_str()} == "LibreCAD_CONIC_OCS";
            });
            REQUIRE(marker != ext.end());
            const auto markerKind = std::find_if(std::next(marker), ext.end(), [](const auto& value) {
                return value && value->code() == 1070;
            });
            REQUIRE(markerKind != ext.end());
            CHECK((*markerKind)->i_val() == static_cast<int>(kind));
            const auto markerNormal = std::find_if(std::next(markerKind), ext.end(), [](const auto& value) {
                return value && value->code() == 1010 &&
                       value->type() == DRW_Variant::COORD;
            });
            REQUIRE(markerNormal != ext.end());
            REQUIRE((*markerNormal)->coord() != nullptr);
            CHECK((*markerNormal)->coord()->x == Catch::Approx(normal.x));
            CHECK((*markerNormal)->coord()->y == Catch::Approx(normal.y));
            CHECK((*markerNormal)->coord()->z == Catch::Approx(normal.z));
            CHECK(data.center.x == Catch::Approx(0.2).margin(1e-10));
            CHECK(data.center.y == Catch::Approx(1.0).margin(1e-10));
            CHECK(data.majorP.x == Catch::Approx(0.0).margin(1e-10));
            CHECK(data.majorP.y == Catch::Approx(5.0).margin(1e-10));
            const double expectedRatio = kind == f::ConicKind::Ellipse ? 0.4 : 0.8;
            CHECK(data.ratio == Catch::Approx(expectedRatio).margin(1e-10));
            if (kind != f::ConicKind::Circle) {
                CHECK(ellipse->isEllipticArc());
                CHECK(ellipse->getStartpoint().x == Catch::Approx(0.2).margin(1e-9));
                CHECK(ellipse->getStartpoint().y == Catch::Approx(6.0).margin(1e-9));
                const double expectedEndX = kind == f::ConicKind::Arc ? -3.8 : -1.8;
                CHECK(ellipse->getEndpoint().x == Catch::Approx(expectedEndX).margin(1e-9));
                CHECK(ellipse->getEndpoint().y == Catch::Approx(1.0).margin(1e-9));
            }
            if (!format.dwg) roundTripSource = std::move(outcome.graphic);
        }

        REQUIRE(roundTripSource);
        const auto dxf = directory.file("roundtrip-" +
            std::to_string(static_cast<int>(kind)) + ".dxf");
        REQUIRE(lc::test::exportDrawing(*roundTripSource, dxf, RS2::FormatDXFRW2018));
        const auto appids = lc::test::dxfRecords(dxf, "APPID");
        CHECK(std::any_of(appids.begin(), appids.end(), [](const auto& record) {
            return record.own(2) == std::vector<std::string>{"LibreCAD_CONIC_OCS"};
        }));
        RS_Graphic reopened;
        REQUIRE(lc::test::importDrawing(reopened, dxf, RS2::FormatDXFRW2018));
        auto* reopenedEllipse = dynamic_cast<RS_Ellipse*>(f::sole(reopened));
        REQUIRE(reopenedEllipse);
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*reopenedEllipse));
#ifdef DWGSUPPORT
        const auto dwg = directory.file("roundtrip-" +
            std::to_string(static_cast<int>(kind)) + ".dwg");
        RS_FilterDXFRW writer;
        REQUIRE(writer.fileExport(*roundTripSource, lc::test::toQString(dwg),
                                  RS2::FormatDWG2018));
        RS_Graphic reopenedDwg;
        REQUIRE(lc::test::importDrawing(reopenedDwg, dwg, RS2::FormatDWG2018));
        CHECK(reopenedDwg.dwgAdvancedMetadata().findAppIdTableEntryByName(
                  "LibreCAD_CONIC_OCS") != nullptr);
        auto* reopenedDwgEllipse = dynamic_cast<RS_Ellipse*>(f::sole(reopenedDwg));
        REQUIRE(reopenedDwgEllipse);
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*reopenedDwgEllipse));
#endif
    }

    const auto reflectedArc = directory.file("reflected-normal-arc.dxf");
    f::binaryDxfConic(reflectedArc, f::ConicKind::Arc,
                      DRW_Coord{0.6, 0.0, -0.8}, DRW::AC1032);
    const auto reflectedOutcome = stagedImport(reflectedArc, Lane::Binary18);
    REQUIRE(reflectedOutcome.status == LC_OffsetImportSession::Status::Complete);
    REQUIRE(reflectedOutcome.graphic);
    const auto* reflected = dynamic_cast<const RS_Ellipse*>(f::sole(*reflectedOutcome.graphic));
    REQUIRE(reflected);
    CHECK(reflected->isReversed());
    CHECK(reflected->getStartpoint().x == Catch::Approx(3.4).margin(1e-9));
    CHECK(reflected->getStartpoint().y == Catch::Approx(6.0).margin(1e-9));
    CHECK(reflected->getEndpoint().x == Catch::Approx(7.4).margin(1e-9));
    CHECK(reflected->getEndpoint().y == Catch::Approx(1.0).margin(1e-9));
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(*reflected));

    const auto degenerate = directory.file("degenerate-xy-circle.dxf");
    f::binaryDxfConic(degenerate, f::ConicKind::Circle,
                      DRW_Coord{1.0, 0.0, 0.0}, DRW::AC1032);
    const auto degenerateOutcome = stagedImport(degenerate, Lane::Binary18);
    REQUIRE(degenerateOutcome.status == LC_OffsetImportSession::Status::Complete);
    REQUIRE(degenerateOutcome.graphic);
    CHECK(std::none_of(degenerateOutcome.graphic->begin(),
                       degenerateOutcome.graphic->end(), [](const RS_Entity* entity) {
        return dynamic_cast<const RS_Ellipse*>(entity) != nullptr;
    }));
}

TEST_CASE("closed legacy 2D POLYLINE import preserves its closing bulge",
          "[lc-offsetcurve][import-session][typed-source][polyline][legacy][closed]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-closed-legacy-polyline-import");
    const double closingBulge = std::tan(M_PI / 16.0);
    DRW_Polyline raw;
    raw.flags = 1;
    raw.addVertex(DRW_Vertex(0, 0, 0, 0));
    raw.addVertex(DRW_Vertex(2, 0, 0, 0));
    raw.addVertex(DRW_Vertex(2, 2, 0, 0));
    raw.addVertex(DRW_Vertex(0, 2, 0, closingBulge));

    RS_Polyline expected;
    expected.addVertex({0, 0});
    expected.addVertex({2, 0});
    expected.addVertex({2, 2});
    expected.addVertex({0, 2}, closingBulge);
    expected.setClosed(true, closingBulge);
    const auto expectedSource = LC_OffsetSourceGeometry::capture(expected);
    REQUIRE(expectedSource);
    const auto& expectedData = std::get<LC_OffsetPolylineData>(expectedSource->value);

    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 4> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& format : formats) {
#ifndef DWGSUPPORT
        if (format.dwg) continue;
#endif
        const auto path = directory.file(std::string("closed-legacy-polyline-") + format.suffix);
#ifdef DWGSUPPORT
        if (format.dwg) f::dwg2dPolylineFile(path, raw, format.version);
        else f::binaryDxf2dPolyline(path, raw, format.version);
#else
        f::binaryDxf2dPolyline(path, raw, format.version);
#endif

        auto outcome = stagedImport(path, format.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        std::vector<const RS_Polyline*> polylines;
        for (const RS_Entity* entity : *outcome.graphic) {
            if (const auto* polyline = dynamic_cast<const RS_Polyline*>(entity))
                polylines.push_back(polyline);
        }
        REQUIRE(polylines.size() == 1);
        const auto actualSource = LC_OffsetSourceGeometry::capture(*polylines.front());
        REQUIRE(actualSource);
        const auto& actualData = std::get<LC_OffsetPolylineData>(actualSource->value);
        CHECK(actualData.closed);
        REQUIRE(actualData.segments.size() == expectedData.segments.size());
        CHECK(actualData.startpoint.distanceTo(expectedData.startpoint) < 1.0e-12);
        REQUIRE(std::holds_alternative<RS_ArcData>(actualData.segments.back()));
        const auto& actualClosingArc = std::get<RS_ArcData>(actualData.segments.back());
        const auto& expectedClosingArc = std::get<RS_ArcData>(expectedData.segments.back());
        CHECK(actualClosingArc.center.distanceTo(expectedClosingArc.center) < 1.0e-12);
        CHECK(actualClosingArc.radius == Catch::Approx(expectedClosingArc.radius).margin(1.0e-12));
        CHECK(actualClosingArc.reversed == expectedClosingArc.reversed);
        CHECK(LC_OffsetSourceGeometry{actualData}.isWellFormed());
    }
}

TEST_CASE("polyline bulge conversion follows the minimum valid arc sweep",
          "[lc-offsetcurve][import-session][typed-source][polyline][bulge][boundary]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-polyline-bulge-sweep-boundary");
    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 3> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const double sweepFactor : {0.5, 2.0}) {
        for (const double sign : {-1.0, 1.0}) {
            const double sweep = RS_TOLERANCE_ANGLE * sweepFactor;
            const double bulge = sign * std::tan(sweep / 4.0);
            for (const auto& format : formats) {
#ifndef DWGSUPPORT
                if (format.dwg) continue;
#endif
                DRW_LWPolyline raw;
                raw.vertexnum = 3;
                raw.addVertex(DRW_Vertex2D(0, 0, bulge));
                raw.addVertex(DRW_Vertex2D(2, 0, 0));
                raw.addVertex(DRW_Vertex2D(4, 1, 0));
                const auto path = directory.file(std::string("sweep-") +
                    std::to_string(sweepFactor) + (sign < 0 ? "-" : "+") + format.suffix);
#ifdef DWGSUPPORT
                if (format.dwg) f::dwgPolylineFile(path, raw, format.version);
                else f::binaryDxfPolyline(path, raw, format.version);
#else
                f::binaryDxfPolyline(path, raw, format.version);
#endif

                auto outcome = stagedImport(path, format.lane);
                REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
                REQUIRE(outcome.graphic);
                std::vector<const RS_Polyline*> polylines;
                for (const RS_Entity* entity : *outcome.graphic) {
                    if (const auto* polyline = dynamic_cast<const RS_Polyline*>(entity))
                        polylines.push_back(polyline);
                }
                REQUIRE(polylines.size() == 1);
                REQUIRE(polylines.front()->size() == 2);
                const bool expectArc = sweepFactor > 1.0;
                CHECK((polylines.front()->entityAt(0)->rtti() == RS2::EntityArc) == expectArc);
                const auto source = LC_OffsetSourceGeometry::capture(*polylines.front());
                CAPTURE(sweepFactor, sign, format.suffix);
                CHECK(source.has_value());
            }
        }
    }
}

TEST_CASE("closed bulged LWPOLYLINE import preserves its closing arc in DXF and DWG",
          "[lc-offsetcurve][import-session][typed-source][polyline][bulge]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-closed-bulged-polyline-import");
    const double bulge = std::tan(M_PI / 16.0);
    DRW_LWPolyline raw;
    raw.flags = 1;
    raw.vertexnum = 4;
    raw.addVertex(DRW_Vertex2D(0, 0, 0));
    raw.addVertex(DRW_Vertex2D(2, 0, 0));
    raw.addVertex(DRW_Vertex2D(2, 2, 0));
    raw.addVertex(DRW_Vertex2D(0, 2, bulge));

    RS_Polyline expected;
    expected.addVertex({0, 0});
    expected.addVertex({2, 0});
    expected.addVertex({2, 2});
    expected.addVertex({0, 2}, bulge);
    expected.setClosed(true, bulge);
    const auto expectedSource = LC_OffsetSourceGeometry::capture(expected);
    REQUIRE(expectedSource);
    const auto& expectedData = std::get<LC_OffsetPolylineData>(expectedSource->value);
    REQUIRE(expectedData.closed);
    REQUIRE(expectedData.segments.size() == 4);
    REQUIRE(std::holds_alternative<RS_ArcData>(expectedData.segments.back()));

    for (const auto& [lane, dwg] : std::array<std::pair<Lane, bool>, 2>{{
             {Lane::Binary18, false}, {Lane::Eed18, true}}}) {
#ifndef DWGSUPPORT
        if (dwg) continue;
#endif
        const auto path = directory.file(dwg ? "closed.dwg" : "closed.dxf");
#ifdef DWGSUPPORT
        if (dwg) f::dwgPolylineFile(path, raw, DRW::AC1032);
        else f::binaryDxfPolyline(path, raw, DRW::AC1032);
#else
        f::binaryDxfPolyline(path, raw, DRW::AC1032);
#endif
        const auto outcome = stagedImport(path, lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        const auto* imported = dynamic_cast<const RS_Polyline*>(f::sole(*outcome.graphic));
        REQUIRE(imported);
        const auto actualSource = LC_OffsetSourceGeometry::capture(*imported);
        REQUIRE(actualSource);
        const auto& actualData = std::get<LC_OffsetPolylineData>(actualSource->value);
        REQUIRE(actualData.closed);
        REQUIRE(actualData.segments.size() == expectedData.segments.size());
        for (std::size_t i = 0; i + 1 < actualData.segments.size(); ++i)
            CHECK(std::holds_alternative<RS_LineData>(actualData.segments[i]));
        REQUIRE(std::holds_alternative<RS_ArcData>(actualData.segments.back()));
        const auto& expectedArc = std::get<RS_ArcData>(expectedData.segments.back());
        const auto& actualArc = std::get<RS_ArcData>(actualData.segments.back());
        CHECK(actualArc.center.x == Catch::Approx(expectedArc.center.x));
        CHECK(actualArc.center.y == Catch::Approx(expectedArc.center.y));
        CHECK(actualArc.radius == Catch::Approx(expectedArc.radius));
        CHECK(actualArc.angle1 == Catch::Approx(expectedArc.angle1));
        CHECK(actualArc.angle2 == Catch::Approx(expectedArc.angle2));
        CHECK(actualArc.reversed == expectedArc.reversed);
    }
}

TEST_CASE("reflected OCS bulge import mirrors coordinates and reverses arc orientation",
          "[lc-offsetcurve][import-session][typed-source][polyline][bulge][ocs]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-reflected-bulged-polyline-import");
    const double bulge = std::tan(M_PI / 16.0);
    DRW_LWPolyline raw;
    raw.vertexnum = 4;
    raw.extPoint.z = -1.0;
    raw.addVertex(DRW_Vertex2D(0, 0, bulge));
    raw.addVertex(DRW_Vertex2D(2, 0, -bulge));
    raw.addVertex(DRW_Vertex2D(4, 0, 0));
    raw.addVertex(DRW_Vertex2D(6, 0, 0));

    RS_Polyline expected;
    expected.addVertex({0, 0}, -bulge);
    expected.addVertex({-2, 0}, bulge);
    expected.addVertex({-4, 0});
    expected.addVertex({-6, 0});
    const auto expectedSource = LC_OffsetSourceGeometry::capture(expected);
    REQUIRE(expectedSource);
    const auto& expectedData = std::get<LC_OffsetPolylineData>(expectedSource->value);

    for (const auto& [lane, dwg] : std::array<std::pair<Lane, bool>, 2>{{
             {Lane::Binary18, false}, {Lane::Eed18, true}}}) {
#ifndef DWGSUPPORT
        if (dwg) continue;
#endif
        const auto path = directory.file(dwg ? "reflected.dwg" : "reflected.dxf");
#ifdef DWGSUPPORT
        if (dwg) f::dwgPolylineFile(path, raw, DRW::AC1032);
        else f::binaryDxfPolyline(path, raw, DRW::AC1032);
#else
        f::binaryDxfPolyline(path, raw, DRW::AC1032);
#endif
        const auto outcome = stagedImport(path, lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        const auto* imported = dynamic_cast<const RS_Polyline*>(f::sole(*outcome.graphic));
        REQUIRE(imported);
        const auto actualSource = LC_OffsetSourceGeometry::capture(*imported);
        REQUIRE(actualSource);
        const auto& actualData = std::get<LC_OffsetPolylineData>(actualSource->value);
        REQUIRE(actualData.segments.size() == expectedData.segments.size());
        CHECK(actualData.startpoint.x == Catch::Approx(expectedData.startpoint.x));
        CHECK(actualData.endpoint.x == Catch::Approx(expectedData.endpoint.x));
        for (std::size_t i = 0; i < expectedData.segments.size(); ++i) {
            CAPTURE(dwg, i);
            REQUIRE(actualData.segments[i].index() == expectedData.segments[i].index());
            if (const auto* expectedArc = std::get_if<RS_ArcData>(&expectedData.segments[i])) {
                const auto& actualArc = std::get<RS_ArcData>(actualData.segments[i]);
                CHECK(actualArc.center.x == Catch::Approx(expectedArc->center.x));
                CHECK(actualArc.center.y == Catch::Approx(expectedArc->center.y));
                CHECK(actualArc.radius == Catch::Approx(expectedArc->radius));
                CHECK(actualArc.angle1 == Catch::Approx(expectedArc->angle1));
                CHECK(actualArc.angle2 == Catch::Approx(expectedArc->angle2));
                CHECK(actualArc.reversed == expectedArc->reversed);
            }
        }
    }
}

TEST_CASE("unweighted spline source imports through operation-8 validation in DXF and DWG",
          "[lc-offsetcurve][import-session][typed-source][spline-variant]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-unweighted-spline-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    RS_SplineData unweighted{1, false};
    unweighted.controlPoints = {{0, 0}, {1, 0}, {2, 0}};
    unweighted.knotslist = {0, 0, .5, 1, 1};
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{unweighted}, .5, 1e-4, false};
    auto source = definition.source.makeEntity();
    REQUIRE(source);
    const auto options = LC_CurveOffset::makeDirectOptions(*source, .5, definition.relativeTolerance);
    const auto generated = LC_CurveOffset::buildDirectBranches(*source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, .5), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
    auto materialized = LC_CurveOffset::materializeBranches(*source, generated, options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(materialized.entities.size() == 1);
    const auto* line = dynamic_cast<const RS_Line*>(materialized.entities.front().get());
    const auto* spline = dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
    REQUIRE((line || spline));

    struct Format {
        Lane lane;
        bool dwg;
        DRW::Version version;
        const char* suffix;
    };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& fileFormat : formats) {
        const auto [lane, dwg, version, suffix] = fileFormat;
#ifndef DWGSUPPORT
        if (dwg) continue;
#endif
        xd::Definition carrier;
        carrier.source = definition;
        xd::Options encoding;
        if (dwg) { encoding.eed = true; encoding.textEncoding = xd::TextEncoding::Utf16; }
        else encoding.asciiDxf = false;
        const auto tags = xd::forExport({std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
            carrier, encoding);
        REQUIRE(tags);
        DRW_Spline cache = f::raw(1, *tags);
        cache.controllist.clear();
        if (line) {
            cache.controllist.push_back(std::make_shared<DRW_Coord>(line->getStartpoint().x,
                line->getStartpoint().y, 0));
            cache.controllist.push_back(std::make_shared<DRW_Coord>(line->getEndpoint().x,
                line->getEndpoint().y, 0));
            cache.knotslist = {0, 0, 1, 1};
            cache.weightlist = {1, 1};
            cache.degree = 1;
        } else {
            const auto& data = spline->getData();
            REQUIRE((data.degree == 1 || data.degree == 3));
            for (const auto& point : data.controlPoints)
                cache.controllist.push_back(std::make_shared<DRW_Coord>(point.x, point.y, 0));
            cache.knotslist = data.knotslist;
            cache.weightlist = data.weights;
            cache.degree = data.degree;
        }
        cache.flags = 8;
        cache.m_scenario = 1;
        cache.ncontrol = cache.controllist.size();
        cache.nknots = cache.knotslist.size();
        cache.nfit = 0;

        const auto input = directory.file(std::string("unweighted-") + suffix);
#ifdef DWGSUPPORT
        if (dwg) f::dwgFile(input, cache, version);
        else f::binaryDxf(input, cache, version);
#else
        f::binaryDxf(input, cache, version);
#endif
        LC_OffsetImportSession session;
        std::optional<LC_OffsetImportSession::Outcome> outcome;
        REQUIRE(session.start(QString::fromStdString(input.string()), format(lane),
            RS2::DxfTextEncoding::Declared, false,
            [&](LC_OffsetImportSession::Outcome result) { outcome.emplace(std::move(result)); }));
        QElapsedTimer elapsed; elapsed.start();
        while (!outcome) {
            REQUIRE(elapsed.elapsed() < 35000);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QThread::msleep(1);
        }
        REQUIRE(outcome->status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome->graphic);
        CHECK(outcome->admitted == 1);
        CHECK(outcome->downgraded == 0);
        auto* imported = dynamic_cast<LC_OffsetCurve*>(f::sole(*outcome->graphic));
        REQUIRE(imported);
        REQUIRE(imported->sourceDefinition());
        CHECK(values::encodeSourceGeometry(imported->sourceDefinition()->source) ==
              values::encodeSourceGeometry(definition.source));
        REQUIRE(std::holds_alternative<RS_SplineData>(imported->sourceDefinition()->source.value));
        CHECK(std::get<RS_SplineData>(imported->sourceDefinition()->source.value).weights.empty());
    }
}

TEST_CASE("wrapped periodic spline source imports through operation-8 validation in DXF and DWG",
          "[lc-offsetcurve][import-session][typed-source][spline-variant]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-wrapped-spline-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    RS_Spline periodic{nullptr, RS_SplineData{3, false}};
    for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30},
                                   RS_Vector{20, 50}, RS_Vector{-15, 25}})
        periodic.addControlPoint(point);
    periodic.setClosed(true);
    auto wrapped = periodic.getData();
    wrapped.weights.clear();
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{wrapped}, .25, 1e-4, false};
    auto source = definition.source.makeEntity();
    REQUIRE(source);
    const auto options = LC_CurveOffset::makeDirectOptions(*source, .25,
        definition.relativeTolerance);
    const auto generated = LC_CurveOffset::buildDirectBranches(*source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, .25), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
    auto materialized = LC_CurveOffset::materializeBranches(*source, generated, options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(materialized.entities.size() == 1);
    const auto* spline = dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
    REQUIRE(spline);
    REQUIRE(spline->validate());
    const RS_SplineData expectedCache = spline->getData();
    REQUIRE(expectedCache.knotslist.size() == expectedCache.controlPoints.size() +
            static_cast<std::size_t>(expectedCache.degree) + 1);
    REQUIRE(expectedCache.weights.size() == expectedCache.controlPoints.size());

    struct Format {
        Lane lane;
        bool dwg;
        DRW::Version version;
        const char* suffix;
    };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& fileFormat : formats) {
        const auto [lane, dwg, version, suffix] = fileFormat;
#ifndef DWGSUPPORT
        if (dwg) continue;
#endif
        xd::Definition carrier;
        carrier.source = definition;
        xd::Options encoding;
        if (dwg) { encoding.eed = true; encoding.textEncoding = xd::TextEncoding::Utf16; }
        else encoding.asciiDxf = false;
        const auto tags = xd::forExport(
            {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))}, carrier, encoding);
        REQUIRE(tags);
        DRW_Spline cache = f::raw(1, *tags);
        cache.controllist.clear();
        for (const auto& point : expectedCache.controlPoints)
            cache.controllist.push_back(std::make_shared<DRW_Coord>(point.x, point.y, 0));
        cache.knotslist = expectedCache.knotslist;
        cache.weightlist = expectedCache.weights;
        cache.degree = expectedCache.degree;
        cache.flags = 8;
        cache.m_scenario = 1;
        cache.ncontrol = cache.controllist.size();
        cache.nknots = cache.knotslist.size();
        cache.nfit = 0;

        const auto input = directory.file(std::string("wrapped-") + suffix);
#ifdef DWGSUPPORT
        if (dwg) f::dwgFile(input, cache, version);
        else f::binaryDxf(input, cache, version);
#else
        f::binaryDxf(input, cache, version);
#endif
        LC_OffsetImportSession session;
        std::optional<LC_OffsetImportSession::Outcome> outcome;
        REQUIRE(session.start(QString::fromStdString(input.string()), format(lane),
            RS2::DxfTextEncoding::Declared, false,
            [&](LC_OffsetImportSession::Outcome result) { outcome.emplace(std::move(result)); }));
        QElapsedTimer elapsed; elapsed.start();
        while (!outcome) {
            REQUIRE(elapsed.elapsed() < 35000);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QThread::msleep(1);
        }
        REQUIRE(outcome->status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome->graphic);
        CHECK(outcome->admitted == 1);
        CHECK(outcome->downgraded == 0);
        auto* imported = dynamic_cast<LC_OffsetCurve*>(f::sole(*outcome->graphic));
        REQUIRE(imported);
        REQUIRE(imported->sourceDefinition());
        const auto expectedSource = values::encodeSourceDefinition(definition);
        const auto receivedSource = values::encodeSourceDefinition(*imported->sourceDefinition());
        REQUIRE(expectedSource); REQUIRE(receivedSource);
        CHECK(QCborValue(*expectedSource).toCbor() == QCborValue(*receivedSource).toCbor());
        CHECK(RS_Spline::geometryFingerprint(imported->getData()) ==
              RS_Spline::geometryFingerprint(expectedCache));
    }
}

TEST_CASE("multi-branch rational spline import refuses native promotion and retains source data",
          "[lc-offsetcurve][import-session][typed-source][spline-variant]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-rational-knot-spline-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    RS_SplineData rational{2, false};
    rational.controlPoints = {{0, 0}, {1, 2}, {2, -1}, {3, 2}, {4, 0}};
    rational.knotslist = {0, 0, 0, .3, .7, 1, 1, 1};
    rational.weights = {1, .6, 1.8, .8, 1};
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{rational}, .2, 1e-4, false};
    auto source = definition.source.makeEntity();
    REQUIRE(source);
    const auto options = LC_CurveOffset::makeDirectOptions(*source, .2,
        definition.relativeTolerance);
    const auto generated = LC_CurveOffset::buildDirectBranches(*source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, .2), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
    auto materialized = LC_CurveOffset::materializeBranches(*source, generated, options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(materialized.entities.size() > 1);
    const auto* expectedSpline = dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
    REQUIRE(expectedSpline);
    const RS_SplineData expectedCache = expectedSpline->getData();
    REQUIRE(expectedSpline->validate());

    struct Format {
        Lane lane;
        bool dwg;
        DRW::Version version;
        const char* suffix;
    };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& fileFormat : formats) {
        const auto [lane, dwg, version, suffix] = fileFormat;
#ifndef DWGSUPPORT
        if (dwg) continue;
#endif
        xd::Definition carrier;
        carrier.source = definition;
        xd::Options encoding;
        if (dwg) { encoding.eed = true; encoding.textEncoding = xd::TextEncoding::Utf16; }
        else encoding.asciiDxf = false;
        const auto tags = xd::forExport(
            {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))}, carrier, encoding);
        REQUIRE(tags);
        DRW_Spline cache = f::raw(1, *tags);
        cache.controllist.clear();
        for (const auto& point : expectedCache.controlPoints)
            cache.controllist.push_back(std::make_shared<DRW_Coord>(point.x, point.y, 0));
        cache.knotslist = expectedCache.knotslist;
        cache.weightlist = expectedCache.weights;
        cache.degree = expectedCache.degree;
        cache.flags = 8;
        cache.m_scenario = 1;
        cache.ncontrol = cache.controllist.size();
        cache.nknots = cache.knotslist.size();
        cache.nfit = 0;

        const auto input = directory.file(std::string("rational-") + suffix);
#ifdef DWGSUPPORT
        if (dwg) f::dwgFile(input, cache, version);
        else f::binaryDxf(input, cache, version);
#else
        f::binaryDxf(input, cache, version);
#endif
        const auto outcome = stagedImport(input, lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        CHECK(outcome.admitted == 0);
        CHECK(outcome.downgraded == 1);
        auto* imported = f::sole(*outcome.graphic);
        REQUIRE(imported);
        CHECK(dynamic_cast<const LC_OffsetCurve*>(imported) == nullptr);
        const auto* fallback = dynamic_cast<const RS_Spline*>(imported);
        REQUIRE(fallback);
        CHECK(RS_Spline::geometryFingerprint(fallback->getData()) ==
              RS_Spline::geometryFingerprint(expectedCache));
        const auto parsed = xd::parseAndStrip(imported->getDrwExtData(), encoding);
        REQUIRE(parsed.status == xd::Status::Schema2);
        REQUIRE(parsed.definition);
        REQUIRE(parsed.definition->source);
        const auto expectedSource = values::encodeSourceDefinition(definition);
        const auto actualSource = values::encodeSourceDefinition(*parsed.definition->source);
        REQUIRE(expectedSource);
        REQUIRE(actualSource);
        CHECK(QCborValue(*expectedSource).toCbor() == QCborValue(*actualSource).toCbor());
    }
}

TEST_CASE("single-cache internal-knot rational spline imports as native offset curve",
          "[lc-offsetcurve][import-session][typed-source][spline-variant]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-single-cache-rational-spline-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    RS_SplineData rational{2, false};
    rational.controlPoints = {{0, 0}, {1, .25}, {3, .25}, {4, 0}};
    rational.knotslist = {0, 0, 0, .5, 1, 1, 1};
    rational.weights = {1, .9, 1.1, 1};
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{rational}, .05, 1e-4, false};
    auto source = definition.source.makeEntity();
    REQUIRE(source);
    const auto options = LC_CurveOffset::makeDirectOptions(*source, .05,
        definition.relativeTolerance);
    const auto generated = LC_CurveOffset::buildDirectBranches(*source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, .05), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
    auto materialized = LC_CurveOffset::materializeBranches(*source, generated, options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(materialized.entities.size() == 1);
    const auto* expectedSpline = dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
    REQUIRE(expectedSpline);
    const RS_SplineData expectedCache = expectedSpline->getData();
    REQUIRE(expectedSpline->validate());

    struct Format {
        Lane lane;
        bool dwg;
        DRW::Version version;
        const char* suffix;
    };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}
    }};
    for (const auto& fileFormat : formats) {
        const auto [lane, dwg, version, suffix] = fileFormat;
#ifndef DWGSUPPORT
        if (dwg) continue;
#endif
        xd::Definition carrier;
        carrier.source = definition;
        xd::Options encoding;
        if (dwg) { encoding.eed = true; encoding.textEncoding = xd::TextEncoding::Utf16; }
        else encoding.asciiDxf = false;
        const auto tags = xd::forExport(
            {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))}, carrier, encoding);
        REQUIRE(tags);
        DRW_Spline cache = f::raw(1, *tags);
        cache.controllist.clear();
        for (const auto& point : expectedCache.controlPoints)
            cache.controllist.push_back(std::make_shared<DRW_Coord>(point.x, point.y, 0));
        cache.knotslist = expectedCache.knotslist;
        cache.weightlist = expectedCache.weights;
        cache.degree = expectedCache.degree;
        cache.flags = 8;
        cache.m_scenario = 1;
        cache.ncontrol = cache.controllist.size();
        cache.nknots = cache.knotslist.size();
        cache.nfit = 0;

        const auto input = directory.file(std::string("single-cache-") + suffix);
#ifdef DWGSUPPORT
        if (dwg) f::dwgFile(input, cache, version);
        else f::binaryDxf(input, cache, version);
#else
        f::binaryDxf(input, cache, version);
#endif
        const auto outcome = stagedImport(input, lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        CHECK(outcome.admitted == 1);
        CHECK(outcome.downgraded == 0);
        const auto* imported = dynamic_cast<const LC_OffsetCurve*>(f::sole(*outcome.graphic));
        REQUIRE(imported);
        REQUIRE(imported->sourceDefinition());
        const auto expectedSource = values::encodeSourceDefinition(definition);
        const auto actualSource = values::encodeSourceDefinition(*imported->sourceDefinition());
        REQUIRE(expectedSource);
        REQUIRE(actualSource);
        CHECK(QCborValue(*expectedSource).toCbor() == QCborValue(*actualSource).toCbor());
        CHECK(RS_Spline::geometryFingerprint(imported->getData()) ==
              RS_Spline::geometryFingerprint(expectedCache));
    }
}

TEST_CASE("fit-point spline-points source imports through operation-8 validation",
          "[lc-offsetcurve][import-session][typed-source][fit-points]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-fit-points-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    std::vector<LC_SplinePointsData> fitVariants;
    fitVariants.emplace_back(false, false);
    fitVariants.back().splinePoints = {{0, 0}, {1, 2}, {2, 0}};
    fitVariants.emplace_back(false, false);
    fitVariants.back().splinePoints = {{0, 0}, {1, 1.5}, {2, 1.5}, {3, 0}};
    fitVariants.emplace_back(false, false);
    fitVariants.back().splinePoints = {{0, 0}, {1, 1.5}, {2, 2}, {3, 1.5}, {4, 0}};
    fitVariants.emplace_back(true, false);
    fitVariants.back().splinePoints = {{0, 0}, {2, 0}, {2, 2}, {0, 2}};
    fitVariants.emplace_back(true, false);
    fitVariants.back().splinePoints = {{2, 0}, {1.4142135623730951, 1.4142135623730951},
        {0, 2}, {-1.4142135623730951, 1.4142135623730951}, {-2, 0},
        {-1.4142135623730951, -1.4142135623730951}, {0, -2},
        {1.4142135623730951, -1.4142135623730951}};
    fitVariants.emplace_back(false, false);
    fitVariants.back().splinePoints = {{0, 0}, {1, 2}, {3, 1.5}, {4, 0}};
    fitVariants.back().fitTangentsConstrained = true;
    fitVariants.back().fitTolerance = 1e-10;

    for (size_t variant = 0; variant < fitVariants.size(); ++variant) {
      std::vector<double> magnitudes{.1, .25, .4};
      if (variant == 2) magnitudes.push_back(.5);
      for (const double magnitude : magnitudes) {
      for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left,
                                            LC_CurveOffsetSide::Right}) {
        CAPTURE(variant, magnitude, side);
        const double signedDistance = side == LC_CurveOffsetSide::Left ? magnitude : -magnitude;
        const LC_OffsetSourceDefinition definition{
            LC_OffsetSourceGeometry{fitVariants[variant]}, signedDistance, 1e-4, false};
        auto source = definition.source.makeEntity();
        REQUIRE(source);
        const auto options = LC_CurveOffset::makeDirectOptions(*source, magnitude,
            definition.relativeTolerance);
        const auto generated = LC_CurveOffset::buildDirectBranches(*source,
            LC_CurveOffset::makeSideRequest(side, magnitude), options,
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
        auto materialized = LC_CurveOffset::materializeBranches(*source, generated, options,
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
        REQUIRE_FALSE(materialized.entities.empty());
        const bool singleCache = materialized.entities.size() == 1;
        const auto* expectedSpline = dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
        REQUIRE(expectedSpline);
        const RS_SplineData expectedCache = expectedSpline->getData();
        REQUIRE(expectedSpline->validate());

        for (const int format : {0, 1, 2, 3, 4}) {
            const bool dwg = format == 2 || format == 3;
            const bool r14 = format == 1;
            const bool r2000 = format == 3;
            const bool r2000Dxf = format == 4;
#ifndef DWGSUPPORT
            if (dwg) continue;
#endif
            xd::Definition carrier;
            carrier.source = definition;
            xd::Options encoding;
            if (dwg) {
                encoding.eed = true;
                if (r2000) {
                    encoding.textEncoding = xd::TextEncoding::Legacy;
                    encoding.legacyEncodedBytes = [](std::string_view text)
                        -> std::optional<std::size_t> { return text.size(); };
                } else {
                    encoding.textEncoding = xd::TextEncoding::Utf16;
                }
            }
            else encoding.asciiDxf = false;
            const auto tags = xd::forExport(
                {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))}, carrier, encoding);
            REQUIRE(tags);
            DRW_Spline cache = f::raw(1, *tags);
            cache.controllist.clear();
            for (const auto& point : expectedCache.controlPoints)
                cache.controllist.push_back(std::make_shared<DRW_Coord>(point.x, point.y, 0));
            cache.knotslist = expectedCache.knotslist;
            cache.weightlist = expectedCache.weights;
            cache.degree = expectedCache.degree;
            cache.flags = 8;
            cache.m_scenario = 1;
            cache.ncontrol = cache.controllist.size();
            cache.nknots = cache.knotslist.size();
            cache.nfit = 0;

            const std::string suffix = std::to_string(variant + 3);
            const std::string extension = dwg ? ".dwg" : ".dxf";
            const std::string version = r14 ? "-r14" :
                (r2000Dxf ? "-r2000-dxf" :
                    (r2000 ? "-r2000-dwg" : (dwg ? "-dwg" : "-r2018")));
            const std::string sideName = side == LC_CurveOffsetSide::Left ? "-left" : "-right";
            const std::string distanceName = "-" +
                std::to_string(static_cast<int>(std::lround(magnitude * 100.0)));
            const auto input = directory.file("fit-points-" + suffix + sideName + distanceName + version + extension);
#ifdef DWGSUPPORT
            if (dwg) f::dwgFile(input, cache, r2000 ? DRW::AC1015 : DRW::AC1032);
            else f::binaryDxf(input, cache,
                r2000Dxf ? DRW::AC1015 : (r14 ? DRW::AC1014 : DRW::AC1032));
#else
            f::binaryDxf(input, cache,
                r2000Dxf ? DRW::AC1015 : (r14 ? DRW::AC1014 : DRW::AC1032));
#endif
            const Lane lane = dwg ? (r2000 ? Lane::Eed2000 : Lane::Eed18)
                                  : (r14 ? Lane::Binary14 :
                                      (r2000Dxf ? Lane::Binary2000 : Lane::Binary18));
            const auto outcome = stagedImport(input, lane);
            REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
            REQUIRE(outcome.graphic);
            CHECK(outcome.admitted == (singleCache ? 1 : 0));
            CHECK(outcome.downgraded == (singleCache ? 0 : 1));
            const auto* importedEntity = f::sole(*outcome.graphic);
            REQUIRE(importedEntity);
            const auto* imported = dynamic_cast<const LC_OffsetCurve*>(importedEntity);
            const RS_SplineData* importedSpline = nullptr;
            const auto checkFitMetadata = [&](const LC_OffsetSourceDefinition& received) {
                const auto* fitSource = std::get_if<LC_SplinePointsData>(
                    &received.source.value);
                REQUIRE(fitSource);
                CHECK(fitSource->fitTangentsConstrained ==
                      fitVariants[variant].fitTangentsConstrained);
                CHECK(fitSource->fitTolerance == fitVariants[variant].fitTolerance);
            };
            if (singleCache) {
                REQUIRE(imported);
                REQUIRE(imported->sourceDefinition());
                importedSpline = &imported->getData();
                checkFitMetadata(*imported->sourceDefinition());
            } else {
                CHECK_FALSE(imported);
                const auto* fallbackSpline = dynamic_cast<const RS_Spline*>(importedEntity);
                REQUIRE(fallbackSpline);
                importedSpline = &fallbackSpline->getData();
            }
            const auto expectedSource = values::encodeSourceDefinition(definition);
            auto actualSource = expectedSource;
            if (singleCache) {
                actualSource = values::encodeSourceDefinition(*imported->sourceDefinition());
            } else {
                const auto preserved = xd::parseAndStrip(importedEntity->getDrwExtData(), encoding);
                REQUIRE(preserved.status == xd::Status::Schema2);
                REQUIRE(preserved.definition);
                REQUIRE(preserved.definition->source);
                checkFitMetadata(*preserved.definition->source);
                actualSource = values::encodeSourceDefinition(*preserved.definition->source);
            }
            REQUIRE(expectedSource);
            REQUIRE(actualSource);
            CHECK(QCborValue(*expectedSource).toCbor() == QCborValue(*actualSource).toCbor());
            CHECK(RS_Spline::geometryFingerprint(*importedSpline) ==
                  RS_Spline::geometryFingerprint(expectedCache));
        }
      }
      }
    }
}

TEST_CASE("Staged offset import cancellation after helper start suppresses delivery",
          "[lc-offsetcurve][import-session][worker][lifetime][asan]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-active-cancel");
    const auto input = directory.file("input.dxf"); writeInput(input, Lane::Ascii, f::raw(3));
    LC_OffsetImportSession session; bool started = false, delivered = false;
    auto& worker = LC_OffsetImportSessionTestAccess::worker(session);
    LC_OffsetWorkerClientTestAccess::onStarted(worker, [&](QProcess& process) { started = process.processId() > 0; session.cancel(); });
    REQUIRE(session.start(QString::fromStdString(input.string()), RS2::FormatDXFRW2018,
        RS2::DxfTextEncoding::Declared, false, [&](LC_OffsetImportSession::Outcome) { delivered = true; }));
    QElapsedTimer elapsed; elapsed.start();
    while (!started || worker.busy()) {
        REQUIRE(elapsed.elapsed() < 5000); QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1);
    }
    CHECK_FALSE(session.busy()); CHECK_FALSE(delivered); CHECK_FALSE(LC_OffsetWorkerClientTestAccess::hasStartHook(worker));
    std::optional<LC_OffsetImportSession::Outcome> retry;
    REQUIRE(session.start(QString::fromStdString(input.string()), RS2::FormatDXFRW2018,
        RS2::DxfTextEncoding::Declared, false, [&](LC_OffsetImportSession::Outcome result) { retry.emplace(std::move(result)); }));
    elapsed.restart();
    while (!retry) { REQUIRE(elapsed.elapsed() < 35000); QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
    REQUIRE(retry->status == LC_OffsetImportSession::Status::Complete); REQUIRE(retry->graphic);
    CHECK(retry->admitted == 1); CHECK_FALSE(delivered);
}

TEST_CASE("Staged offset import requires explicit partial-read acceptance",
          "[lc-offsetcurve][import-session][partial]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-partial");
    const auto input = directory.file("input.dxf");
    auto text = f::ascii(f::raw(3));
    const auto end = text.rfind("0\nENDSEC\n0\nEOF\n"); REQUIRE(end != std::string::npos);
    text.replace(end, std::string::npos, "0\nLINE\n8\n0\n10\n0.0\n"); f::put(input, text);
    auto refused = stagedImport(input, Lane::Ascii);
    CHECK(refused.status == LC_OffsetImportSession::Status::ReadFailed); CHECK_FALSE(refused.graphic);
    auto accepted = stagedImport(input, Lane::Ascii, true);
    REQUIRE(accepted.status == LC_OffsetImportSession::Status::Complete); REQUIRE(accepted.graphic);
    CHECK(accepted.partial); CHECK(accepted.admitted == 1); CHECK(accepted.downgraded == 0);
    CHECK(accepted.graphic->getFormatType() == RS2::FormatUnknown);
    CHECK(f::native(*accepted.graphic, 3) != nullptr);
}

TEST_CASE("Staged offset import asks for partial-read acceptance without reparsing",
          "[lc-offsetcurve][import-session][partial][lifetime]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-decision");
    const auto input = directory.file("input.dxf");
    auto text = f::ascii(f::raw(3));
    const auto end = text.rfind("0\nENDSEC\n0\nEOF\n"); REQUIRE(end != std::string::npos);
    text.replace(end, std::string::npos, "0\nLINE\n8\n0\n10\n0.0\n"); f::put(input, text);
    auto session = std::make_unique<LC_OffsetImportSession>();
    std::optional<LC_OffsetImportSession::Outcome> outcome;
    bool asked = false;
    bool accept = false, cancel = false, destroy = false;
    SECTION("accept") { accept = true; }
    SECTION("reject") {}
    SECTION("cancel during decision") { cancel = true; }
    SECTION("destroy during decision") { destroy = true; }
    REQUIRE(session->start(QString::fromStdString(input.string()), format(Lane::Ascii), RS2::DxfTextEncoding::Declared,
        [&](bool hasEntities, const QString&) {
            asked = true; CHECK(hasEntities);
            if (accept) CHECK(std::filesystem::remove(input));
            if (cancel) session->cancel();
            if (destroy) session.reset();
            return accept;
        }, [&](LC_OffsetImportSession::Outcome result) { outcome.emplace(std::move(result)); }));
    QElapsedTimer elapsed; elapsed.start();
    while (!asked || (!cancel && !destroy && !outcome)) {
        REQUIRE(elapsed.elapsed() < 35000); QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1);
    }
    if (cancel || destroy) {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        CHECK_FALSE(outcome); if (session) CHECK_FALSE(session->busy());
    } else if (accept) {
        REQUIRE(outcome->status == LC_OffsetImportSession::Status::Complete); REQUIRE(outcome->graphic);
        CHECK(outcome->partial); CHECK(outcome->graphic->getFormatType() == RS2::FormatUnknown);
        CHECK(outcome->sourceUnverified);
        CHECK(f::native(*outcome->graphic, 3) != nullptr);
    } else {
        CHECK(outcome->status == LC_OffsetImportSession::Status::ReadFailed); CHECK_FALSE(outcome->graphic);
    }
}

TEST_CASE("Hatch pattern import rejects partial DXF and keeps ordinary native display cache",
          "[lc-offsetcurve][hatch-pattern][hostile]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-pattern-import");
    const auto complete = f::ascii(f::raw(3));

    const auto validPath = directory.file("valid.dxf");
    f::put(validPath, complete);
    RS_Graphic drawing;
    REQUIRE(lc::test::importDrawing(drawing, validPath, RS2::FormatDXFRW2018));
    REQUIRE(f::native(drawing, 3));

    RS_Pattern valid(lc::test::toQString(validPath));
    REQUIRE(valid.loadPattern());
    CHECK_FALSE(valid.empty());
    bool keptSpline = false;
    for (const auto* entity : valid) {
        REQUIRE(entity);
        CHECK((entity->rtti() == RS2::EntityLine || entity->rtti() == RS2::EntityArc ||
               entity->rtti() == RS2::EntityEllipse || entity->rtti() == RS2::EntitySpline));
        if (entity->rtti() == RS2::EntitySpline) {
            keptSpline = true;
            CHECK(dynamic_cast<const LC_OffsetCurve*>(entity) == nullptr);
            CHECK(f::markerCount(entity->getDrwExtData()) == 0);
        }
    }
    CHECK(keptSpline);

    auto partialBytes = complete;
    const auto end = partialBytes.rfind("0\nENDSEC\n0\nEOF\n");
    REQUIRE(end != std::string::npos);
    partialBytes.replace(end, std::string::npos, "0\nLINE\n8\n0\n10\n0.0\n");
    const auto partialPath = directory.file("partial.dxf");
    f::put(partialPath, partialBytes);
    RS_Pattern partial(lc::test::toQString(partialPath));
    CHECK_FALSE(partial.loadPattern());
    CHECK(partial.empty());
}

TEST_CASE("Staged offset import waits for a one-shot asynchronous partial decision",
          "[lc-offsetcurve][import-session][partial][lifetime][async]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-async-decision");
    const auto input = directory.file("input.dxf");
    auto text = f::ascii(f::raw(3));
    const auto end = text.rfind("0\nENDSEC\n0\nEOF\n"); REQUIRE(end != std::string::npos);
    text.replace(end, std::string::npos, "0\nLINE\n8\n0\n10\n0.0\n"); f::put(input, text);
    auto session = std::make_unique<LC_OffsetImportSession>();
    LC_OffsetImportSession::PartialReply reply;
    std::optional<LC_OffsetImportSession::Outcome> outcome;
    int deliveries = 0, ticks = 0;
    REQUIRE(session->startWithDecision(QString::fromStdString(input.string()), format(Lane::Ascii),
        RS2::DxfTextEncoding::Declared,
        [&](bool hasEntities, const QString&, LC_OffsetImportSession::PartialReply decide) {
            CHECK(hasEntities); reply = std::move(decide);
        }, [&](LC_OffsetImportSession::Outcome result) { ++deliveries; outcome.emplace(std::move(result)); }));
    QElapsedTimer elapsed; elapsed.start();
    while (!reply) { REQUIRE(elapsed.elapsed() < 5000); QCoreApplication::processEvents(QEventLoop::AllEvents); }
    REQUIRE(session->busy()); CHECK_FALSE(outcome);
    QTimer::singleShot(0, [&] { ++ticks; });
    QCoreApplication::processEvents(QEventLoop::AllEvents); CHECK(ticks == 1); CHECK_FALSE(outcome);
    SECTION("accept once without reparsing") {
        REQUIRE(std::filesystem::remove(input)); reply(true); reply(false);
        while (!outcome) { REQUIRE(elapsed.elapsed() < 35000); QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
        REQUIRE(outcome->status == LC_OffsetImportSession::Status::Complete); REQUIRE(outcome->graphic);
        CHECK(outcome->graphic->getFormatType() == RS2::FormatUnknown);
        CHECK(f::native(*outcome->graphic, 3) != nullptr); CHECK(deliveries == 1);
    }
    SECTION("reject once") {
        reply(false); reply(true); REQUIRE(outcome);
        CHECK(outcome->status == LC_OffsetImportSession::Status::ReadFailed); CHECK_FALSE(outcome->graphic);
        CHECK(deliveries == 1);
    }
    SECTION("cancel then reply") {
        session->cancel(); reply(true); QCoreApplication::processEvents(QEventLoop::AllEvents);
        CHECK_FALSE(session->busy()); CHECK_FALSE(outcome); CHECK(deliveries == 0);
    }
    SECTION("stale reply cannot affect a new import") {
        session->cancel();
        REQUIRE(session->start(QString::fromStdString(input.string()), format(Lane::Ascii),
            RS2::DxfTextEncoding::Declared, true,
            [&](LC_OffsetImportSession::Outcome result) { ++deliveries; outcome.emplace(std::move(result)); }));
        reply(false);
        while (!outcome) { REQUIRE(elapsed.elapsed() < 35000); QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
        REQUIRE(outcome->status == LC_OffsetImportSession::Status::Complete); REQUIRE(outcome->graphic);
        CHECK(f::native(*outcome->graphic, 3) != nullptr); CHECK(deliveries == 1);
    }
    SECTION("destroy then reply") {
        session.reset(); reply(true); QCoreApplication::processEvents(QEventLoop::AllEvents);
        CHECK_FALSE(outcome); CHECK(deliveries == 0);
    }
}

TEST_CASE("A partial decision callback cannot revoke its own accepted reply by throwing",
          "[lc-offsetcurve][import-session][partial][async]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-decision-throw");
    const auto input = directory.file("input.dxf"); auto text = f::ascii(f::raw(3));
    const auto end = text.rfind("0\nENDSEC\n0\nEOF\n"); REQUIRE(end != std::string::npos);
    text.replace(end, std::string::npos, "0\nLINE\n8\n0\n10\n0.0\n"); f::put(input, text);
    LC_OffsetImportSession session; std::optional<LC_OffsetImportSession::Outcome> outcome;
    REQUIRE(session.startWithDecision(QString::fromStdString(input.string()), format(Lane::Ascii),
        RS2::DxfTextEncoding::Declared,
        [](bool, const QString&, LC_OffsetImportSession::PartialReply reply) {
            reply(true); throw std::runtime_error("after reply");
        }, [&](LC_OffsetImportSession::Outcome result) { outcome.emplace(std::move(result)); }));
    QElapsedTimer elapsed; elapsed.start();
    while (!outcome) { REQUIRE(elapsed.elapsed() < 35000); QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
    REQUIRE(outcome->status == LC_OffsetImportSession::Status::Complete); REQUIRE(outcome->graphic);
    CHECK(f::native(*outcome->graphic, 3) != nullptr);
}

TEST_CASE("Staged offset import refuses formats outside its DXF and DWG contract",
          "[lc-offsetcurve][import-session][refusal]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-format");
    const auto input = directory.file("input.dxf"); writeInput(input, Lane::Ascii, f::raw(3));
    LC_OffsetImportSession session; std::optional<LC_OffsetImportSession::Outcome> outcome;
    REQUIRE(session.start(QString::fromStdString(input.string()), RS2::FormatSHP,
        RS2::DxfTextEncoding::Declared, true, [&](LC_OffsetImportSession::Outcome result) { outcome.emplace(std::move(result)); }));
    QElapsedTimer elapsed; elapsed.start();
    while (!outcome) { REQUIRE(elapsed.elapsed() < 5000); QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
    CHECK(outcome->status == LC_OffsetImportSession::Status::ReadFailed); CHECK_FALSE(outcome->graphic);
    CHECK_FALSE(outcome->error.isEmpty()); CHECK_FALSE(outcome->partial);
}

TEST_CASE("Staged offset import follows XREF clones into the host block",
          "[lc-offsetcurve][import-session][xref][worker]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-xref");
    const auto external = directory.file("external.dxf"), host = directory.file("host.dxf");
    writeInput(external, Lane::Ascii, f::raw(3));
    auto prefix = f::ascii(f::raw(3));
    const auto entities = prefix.find("0\nSECTION\n2\nENTITIES\n"); REQUIRE(entities != std::string::npos);
    prefix.resize(entities);
    f::put(host, prefix + "0\nSECTION\n2\nBLOCKS\n0\nBLOCK\n5\nB1\n8\n0\n2\nREF\n70\n4\n1\nexternal.dxf\n"
        "10\n0\n20\n0\n30\n0\n0\nENDBLK\n5\nB2\n8\n0\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nINSERT\n5\nB3\n8\n0\n2\nREF\n10\n0\n20\n0\n30\n0\n"
        "0\nENDSEC\n0\nEOF\n");
    auto outcome = stagedImport(host, Lane::Ascii);
    INFO(outcome.error.toStdString());
    REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete); REQUIRE(outcome.graphic);
    CHECK(outcome.admitted == 1); CHECK(outcome.downgraded == 0);
    auto* block = outcome.graphic->findBlock("REF"); REQUIRE(block); REQUIRE(block->count() == 1);
    auto* native = dynamic_cast<LC_OffsetCurve*>(block->entityAt(0)); REQUIRE(native);
    CHECK(native->hasEditableDefinition()); CHECK(native->getParent() == block);
    REQUIRE(native->getLayer(false)); CHECK(native->getLayer(false)->getName() == "REF|FixtureOffsets");
    auto* insert = outcome.graphic->entityAt(0); REQUIRE(insert); REQUIRE(insert->isContainer());
    CHECK(static_cast<RS_EntityContainer*>(insert)->count() > 0);
}

TEST_CASE("Staged XREF parsing uses captured bytes and refuses a changed dependency",
          "[lc-offsetcurve][import-session][xref][snapshot]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-xref-snapshot");
    const auto external = directory.file("external.dxf"), host = directory.file("host.dxf");
    writeInput(external, Lane::Ascii, f::raw(3));
    writeXrefHost(host, {{"REF", "external.dxf"}});

    LC_OffsetImportSession session;
    LC_OffsetImportSessionTestAccess::afterXrefSnapshot(session, [&] {
        f::put(external, "0\nEOF\n");
    });
    bool parsedCapturedXref = false;
    LC_OffsetImportSessionTestAccess::afterSnapshotParse(session, [&](RS_Graphic& graphic) {
        const auto* block = graphic.findBlock("REF");
        parsedCapturedXref = block && block->count() == 1 &&
            dynamic_cast<const LC_OffsetImportPlaceholder*>(block->entityAt(0));
    });
    std::optional<LC_OffsetImportSession::Outcome> outcome;
    REQUIRE(session.start(QString::fromStdString(host.string()), RS2::FormatDXFRW2018,
        RS2::DxfTextEncoding::Declared, false,
        [&](LC_OffsetImportSession::Outcome result) { outcome.emplace(std::move(result)); }));
    QElapsedTimer elapsed; elapsed.start();
    while (!outcome) {
        REQUIRE(elapsed.elapsed() < 10000);
        QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1);
    }
    CHECK(parsedCapturedXref);
    CHECK(outcome->status == LC_OffsetImportSession::Status::SourceChanged);
    CHECK_FALSE(outcome->graphic);
}

TEST_CASE("Staged XREF admission fails closed for missing and oversized dependencies",
          "[lc-offsetcurve][import-session][xref][snapshot][limit]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-xref-admission");
    const auto host = directory.file("host.dxf");
    const auto run = [&](const std::filesystem::path& source) {
        LC_OffsetImportSession session;
        std::optional<LC_OffsetImportSession::Outcome> outcome;
        REQUIRE(session.start(QString::fromStdString(source.string()), RS2::FormatDXFRW2018,
            RS2::DxfTextEncoding::Declared, false,
            [&](LC_OffsetImportSession::Outcome result) { outcome.emplace(std::move(result)); }));
        QElapsedTimer elapsed; elapsed.start();
        while (!outcome) {
            REQUIRE(elapsed.elapsed() < 10000);
            QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1);
        }
        return std::move(*outcome);
    };

    SECTION("missing dependency") {
        writeXrefHost(host, {{"MISSING", "not-present.dxf"}});
        const auto outcome = run(host);
        CHECK(outcome.status == LC_OffsetImportSession::Status::ReadFailed);
        CHECK_FALSE(outcome.graphic);
    }
    SECTION("dependency exceeds the aggregate byte budget") {
        const auto external = directory.file("oversized.dxf");
        QFile sparse(QString::fromStdString(external.string()));
        REQUIRE(sparse.open(QIODevice::WriteOnly));
        REQUIRE(sparse.resize(qint64{256} * 1024 * 1024));
        sparse.close();
        writeXrefHost(host, {{"OVERSIZED", "oversized.dxf"}});
        const auto outcome = run(host);
        CHECK(outcome.status == LC_OffsetImportSession::Status::ResourceLimit);
        CHECK_FALSE(outcome.graphic);
    }
}

TEST_CASE("Staged XREF snapshots parse DWG dependencies from captured bytes",
          "[lc-offsetcurve][import-session][xref][dwg]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-dwg-xref-snapshot");
    const auto external = directory.file("external.dwg"), host = directory.file("host.dxf");
    writeInput(external, Lane::Eed18, f::raw(3));
    writeXrefHost(host, {{"DWG_REF", "external.dwg"}});
    const auto outcome = stagedImport(host, Lane::Ascii);
    REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
    REQUIRE(outcome.graphic);
    const auto* block = outcome.graphic->findBlock("DWG_REF");
    REQUIRE(block);
    REQUIRE(block->count() == 1);
    CHECK(dynamic_cast<const LC_OffsetCurve*>(block->entityAt(0)) != nullptr);
}

TEST_CASE("Staged XREF snapshots deduplicate paths and cap distinct dependencies",
          "[lc-offsetcurve][import-session][xref][snapshot][limit]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-xref-bounds");
    SECTION("repeated references share one captured byte buffer") {
        const auto external = directory.file("external.dxf"), host = directory.file("host.dxf");
        writeInput(external, Lane::Ascii, f::raw(3));
        writeXrefHost(host, {{"REF1", "external.dxf"}, {"REF2", "external.dxf"}});
        LC_OffsetImportSession session; std::size_t captures = 0;
        LC_OffsetImportSessionTestAccess::afterXrefSnapshot(session, [&] { ++captures; });
        std::optional<LC_OffsetImportSession::Outcome> outcome;
        REQUIRE(session.start(QString::fromStdString(host.string()), RS2::FormatDXFRW2018,
            RS2::DxfTextEncoding::Declared, false,
            [&](LC_OffsetImportSession::Outcome result) { outcome.emplace(std::move(result)); }));
        QElapsedTimer elapsed; elapsed.start();
        while (!outcome) {
            REQUIRE(elapsed.elapsed() < 35000);
            QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1);
        }
        REQUIRE(outcome->status == LC_OffsetImportSession::Status::Complete); REQUIRE(outcome->graphic);
        CHECK(captures == 1);
        for (const auto* name : {"REF1", "REF2"}) {
            const auto* block = outcome->graphic->findBlock(name); REQUIRE(block); REQUIRE(block->count() == 1);
            CHECK(dynamic_cast<const LC_OffsetCurve*>(block->entityAt(0)) != nullptr);
        }
    }
    SECTION("the sixty-fifth distinct dependency is rejected before publication") {
        std::vector<std::pair<std::string, std::string>> refs;
        refs.reserve(65);
        for (std::size_t i = 0; i < 65; ++i) {
            const auto name = "external-" + std::to_string(i) + ".dxf";
            writeInput(directory.file(name), Lane::Ascii, f::raw(3));
            refs.emplace_back("REF" + std::to_string(i), name);
        }
        const auto host = directory.file("host.dxf"); writeXrefHost(host, refs);
        LC_OffsetImportSession session; std::size_t captures = 0;
        LC_OffsetImportSessionTestAccess::afterXrefSnapshot(session, [&] { ++captures; });
        std::optional<LC_OffsetImportSession::Outcome> outcome;
        REQUIRE(session.start(QString::fromStdString(host.string()), RS2::FormatDXFRW2018,
            RS2::DxfTextEncoding::Declared, false,
            [&](LC_OffsetImportSession::Outcome result) { outcome.emplace(std::move(result)); }));
        QElapsedTimer elapsed; elapsed.start();
        while (!outcome) {
            REQUIRE(elapsed.elapsed() < 15000);
            QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1);
        }
        CHECK(outcome->status == LC_OffsetImportSession::Status::ResourceLimit);
        CHECK_FALSE(outcome->graphic); CHECK(captures == 64);
    }
}

TEST_CASE("Offset filter imports DXF and DWG streams using a logical source path",
          "[lc-offsetcurve][import-session][stream]") {
    Setup setup; lc::test::OutputDirectory directory("offset-filter-stream");
    unsigned index = 0;
    for (const auto lane : lanes()) {
        CAPTURE(static_cast<int>(lane));
        const std::string stem = std::to_string(index++);
        const auto source = directory.file(stem + (isDwg(lane) ? "-source.dwg" : "-source.dxf"));
        const auto logical = directory.file(stem + (isDwg(lane) ? "-logical.dwg" : "-logical.dxf"));
        writeInput(source, lane, f::raw(3));
        const auto bytes = lc::test::fileBytes(source); REQUIRE_FALSE(bytes.empty());
        CHECK_FALSE(std::filesystem::exists(logical));
        const std::string prefix("prefix\0", 7);
        std::istringstream input(prefix + bytes, std::ios::in | std::ios::binary);
        input.seekg(static_cast<std::streamoff>(prefix.size())); REQUIRE(input.good());
        RS_Graphic graphic; graphic.initForNewDocument(); RS_FilterDXFRW filter;
        REQUIRE(filter.fileImportStream(graphic, QString::fromStdString(logical.string()), format(lane), input));
        CHECK(f::native(graphic, 3) != nullptr);
    }
}

TEST_CASE("Offset filter resolves stream XREFs relative to the logical host path",
          "[lc-offsetcurve][import-session][stream][xref]") {
    Setup setup; lc::test::OutputDirectory directory("offset-filter-stream-xref");
    const auto external = directory.file("external.dxf"), logical = directory.file("unwritten-host.dxf");
    writeInput(external, Lane::Ascii, f::raw(3));
    auto text = f::ascii(f::raw(3));
    const auto entities = text.find("0\nSECTION\n2\nENTITIES\n"); REQUIRE(entities != std::string::npos);
    text.resize(entities);
    text += "0\nSECTION\n2\nBLOCKS\n0\nBLOCK\n5\nB1\n8\n0\n2\nREF\n70\n4\n1\nexternal.dxf\n"
        "10\n0\n20\n0\n30\n0\n0\nENDBLK\n5\nB2\n8\n0\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nINSERT\n5\nB3\n8\n0\n2\nREF\n10\n0\n20\n0\n30\n0\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(std::filesystem::exists(logical));
    std::istringstream input(text, std::ios::in | std::ios::binary);
    RS_Graphic graphic; graphic.initForNewDocument(); RS_FilterDXFRW filter;
    REQUIRE(filter.fileImportStream(graphic, QString::fromStdString(logical.string()), RS2::FormatDXFRW2018, input));
    auto* block = graphic.findBlock("REF"); REQUIRE(block); REQUIRE(block->count() == 1);
    auto* native = dynamic_cast<LC_OffsetCurve*>(block->entityAt(0)); REQUIRE(native);
    CHECK(native->hasEditableDefinition()); CHECK(native->getParent() == block);
    REQUIRE(native->getLayer(false)); CHECK(native->getLayer(false)->getName() == "REF|FixtureOffsets");
}

TEST_CASE("Staged offset import preserves carrier order around ordinary entities",
          "[lc-offsetcurve][import-session][worker][order]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-order");
    const auto input = directory.file("input.dxf");
    auto text = f::ascii(f::raw(3));
    const auto first = text.find("0\nSPLINE\n"); const auto end = text.rfind("0\nENDSEC\n0\nEOF\n");
    REQUIRE(first != std::string::npos); REQUIRE(end != std::string::npos); REQUIRE(end > first);
    auto second = text.substr(first, end - first);
    const auto handle = second.find("5\nA1\n"); REQUIRE(handle != std::string::npos);
    second.replace(handle, 5, "5\nA2\n");
    text.insert(end, "0\nLINE\n5\nA3\n8\n0\n10\n0\n20\n0\n11\n1\n21\n0\n" + second);
    f::put(input, text);
    auto outcome = stagedImport(input, Lane::Ascii);
    REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete); REQUIRE(outcome.graphic);
    CHECK(outcome.admitted == 2); CHECK(outcome.downgraded == 0); REQUIRE(outcome.graphic->count() == 3);
    CHECK(dynamic_cast<LC_OffsetCurve*>(outcome.graphic->entityAt(0)) != nullptr);
    CHECK(outcome.graphic->entityAt(1)->rtti() == RS2::EntityLine);
    CHECK(dynamic_cast<LC_OffsetCurve*>(outcome.graphic->entityAt(2)) != nullptr);
}

#ifdef Q_OS_UNIX
TEST_CASE("Staged offset import waits for a timed-out helper to be reaped before delivery",
          "[lc-offsetcurve][import-session][worker][timeout][asan]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-timeout");
    const auto input = directory.file("input.dxf"); writeInput(input, Lane::Ascii, f::raw(3));
    LC_OffsetImportSession session; auto& worker = LC_OffsetImportSessionTestAccess::worker(session);
    qint64 child = 0; bool stopped = false, reapedAtDelivery = false;
    LC_OffsetWorkerClientTestAccess::onStarted(worker, [&](QProcess& process) {
        child = process.processId(); stopped = ::kill(static_cast<pid_t>(child), SIGSTOP) == 0;
        LC_OffsetImportSessionTestAccess::deadline(session).start(500);
    });
    std::optional<LC_OffsetImportSession::Outcome> outcome;
    REQUIRE(session.start(QString::fromStdString(input.string()), RS2::FormatDXFRW2018,
        RS2::DxfTextEncoding::Declared, false, [&](LC_OffsetImportSession::Outcome result) {
            errno = 0; reapedAtDelivery = worker.healthyAndIdle() && ::kill(static_cast<pid_t>(child), 0) == -1 && errno == ESRCH;
            outcome.emplace(std::move(result));
        }));
    QElapsedTimer elapsed; elapsed.start();
    while (!outcome) { REQUIRE(elapsed.elapsed() < 5000); QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
    REQUIRE(child > 0); REQUIRE(stopped); REQUIRE(reapedAtDelivery);
    REQUIRE(outcome->status == LC_OffsetImportSession::Status::Complete); REQUIRE(outcome->graphic);
    CHECK(outcome->admitted == 0); CHECK(outcome->downgraded == 1);
    REQUIRE(outcome->diagnostics.size() == 1);
    CHECK(outcome->diagnostics.front().kind == LC_OffsetImportDiagnostic::Kind::Timeout);
    auto* entity = f::sole(*outcome->graphic); REQUIRE(entity);
    CHECK(dynamic_cast<LC_OffsetImportPlaceholder*>(entity) == nullptr);
}

TEST_CASE("Staged offset import owner destruction reaps an active stopped helper",
          "[lc-offsetcurve][import-session][worker][lifetime][asan]") {
    Setup setup; lc::test::OutputDirectory directory("offset-staged-owner-death");
    const auto input = directory.file("input.dxf"); writeInput(input, Lane::Ascii, f::raw(3));
    auto session = std::make_unique<LC_OffsetImportSession>();
    qint64 child = 0; bool stopped = false, delivered = false;
    LC_OffsetWorkerClientTestAccess::onStarted(LC_OffsetImportSessionTestAccess::worker(*session),
        [&](QProcess& process) {
            child = process.processId(); stopped = ::kill(static_cast<pid_t>(child), SIGSTOP) == 0;
            QTimer::singleShot(0, [&] { session.reset(); });
        });
    REQUIRE(session->start(QString::fromStdString(input.string()), RS2::FormatDXFRW2018,
        RS2::DxfTextEncoding::Declared, false, [&](LC_OffsetImportSession::Outcome) { delivered = true; }));
    QElapsedTimer elapsed; elapsed.start();
    while (session) { REQUIRE(elapsed.elapsed() < 5000); QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
    REQUIRE(child > 0); REQUIRE(stopped); CHECK_FALSE(delivered);
    errno = 0; CHECK(::kill(static_cast<pid_t>(child), 0) == -1); CHECK(errno == ESRCH);
    elapsed.restart(); while (elapsed.elapsed() < 100) { QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
    CHECK_FALSE(delivered);
}
#endif
