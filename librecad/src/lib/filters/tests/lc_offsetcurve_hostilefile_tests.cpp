/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <array>
#include <limits>
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

enum class Lane { Ascii, Binary14, Binary18, Eed2000, Eed18 };
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
    return lane == Lane::Binary14 ? RS2::FormatDXFRW14 : RS2::FormatDXFRW2018;
}
void writeInput(const std::filesystem::path& path, Lane lane, const DRW_Spline& raw) {
    switch (lane) {
    case Lane::Ascii: f::put(path, f::ascii(raw)); return;
    case Lane::Binary14: f::binaryDxf(path, raw, DRW::AC1014); return;
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
    QFile file(QString::fromStdString(path.string())); REQUIRE(file.open(QIODevice::ReadWrite));
    REQUIRE(stamp.matchesDescriptor(file.handle()));
    REQUIRE(file.write("X", 1) == 1); REQUIRE(file.flush());
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
