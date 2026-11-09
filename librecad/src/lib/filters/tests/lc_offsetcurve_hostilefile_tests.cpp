/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/catch_approx.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "lc_offsetcurve_persistence_fixture.h"
#include "../../offset_worker/lc_offsetworkertolerance.h"
#include "../../offset_worker/lc_offsetworkerclient.h"
#include "../../offset_worker/lc_offsetworkersourcecache.h"
#include "lc_offsetimportsession.h"
#include "intern/dwg_dxf_output_transaction.h"
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QProcess>
#include <QStandardPaths>
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

void preserveTypedSourceDwg(const std::filesystem::path& source,
                            const std::string& fileName) {
    const char* directory = std::getenv("LIBRECAD_TYPED_SOURCE_DWG_DIR");
    if (directory == nullptr || *directory == '\0') return;
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    REQUIRE_FALSE(error);
    const bool copied = std::filesystem::copy_file(source,
        std::filesystem::path(directory) / fileName,
        std::filesystem::copy_options::overwrite_existing, error);
    REQUIRE(copied);
    REQUIRE_FALSE(error);
}

std::optional<double> sampledSplineDeviation(const RS_SplineData& expected,
                                             const DRW_Spline& converted,
                                             const int intervals) {
    if (intervals <= 0) return std::nullopt;
    RS_SplineData convertedData{converted.degree, false};
    for (const auto& control : converted.controllist) {
        if (!control) return std::nullopt;
        convertedData.controlPoints.emplace_back(control->x, control->y, control->z);
    }
    convertedData.knotslist = converted.knotslist;
    convertedData.weights = converted.weightlist;
    if (convertedData.weights.empty())
        convertedData.weights.assign(convertedData.controlPoints.size(), 1.0);

    RS_Spline expectedCurve{nullptr, expected};
    RS_Spline convertedCurve{nullptr, std::move(convertedData)};
    if (!expectedCurve.validate() || !convertedCurve.validate()) return std::nullopt;
    double expectedStart = 0.0;
    double expectedEnd = 0.0;
    double convertedStart = 0.0;
    double convertedEnd = 0.0;
    if (!expectedCurve.getParameterDomain(expectedStart, expectedEnd) ||
        !convertedCurve.getParameterDomain(convertedStart, convertedEnd) ||
        convertedStart != expectedStart || convertedEnd != expectedEnd)
        return std::nullopt;

    double maximumDeviation = 0.0;
    for (int sample = 0; sample <= intervals; ++sample) {
        const double fraction = static_cast<double>(sample) / intervals;
        const double parameter = expectedStart +
            fraction * (expectedEnd - expectedStart);
        const auto side = sample == intervals
            ? LC_CurveEvaluationSide::Left
            : LC_CurveEvaluationSide::Right;
        LC_CurveJet expectedJet;
        LC_CurveJet convertedJet;
        if (!expectedCurve.tryEvaluateJet(parameter, side, expectedJet) ||
            !convertedCurve.tryEvaluateJet(parameter, side, convertedJet))
            return std::nullopt;
        maximumDeviation = std::max(maximumDeviation,
                                    expectedJet.point.distanceTo(convertedJet.point));
    }
    return maximumDeviation;
}

double periodicSplineGeometryTolerance(const RS_SplineData& expected,
                                       const double modelScale) {
    double coordinateScale = 1.0;
    for (const RS_Vector& point : expected.controlPoints)
        coordinateScale = std::max({coordinateScale,
                                    std::abs(point.x), std::abs(point.y)});
    return 1e-9 * std::max(1.0, modelScale) +
           8.0 * std::numeric_limits<double>::epsilon() * coordinateScale;
}

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
    RS_SplineData rationalTangentCubic{3, false};
    rationalTangentCubic.controlPoints = {{0, 0}, {2, 3}, {6, 4}, {9, 0}};
    rationalTangentCubic.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    rationalTangentCubic.weights = {1.0, 0.75, 0.8, 1.0};
    rationalTangentCubic.fitTangentsConstrained = true;
    rationalTangentCubic.fitTolerance = 1e-2;
    rationalTangentCubic.fitStartTangent = {2, 3};
    rationalTangentCubic.fitEndTangent = {3, -4};
    RS_Spline rationalTangentEvaluator{nullptr, rationalTangentCubic};
    for (const double parameter : {0.0, 0.2, 0.5, 0.8, 1.0}) {
        LC_CurveJet jet;
        REQUIRE(rationalTangentEvaluator.tryEvaluateJet(parameter,
            parameter == 1.0 ? LC_CurveEvaluationSide::Left
                             : LC_CurveEvaluationSide::Right, jet));
        rationalTangentCubic.fitPoints.push_back(jet.point);
    }
    RS_Spline rationalTangentCheck{nullptr, rationalTangentCubic};
    REQUIRE(rationalTangentCheck.fitTangentsMatchCurve());
    RS_SplineData rationalTangentQuadratic{2, false};
    rationalTangentQuadratic.controlPoints = {{0, 0}, {2, 3}, {5, 0}};
    rationalTangentQuadratic.knotslist = {0, 0, 0, 1, 1, 1};
    rationalTangentQuadratic.weights = {1.0, 0.7, 1.0};
    rationalTangentQuadratic.fitTangentsConstrained = true;
    rationalTangentQuadratic.fitTolerance = 1e-2;
    rationalTangentQuadratic.fitStartTangent = {2, 3};
    rationalTangentQuadratic.fitEndTangent = {3, -3};
    RS_Spline rationalQuadraticEvaluator{nullptr, rationalTangentQuadratic};
    for (const double parameter : {0.0, 0.2, 0.5, 0.8, 1.0}) {
        LC_CurveJet jet;
        REQUIRE(rationalQuadraticEvaluator.tryEvaluateJet(parameter,
            parameter == 1.0 ? LC_CurveEvaluationSide::Left
                             : LC_CurveEvaluationSide::Right, jet));
        rationalTangentQuadratic.fitPoints.push_back(jet.point);
    }
    RS_Spline rationalQuadraticCheck{nullptr, rationalTangentQuadratic};
    REQUIRE(rationalQuadraticCheck.fitTangentsMatchCurve());
    RS_SplineData linearTangentSpline{1, false};
    linearTangentSpline.controlPoints = {{0, 0}, {4, 0}};
    linearTangentSpline.knotslist = {0, 0, 1, 1};
    linearTangentSpline.weights = {1.0, 0.5};
    linearTangentSpline.fitPoints = {{0, 0}, {2, 0}, {4, 0}};
    linearTangentSpline.fitTangentsConstrained = true;
    linearTangentSpline.fitTolerance = 1e-8;
    linearTangentSpline.fitStartTangent = {1, 0};
    linearTangentSpline.fitEndTangent = {2, 0};
    RS_Spline linearTangentCheck{nullptr, linearTangentSpline};
    REQUIRE(linearTangentCheck.fitTangentsMatchCurve());
    RS_SplineData oneEndedLinearTangentSpline = linearTangentSpline;
    oneEndedLinearTangentSpline.fitEndTangent = {0, 0};
    RS_Spline oneEndedLinearTangentCheck{nullptr, oneEndedLinearTangentSpline};
    REQUIRE(oneEndedLinearTangentCheck.fitTangentsMatchCurve());
    LC_SplinePointsData tangentSplinePointsData{false, false};
    tangentSplinePointsData.splinePoints = {
        {0, 0}, {1, 2}, {3, 1.5}, {4, 0}};
    LC_SplinePoints tangentSplinePointsEntity{nullptr,
                                               tangentSplinePointsData};
    LC_CurveJet tangentPointsStart;
    LC_CurveJet tangentPointsEnd;
    REQUIRE(tangentSplinePointsEntity.tryEvaluateJet(
        0.0, LC_CurveEvaluationSide::Right, tangentPointsStart));
    REQUIRE(tangentSplinePointsEntity.tryEvaluateJet(
        static_cast<double>(tangentSplinePointsEntity.getSegmentCount()),
        LC_CurveEvaluationSide::Left, tangentPointsEnd));
    tangentSplinePointsEntity.getData().fitTangentsConstrained = true;
    tangentSplinePointsEntity.getData().fitTolerance = 1e-7;
    tangentSplinePointsEntity.getData().fitStartTangent = tangentPointsStart.first;
    tangentSplinePointsEntity.getData().fitEndTangent = tangentPointsEnd.first;
    tangentSplinePointsData = tangentSplinePointsEntity.getData();
    REQUIRE(tangentSplinePointsEntity.fitTangentsMatchCurve());
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
    RS_Polyline mixedBulgePolyline;
    mixedBulgePolyline.addVertex({0, 0}, 0.2);
    mixedBulgePolyline.addVertex({8, 0}, -0.2);
    mixedBulgePolyline.addVertex({8, 6});
    mixedBulgePolyline.addVertex({0, 6});
    mixedBulgePolyline.setClosed(true);
    mixedBulgePolyline.endPolyline();
    const auto mixedBulgePolylineSource =
        LC_OffsetSourceGeometry::capture(mixedBulgePolyline);
    REQUIRE(mixedBulgePolylineSource);
    REQUIRE(mixedBulgePolylineSource->kind() ==
            LC_OffsetSourceGeometry::Kind::Polyline);
    const auto &mixedPolylineData =
        std::get<LC_OffsetPolylineData>(mixedBulgePolylineSource->value);
    REQUIRE(mixedPolylineData.closed);
    REQUIRE(mixedPolylineData.segments.size() == 4);
    REQUIRE(std::holds_alternative<RS_ArcData>(mixedPolylineData.segments[0]));
    REQUIRE(std::holds_alternative<RS_ArcData>(mixedPolylineData.segments[1]));
    REQUIRE(std::holds_alternative<RS_LineData>(mixedPolylineData.segments[2]));
    REQUIRE(std::holds_alternative<RS_LineData>(mixedPolylineData.segments[3]));
    const auto singleSpanRationalSpline = [](const std::size_t degree) {
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        for (std::size_t i = 0; i <= degree; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(degree);
            data.controlPoints.emplace_back(4.0 * t, 1.5 * t * (1.0 - t));
            data.weights.push_back(0.75 + 0.5 * t);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        return data;
    };
    const RS_SplineData quarticSpline = singleSpanRationalSpline(4);
    const RS_SplineData sexticSpline = singleSpanRationalSpline(6);
    const auto twoSpanRationalSpline = [](const std::size_t degree) {
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        for (std::size_t i = 0; i <= degree + 1; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(degree + 1);
            data.controlPoints.emplace_back(4.0 * t, 1.5 * t * (1.0 - t));
            data.weights.push_back(0.75 + 0.5 * t);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.push_back(.5);
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        return data;
    };
    const RS_SplineData quarticTwoSpanSpline = twoSpanRationalSpline(4);
    const RS_SplineData sexticTwoSpanSpline = twoSpanRationalSpline(6);
    const auto nonuniformRationalSpline = [](const std::size_t degree) {
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        for (std::size_t i = 0; i <= degree + 2; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(degree + 2);
            data.controlPoints.emplace_back(4.0 * t, 1.5 * t * (1.0 - t));
            data.weights.push_back(0.75 + 0.5 * t);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.insert(data.knotslist.end(), {.2, .7});
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        return data;
    };
    const RS_SplineData quarticNonuniformSpline = nonuniformRationalSpline(4);
    const RS_SplineData sexticNonuniformSpline = nonuniformRationalSpline(6);
    const auto repeatedKnotRationalSpline = [](const std::size_t degree) {
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        for (std::size_t i = 0; i <= degree + 2; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(degree + 2);
            data.controlPoints.emplace_back(4.0 * t, 1.5 * t * (1.0 - t));
            data.weights.push_back(0.75 + 0.5 * t);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.insert(data.knotslist.end(), 2, .4);
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        return data;
    };
    const RS_SplineData quarticRepeatedKnotSpline = repeatedKnotRationalSpline(4);
    const RS_SplineData sexticRepeatedKnotSpline = repeatedKnotRationalSpline(6);
    const auto asymmetricRationalSpline = [](const std::size_t degree) {
        constexpr std::array<double, 8> yValues{0.0, 1.5, -.4, 2.2, .3, 1.1, -1.0, .5};
        constexpr std::array<double, 8> weights{.7, 1.5, .6, 1.8, .85, 1.3, 2.0, .9};
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        for (std::size_t i = 0; i <= degree + 1; ++i) {
            data.controlPoints.emplace_back(.8 * static_cast<double>(i) + .04 * (i % 2),
                                            yValues[i]);
            data.weights.push_back(weights[i]);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.push_back(degree == 4 ? .3 : .62);
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        return data;
    };
    const RS_SplineData quarticAsymmetricSpline = asymmetricRationalSpline(4);
    const RS_SplineData sexticAsymmetricSpline = asymmetricRationalSpline(6);
    const std::array<LC_OffsetSourceGeometry, 44> sources{
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {2, 0}, .5, 0, 1, false}},
        LC_OffsetSourceGeometry{RS_EllipseData{{1, -2}, {2, 0}, 1.4, .3, 2.3, false}},
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {2, 0}, .2, 0, 2 * M_PI, false}},
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {2, 0}, .2, 0, 0, true}},
        LC_OffsetSourceGeometry{RS_EllipseData{{-3, 1}, {2, 1}, .8, 1.8, .1, true}},
        LC_OffsetSourceGeometry{splineSource},
        LC_OffsetSourceGeometry{nonuniformQuadratic},
        LC_OffsetSourceGeometry{repeatedKnotCubic},
        LC_OffsetSourceGeometry{unweightedSpline},
        LC_OffsetSourceGeometry{rationalTangentCubic},
        LC_OffsetSourceGeometry{rationalTangentQuadratic},
        LC_OffsetSourceGeometry{linearTangentSpline},
        LC_OffsetSourceGeometry{oneEndedLinearTangentSpline},
        LC_OffsetSourceGeometry{quarticSpline},
        LC_OffsetSourceGeometry{sexticSpline},
        LC_OffsetSourceGeometry{quarticTwoSpanSpline},
        LC_OffsetSourceGeometry{sexticTwoSpanSpline},
        LC_OffsetSourceGeometry{quarticNonuniformSpline},
        LC_OffsetSourceGeometry{sexticNonuniformSpline},
        LC_OffsetSourceGeometry{quarticRepeatedKnotSpline},
        LC_OffsetSourceGeometry{sexticRepeatedKnotSpline},
        LC_OffsetSourceGeometry{quarticAsymmetricSpline},
        LC_OffsetSourceGeometry{sexticAsymmetricSpline},
        LC_OffsetSourceGeometry{wrappedSpline}, LC_OffsetSourceGeometry{splinePoints},
        LC_OffsetSourceGeometry{tangentSplinePointsData},
        LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{0, 1}, RS_Vector{1, 0}, RS_Vector{2, 1}}}},
        LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{2, 1}, RS_Vector{1, 0}, RS_Vector{0, 1}}}},
        LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{3, 4}, RS_Vector{-7, 9}, RS_Vector{3, 14}}}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {2, 0}, .5, -.5, .5}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .6, 1.5, -1.2, false}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .6, -.4, .4, true}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, true}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .6, .2, 1.2, false}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, -.2, false}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, false}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, true}},
        LC_OffsetSourceGeometry{RS_LineData{{0, 0}, {2, 0}}},
        LC_OffsetSourceGeometry{RS_ArcData{{0, 0}, 2, 0, 1, false}},
        LC_OffsetSourceGeometry{RS_CircleData{{0, 0}, 2}},
        LC_OffsetSourceGeometry{LC_OffsetPolylineData{{0, 0}, {2, 0}, false,
            {RS_LineData{{0, 0}, {2, 0}}}}},
        LC_OffsetSourceGeometry{closedArcPolyline},
        *mixedBulgePolylineSource
    };
    const auto app = std::make_shared<DRW_Variant>(1001, std::string("LibreCad"));
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
    for (std::size_t i = 0; i < sources.size(); ++i) {
        CAPTURE(i);
        const bool tangentSource = std::visit([](const auto& data) {
            using T = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<T, RS_SplineData> ||
                          std::is_same_v<T, LC_SplinePointsData>)
                return data.fitTangentsConstrained;
            return false;
        }, sources[i].value);
        const auto expectedSchema = tangentSource ? xd::Status::Schema3
                                                  : xd::Status::Schema2;
        xd::Definition definition;
        definition.source = LC_OffsetSourceDefinition{sources[i], -.75, .00025, true};
        definition.reservedFlags = 0x10;
        definition.tail = {{1000, QStringLiteral("file roundtrip tail")}, {1070, std::int16_t{23}}};
        if (tangentSource) {
            const auto encodedSource = values::encodeSourceDefinition(*definition.source);
            REQUIRE(encodedSource);
            const auto decodedSource = values::decodeSourceDefinition(*encodedSource);
            REQUIRE(decodedSource);
        }

        const auto verify = [&](const std::filesystem::path &path, const Format& fileFormat,
                                bool secondSave) {
            CAPTURE(fileFormat.suffix);
            const auto captured = f::readCodec(path, fileFormat.dwg);
            REQUIRE(captured.splines.size() == 1);
            xd::Options options;
            if (fileFormat.dwg) {
                options.eed = true;
                options.textEncoding = xd::TextEncoding::Utf16;
            } else {
                options.asciiDxf = false;
            }
            CAPTURE(f::tagsState(captured.splines.front().extData));
            const auto parsed = xd::parseAndStrip(captured.splines.front().extData, options);
            REQUIRE(parsed.status == expectedSchema);
            REQUIRE(parsed.definition);
            REQUIRE(parsed.definition->source);
            CHECK(parsed.definition->reservedFlags == definition.reservedFlags);
            REQUIRE(parsed.definition->tail.size() == definition.tail.size());
            const auto before = values::encodeSourceDefinition(*definition.source);
            const auto after = values::encodeSourceDefinition(*parsed.definition->source);
            REQUIRE(before); REQUIRE(after);
            CHECK(QCborValue(*before).toCbor() == QCborValue(*after).toCbor());
            if (!secondSave) return;

            const auto repeat = directory.file("second-" + std::to_string(i) + "-" +
                                               fileFormat.suffix);
#ifdef DWGSUPPORT
            if (fileFormat.dwg)
                f::dwgFile(repeat, captured.splines.front(), fileFormat.version);
            else
                f::binaryDxf(repeat, captured.splines.front(), fileFormat.version);
#else
            REQUIRE_FALSE(fileFormat.dwg);
            f::binaryDxf(repeat, captured.splines.front(), fileFormat.version);
#endif
            const auto second = f::readCodec(repeat, fileFormat.dwg);
            REQUIRE(second.splines.size() == 1);
            const auto repeated = xd::parseAndStrip(second.splines.front().extData, options);
            REQUIRE(repeated.status == expectedSchema);
            REQUIRE(repeated.definition);
            REQUIRE(repeated.definition->source);
            const auto repeatedSource = values::encodeSourceDefinition(*repeated.definition->source);
            REQUIRE(repeatedSource);
            CHECK(QCborValue(*before).toCbor() == QCborValue(*repeatedSource).toCbor());

            // Keep one second-saved carrier per source family for optional
            // independent CAD inspection; default tests remain hermetic.
            constexpr std::array<std::size_t, 9> representativeSources{
                0, 5, 24, 26, 29, 38, 39, 40, 41};
            if (fileFormat.dwg &&
                std::find(representativeSources.begin(), representativeSources.end(), i) !=
                    representativeSources.end()) {
                preserveTypedSourceDwg(repeat,
                    "typed-source-" + std::to_string(i) + "-" + fileFormat.suffix);
            }
        };

        for (const auto& fileFormat : formats) {
#ifndef DWGSUPPORT
            if (fileFormat.dwg) continue;
#endif
            xd::Options options;
            if (fileFormat.dwg) {
                options.eed = true;
                options.textEncoding = xd::TextEncoding::Utf16;
            }
            else options.asciiDxf = false;
            const auto tags = xd::forExport({app}, definition, options);
            REQUIRE(tags);
            if (tangentSource) {
                CAPTURE(fileFormat.suffix);
                const auto immediate = xd::parseAndStrip(*tags, options);
                CHECK(immediate.status == xd::Status::Schema3);
            }
            auto entity = f::raw(1, *tags);
            const auto path = directory.file(std::to_string(i) + "-" + fileFormat.suffix);
#ifdef DWGSUPPORT
            if (fileFormat.dwg) f::dwgFile(path, entity, fileFormat.version);
            else f::binaryDxf(path, entity, fileFormat.version);
#else
            f::binaryDxf(path, entity, fileFormat.version);
#endif
            verify(path, fileFormat, true);
        }
    }
}

TEST_CASE("ODA-round-tripped typed-source DWGs retain all modeled source families",
          "[lc-offsetcurve][persistence][typed-source][external][oda]") {
    Setup setup;
#ifndef DWGSUPPORT
    SKIP("requires DWGSUPPORT to read the original EED fixtures");
#endif

    const auto fixtureRoot = std::filesystem::path(__FILE__).parent_path() /
        "testdata" / "offsetcurve_independent" / "oda_typed_source";
    const char* sourceOverride = std::getenv("LIBRECAD_TYPED_SOURCE_DWG_DIR");
    const char* dxfOverride = std::getenv("LIBRECAD_TYPED_SOURCE_ODA_DXF_DIR");
    const auto sourceDirectory = sourceOverride != nullptr && *sourceOverride != '\0'
        ? std::filesystem::path(sourceOverride) : fixtureRoot / "dwg";
    const auto directory = dxfOverride != nullptr && *dxfOverride != '\0'
        ? std::filesystem::path(dxfOverride) : fixtureRoot / "dxf";

    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;
    using Kind = LC_OffsetSourceGeometry::Kind;
    struct Representative {
        std::size_t sourceIndex;
        Kind kind;
    };
    constexpr std::array<Representative, 9> representatives{{
        {0, Kind::Ellipse}, {5, Kind::Spline}, {14, Kind::SplinePoints},
        {16, Kind::Parabola}, {19, Kind::BoundedHyperbola}, {28, Kind::Line},
        {29, Kind::Arc}, {30, Kind::Circle}, {31, Kind::Polyline}}};

    for (const auto& representative : representatives) {
        CAPTURE(representative.sourceIndex, static_cast<int>(representative.kind));
        for (const char* release : {"r2000", "r2018"}) {
            CAPTURE(release);
            const auto sourcePath = sourceDirectory /
                ("typed-source-" + std::to_string(representative.sourceIndex) + "-" +
                 release + ".dwg");
            const auto path = directory /
                ("typed-source-" + std::to_string(representative.sourceIndex) + "-" +
                 release + ".dxf");
            REQUIRE(std::filesystem::is_regular_file(sourcePath));
            REQUIRE(std::filesystem::is_regular_file(path));
            const auto originalDrawing = f::readCodec(sourcePath, true);
            REQUIRE(originalDrawing.splines.size() == 1);
            const auto drawing = f::readCodec(path, false);
            REQUIRE(drawing.splines.size() == 1);
            xd::Options sourceEncoding;
            sourceEncoding.eed = true;
            sourceEncoding.textEncoding = xd::TextEncoding::Utf16;
            const auto original = xd::parseAndStrip(
                originalDrawing.splines.front().extData, sourceEncoding);
            REQUIRE(original.status == xd::Status::Schema2);
            REQUIRE(original.definition);
            REQUIRE(original.definition->source);
            xd::Options dxfEncoding;
            dxfEncoding.asciiDxf = true;
            const auto converted = xd::parseAndStrip(
                drawing.splines.front().extData, dxfEncoding);
            REQUIRE(converted.status == xd::Status::Schema2);
            REQUIRE(converted.definition);
            REQUIRE(converted.definition->source);
            CHECK(converted.definition->source->source.kind() == representative.kind);
            CHECK(converted.definition->source->isWellFormed());
            const auto originalSource = values::encodeSourceDefinition(*original.definition->source);
            const auto convertedSource = values::encodeSourceDefinition(*converted.definition->source);
            REQUIRE(originalSource);
            REQUIRE(convertedSource);
            CHECK(QCborValue(*originalSource).toCbor() ==
                  QCborValue(*convertedSource).toCbor());
            CHECK(converted.definition->reservedFlags == original.definition->reservedFlags);
            REQUIRE(converted.definition->tail.size() == original.definition->tail.size());
            REQUIRE(original.remaining);
            REQUIRE(converted.remaining);
            CHECK(f::tagsState(*converted.remaining) == f::tagsState(*original.remaining));

            const auto imported = stagedImport(path, Lane::Binary18);
            REQUIRE(imported.status == LC_OffsetImportSession::Status::Complete);
            REQUIRE(imported.graphic);
            CHECK(imported.admitted == 0);
            CHECK(imported.downgraded == 1);
            auto* fallback = f::sole(*imported.graphic);
            REQUIRE(fallback);
            const auto retained = xd::parseAndStrip(fallback->getDrwExtData(), dxfEncoding);
            REQUIRE(retained.status == xd::Status::Schema2);
            REQUIRE(retained.definition);
            REQUIRE(retained.definition->source);
            const auto retainedSource = values::encodeSourceDefinition(
                *retained.definition->source);
            REQUIRE(retainedSource);
            CHECK(QCborValue(*originalSource).toCbor() ==
                  QCborValue(*retainedSource).toCbor());
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
    const auto options = LC_OffsetWorkerTolerance::directOptions(
        source, .5, definition.relativeTolerance).value();
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
    namespace req = LC_OffsetWorkerRequest;
    const double tau = 2.0 * M_PI;
    LC_OffsetWorkerClient worker;
    qint64 nextRequestId = 1;
    const auto runWorker = [&](auto operation) {
        req::Request request;
        request.token = {nextRequestId++, 1, 1, 1};
        request.operation = std::move(operation);
        std::optional<LC_OffsetWorkerClient::Outcome> outcome;
        REQUIRE(worker.submit(request, LC_OffsetWorkerClient::Mode::Commit,
            [&](LC_OffsetWorkerClient::Outcome completed) { outcome.emplace(std::move(completed)); }));
        QElapsedTimer elapsed;
        elapsed.start();
        while (!outcome) {
            REQUIRE(elapsed.elapsed() < LC_OffsetWorkerClient::Deadlines::commit + 5000);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QThread::msleep(1);
        }
        REQUIRE(outcome->status == LC_OffsetWorkerClient::Status::Completed);
        REQUIRE(outcome->completed);
        CHECK(outcome->completed->isJoined());
        CHECK(worker.healthyAndIdle());
        return std::move(outcome->completed);
    };
    const auto singleSpanRationalSpline = [](const std::size_t degree) {
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        for (std::size_t i = 0; i <= degree; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(degree);
            data.controlPoints.emplace_back(4.0 * t, 1.2 * t * (1.0 - t) + 0.1 * t);
            data.weights.push_back(0.75 + 0.5 * t);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        return data;
    };
    const auto twoSpanRationalSpline = [](const std::size_t degree) {
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        for (std::size_t i = 0; i <= degree + 1; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(degree + 1);
            data.controlPoints.emplace_back(4.0 * t, 1.2 * t * (1.0 - t) + 0.1 * t);
            data.weights.push_back(0.75 + 0.5 * t);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.push_back(.5);
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        return data;
    };
    const auto nonuniformRationalSpline = [](const std::size_t degree) {
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        for (std::size_t i = 0; i <= degree + 2; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(degree + 2);
            data.controlPoints.emplace_back(4.0 * t, 1.2 * t * (1.0 - t) + 0.1 * t);
            data.weights.push_back(0.75 + 0.5 * t);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.insert(data.knotslist.end(), {.2, .7});
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        return data;
    };
    const auto repeatedKnotRationalSpline = [](const std::size_t degree) {
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        for (std::size_t i = 0; i <= degree + 2; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(degree + 2);
            data.controlPoints.emplace_back(4.0 * t, 1.2 * t * (1.0 - t) + 0.1 * t);
            data.weights.push_back(0.75 + 0.5 * t);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.insert(data.knotslist.end(), 2, .4);
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        return data;
    };
    const auto asymmetricRationalSpline = [](const std::size_t degree) {
        constexpr std::array<double, 8> yValues{0.0, 1.5, -.4, 2.2, .3, 1.1, -1.0, .5};
        constexpr std::array<double, 8> weights{.7, 1.5, .6, 1.8, .85, 1.3, 2.0, .9};
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        for (std::size_t i = 0; i <= degree + 1; ++i) {
            data.controlPoints.emplace_back(.8 * static_cast<double>(i) + .04 * (i % 2),
                                            yValues[i]);
            data.weights.push_back(weights[i]);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.push_back(degree == 4 ? .3 : .62);
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        return data;
    };
    const std::array<LC_OffsetSourceGeometry, 30> sources{
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {4, 0}, .6, 0, tau, false}},
        LC_OffsetSourceGeometry{RS_EllipseData{{3, -2}, {3, 0}, .7, .2, 2.1, false}},
        LC_OffsetSourceGeometry{RS_EllipseData{{-3, 1}, {2, 1}, .8, 1.8, .1, true}},
        LC_OffsetSourceGeometry{RS_EllipseData{{1, -2}, {2, 0}, 1.4, .3, 2.3, false}},
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {2, 0}, .2, 0, tau, false}},
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {2, 0}, .2, 0, 0, true}},
        LC_OffsetSourceGeometry{RS_ArcData{{-1, 2}, 2.5, .2, 2.4, true}},
        LC_OffsetSourceGeometry{RS_CircleData{{1, -1}, 2}},
        LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{-4, 4}, RS_Vector{0, -4}, RS_Vector{4, 4}}}},
        LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{4, 4}, RS_Vector{0, -4}, RS_Vector{-4, 4}}}},
        LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{4, 4}, RS_Vector{-6, 9}, RS_Vector{4, 14}}}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .6, 1.5, -1.2, false}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .6, -.4, .4, true}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, true}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .6, .2, 1.2, false}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, -.2, false}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, false}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, true}},
        LC_OffsetSourceGeometry{RS_LineData{{0, 0}, {2, 0}}},
        LC_OffsetSourceGeometry{singleSpanRationalSpline(4)},
        LC_OffsetSourceGeometry{singleSpanRationalSpline(6)},
        LC_OffsetSourceGeometry{twoSpanRationalSpline(4)},
        LC_OffsetSourceGeometry{twoSpanRationalSpline(6)},
        LC_OffsetSourceGeometry{nonuniformRationalSpline(4)},
        LC_OffsetSourceGeometry{nonuniformRationalSpline(6)},
        LC_OffsetSourceGeometry{repeatedKnotRationalSpline(4)},
        LC_OffsetSourceGeometry{repeatedKnotRationalSpline(6)},
        LC_OffsetSourceGeometry{asymmetricRationalSpline(4)},
        LC_OffsetSourceGeometry{asymmetricRationalSpline(6)}
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
    std::size_t representativeNativeWriterCases = 0;
    for (std::size_t sourceIndex = 0; sourceIndex < sources.size(); ++sourceIndex) {
        CAPTURE(sourceIndex, static_cast<int>(sources[sourceIndex].kind()));
        const auto* splineSource =
            std::get_if<RS_SplineData>(&sources[sourceIndex].value);
        const std::vector<double> signedDistances = splineSource && splineSource->degree > 3
                ? std::vector<double>{-.01, .01}
            :
            sources[sourceIndex].kind() == LC_OffsetSourceGeometry::Kind::BoundedHyperbola
                ? std::vector<double>{-.5, -.3, -.1, .1, .3, .5}
                : (std::get_if<RS_EllipseData>(&sources[sourceIndex].value) &&
                   std::get<RS_EllipseData>(sources[sourceIndex].value).ratio == .2)
                    ? std::vector<double>{-.08, -.07, -.05, .05, .07, .08}
                : std::vector<double>{-.4, -.1, .1, .4};
        for (const double signedDistance : signedDistances) {
            CAPTURE(signedDistance);
            const double relativeTolerance = splineSource && splineSource->degree > 3
                ? 2e-3 : 1e-4;
            const LC_OffsetSourceDefinition definition{
                sources[sourceIndex], signedDistance, relativeTolerance, false};
            auto source = definition.source.makeEntity();
            REQUIRE(source);
            auto generation = runWorker(req::GenerateSource{definition, {}});
            const auto& generatedReply = generation->reply();
            REQUIRE(generatedReply.operation == 7);
            REQUIRE(std::holds_alternative<LC_OffsetWorkerResult::Native>(generatedReply.result));
            const auto& generated = std::get<LC_OffsetWorkerResult::Native>(generatedReply.result);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == LC_OffsetWorkerResult::OrdinaryStatus::Complete);
            REQUIRE(generated.ordinary.size() == 1);
            const auto generatedNative = generation->nativeSource(*source);
            REQUIRE(generatedNative);
            RS_SplineData expectedCache = generatedNative->getData();
            RS_Spline checkedCache{nullptr, expectedCache};
            REQUIRE(checkedCache.validate());

            for (const auto& fileFormat : formats) {
                const auto [lane, dwg, version, formatSuffix] = fileFormat;
                CAPTURE(formatSuffix);
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

                const bool representativeWriterCase = signedDistance == -.1 &&
                    (sourceIndex == 0 || sourceIndex == 6 || sourceIndex == 7 ||
                     sourceIndex == 19);
                if (representativeWriterCase) {
                    const auto nativeOutput = directory.file(
                        "native-" + suffix);
                    REQUIRE(lc::test::exportDrawing(*outcome.graphic, nativeOutput,
                                                     ::format(lane)));
                    const auto nativeRoundTrip = stagedImport(nativeOutput, lane);
                    INFO("native typed-source writer round-trip " << suffix);
                    REQUIRE(nativeRoundTrip.status ==
                            LC_OffsetImportSession::Status::Complete);
                    REQUIRE(nativeRoundTrip.graphic);
                    CHECK(nativeRoundTrip.admitted == 1);
                    CHECK(nativeRoundTrip.downgraded == 0);
                    const auto* roundTripped = dynamic_cast<const LC_OffsetCurve*>(
                        f::sole(*nativeRoundTrip.graphic));
                    REQUIRE(roundTripped);
                    REQUIRE(roundTripped->sourceDefinition());
                    const auto roundTripSource = values::encodeSourceDefinition(
                        *roundTripped->sourceDefinition());
                    REQUIRE(roundTripSource);
                    CHECK(QCborValue(*expectedSource).toCbor() ==
                          QCborValue(*roundTripSource).toCbor());
                    CHECK(RS_Spline::geometryFingerprint(roundTripped->getData()) ==
                          RS_Spline::geometryFingerprint(expectedCache));
                    ++representativeNativeWriterCases;
                }
            }
        }
    }
#ifdef DWGSUPPORT
    CHECK(representativeNativeWriterCases == 4 * formats.size());
#else
    CHECK(representativeNativeWriterCases == 4 * (formats.size() - 2));
#endif
}

TEST_CASE("high-eccentricity ellipse refuses partial cache promotion across file lanes",
          "[lc-offsetcurve][import-session][typed-source][ellipse][refusal]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-thin-ellipse-refusal");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;
    const double tau = 2.0 * M_PI;
    const std::array<LC_OffsetSourceGeometry, 2> sourceGeometries{
        LC_OffsetSourceGeometry{
            RS_EllipseData{{0, 0}, {2, 0}, .2, 0, tau, false}},
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {2, 0}, .2, 0, 0, true}}};

    struct Format {
        Lane lane;
        bool dwg;
        DRW::Version version;
        const char* suffix;
    };
    const std::array<Format, 5> formats{
        {{Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
         {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
         {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
         {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
         {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};

    for (std::size_t sourceIndex = 0; sourceIndex < sourceGeometries.size();
         ++sourceIndex) {
        const auto& sourceGeometry = sourceGeometries[sourceIndex];
        const auto* ellipse = std::get_if<RS_EllipseData>(&sourceGeometry.value);
        REQUIRE(ellipse);
        const double inwardSign = ellipse->reversed ? -1.0 : 1.0;
        for (const double magnitude : {.09, .1, .4}) {
            const double signedDistance = inwardSign * magnitude;
            CAPTURE(sourceIndex, signedDistance);
            const LC_OffsetSourceDefinition definition{sourceGeometry, signedDistance,
                                                       1e-4, false};
            auto source = definition.source.makeEntity();
            REQUIRE(source);
            const auto options = LC_OffsetWorkerTolerance::directOptions(
                *source, magnitude, definition.relativeTolerance).value();
            const auto generated = LC_CurveOffset::buildDirectBranches(
                *source,
                LC_CurveOffset::makeSideRequest(signedDistance < 0
                                                    ? LC_CurveOffsetSide::Right
                                                    : LC_CurveOffsetSide::Left,
                                                magnitude),
                options, LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
            const auto materialized = LC_CurveOffset::materializeBranches(
                *source, generated, options,
                LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
            REQUIRE(materialized.entities.size() > 1);
            const auto* firstBranch =
                dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
            REQUIRE(firstBranch);
            const RS_SplineData firstBranchCache = firstBranch->getData();
            REQUIRE(firstBranch->validate());

            for (const auto& format : formats) {
#ifndef DWGSUPPORT
                if (format.dwg)
                    continue;
#endif
                xd::Definition metadata;
                metadata.source = definition;
                xd::Options encoding;
                if (format.dwg) {
                    encoding.eed = true;
                    encoding.textEncoding = xd::TextEncoding::Utf16;
                } else {
                    encoding.asciiDxf = false;
                }
                const auto tags = xd::forExport(
                    {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
                    metadata, encoding);
                REQUIRE(tags);
                DRW_Spline cache = f::raw(1, *tags);
                cache.controllist.clear();
                for (const auto& point : firstBranchCache.controlPoints)
                    cache.controllist.push_back(
                        std::make_shared<DRW_Coord>(point.x, point.y, 0));
                cache.knotslist = firstBranchCache.knotslist;
                cache.weightlist = firstBranchCache.weights;
                cache.degree = firstBranchCache.degree;
                cache.flags = 8;
                cache.m_scenario = 1;
                cache.ncontrol = cache.controllist.size();
                cache.nknots = cache.knotslist.size();
                cache.nfit = 0;

                const auto input = directory.file(std::to_string(sourceIndex) + "-" +
                                                  std::to_string(signedDistance) + "-" +
                                                  format.suffix);
#ifdef DWGSUPPORT
                if (format.dwg)
                    f::dwgFile(input, cache, format.version);
                else
                    f::binaryDxf(input, cache, format.version);
#else
                f::binaryDxf(input, cache, format.version);
#endif
                const auto outcome = stagedImport(input, format.lane);
                REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
                REQUIRE(outcome.graphic);
                CHECK(outcome.admitted == 0);
                CHECK(outcome.downgraded == 1);
                auto* fallback = dynamic_cast<RS_Spline*>(f::sole(*outcome.graphic));
                REQUIRE(fallback);
                CHECK(dynamic_cast<const LC_OffsetCurve*>(fallback) == nullptr);
                CHECK(RS_Spline::geometryFingerprint(fallback->getData()) ==
                      RS_Spline::geometryFingerprint(firstBranchCache));
                const auto parsed =
                    xd::parseAndStrip(fallback->getDrwExtData(), encoding);
                REQUIRE(parsed.status == xd::Status::Schema2);
                REQUIRE(parsed.definition);
                REQUIRE(parsed.definition->source);
                const auto expected = values::encodeSourceDefinition(definition);
                const auto received =
                    values::encodeSourceDefinition(*parsed.definition->source);
                REQUIRE(expected);
                REQUIRE(received);
                CHECK(QCborValue(*expected).toCbor() == QCborValue(*received).toCbor());
            }
        }
    }
}

TEST_CASE("parabola curvature neighborhood admits only exact single-cache imports",
          "[lc-offsetcurve][import-session][typed-source][parabola][curvature]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-parabola-curvature-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;
    namespace req = LC_OffsetWorkerRequest;
    namespace result = LC_OffsetWorkerResult;
    const std::array<LC_OffsetSourceGeometry, 6> sources{
        LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{0, 0}, RS_Vector{5, 10}, RS_Vector{10, 0}}}},
        LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{10, 0}, RS_Vector{5, 10}, RS_Vector{0, 0}}}},
        LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{3, 4}, RS_Vector{-7, 9}, RS_Vector{3, 14}}}},
        LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{0, 0}, RS_Vector{.005, .01}, RS_Vector{.01, 0}}}},
        LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{0, 0}, RS_Vector{5000, 10000}, RS_Vector{10000, 0}}}},
        LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{0, 0}, RS_Vector{5e12, 1e13}, RS_Vector{1e13, 0}}}}
    };
    const std::array<double, 6> sourceScales{1.0, 1.0, 1.0, 1e-3, 1e3, 1e12};

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
    std::size_t promotedCases = 0;
    std::size_t downgradedCases = 0;
    std::size_t parabolaNativeWriterCases = 0;
    LC_OffsetWorkerClient worker;
    qint64 nextRequestId = 1;
    const auto runWorker = [&](req::Request request) {
        std::optional<LC_OffsetWorkerClient::Outcome> outcome;
        REQUIRE(worker.submit(request, LC_OffsetWorkerClient::Mode::Commit,
            [&](LC_OffsetWorkerClient::Outcome completed) { outcome.emplace(std::move(completed)); }));
        QElapsedTimer elapsed;
        elapsed.start();
        while (!outcome) {
            REQUIRE(elapsed.elapsed() < LC_OffsetWorkerClient::Deadlines::commit + 5000);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QThread::msleep(1);
        }
        REQUIRE(outcome->status == LC_OffsetWorkerClient::Status::Completed);
        REQUIRE(outcome->completed);
        CHECK(outcome->completed->isJoined());
        CHECK(worker.healthyAndIdle());
        return std::move(outcome->completed);
    };

    for (std::size_t sourceIndex = 0; sourceIndex < sources.size(); ++sourceIndex) {
        bool sourcePromoted = false;
        bool sourceDowngraded = false;
        for (const double unitDistance : {-3.0, -2.5, -2.0, 2.0, 2.5, 3.0}) {
            const double signedDistance = unitDistance * sourceScales[sourceIndex];
            CAPTURE(sourceIndex, unitDistance, signedDistance);
            const LC_OffsetSourceDefinition definition{
                sources[sourceIndex], signedDistance, 1e-4, false};
            const auto completion = runWorker({
                {nextRequestId++, 1, 1, 1}, req::GenerateSource{definition, {}}});
            const auto& reply = completion->reply();
            REQUIRE(reply.failure == result::Failure::None);
            REQUIRE(reply.operation == 7);
            REQUIRE(std::holds_alternative<result::Native>(reply.result));
            const auto& generated = std::get<result::Native>(reply.result);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE_FALSE(generated.ordinary.empty());
            const auto* expectedCache = std::get_if<RS_SplineData>(&generated.ordinary.front());
            REQUIRE(expectedCache);
            RS_Spline expectedSpline{nullptr, *expectedCache};
            REQUIRE(expectedSpline.validate());
            const bool shouldPromote = generated.ordinary.size() == 1;
            sourcePromoted |= shouldPromote;
            sourceDowngraded |= !shouldPromote;
            if (shouldPromote) ++promotedCases;
            else ++downgradedCases;

            for (const auto& format : formats) {
#ifndef DWGSUPPORT
                if (format.dwg) continue;
#endif
                xd::Definition metadata;
                metadata.source = definition;
                xd::Options encoding;
                if (format.dwg) {
                    encoding.eed = true;
                    encoding.textEncoding = xd::TextEncoding::Utf16;
                } else {
                    encoding.asciiDxf = false;
                }
                const auto tags = xd::forExport(
                    {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
                    metadata, encoding);
                REQUIRE(tags);
                DRW_Spline cache = f::raw(1, *tags);
                cache.controllist.clear();
                for (const auto& point : expectedCache->controlPoints)
                    cache.controllist.push_back(
                        std::make_shared<DRW_Coord>(point.x, point.y, 0));
                cache.knotslist = expectedCache->knotslist;
                cache.weightlist = expectedCache->weights;
                cache.degree = expectedCache->degree;
                cache.flags = 8;
                cache.m_scenario = 1;
                cache.ncontrol = cache.controllist.size();
                cache.nknots = cache.knotslist.size();
                cache.nfit = 0;

                const auto input = directory.file(
                    std::to_string(sourceIndex) + "-" + std::to_string(signedDistance) +
                    "-" + format.suffix);
#ifdef DWGSUPPORT
                if (format.dwg) f::dwgFile(input, cache, format.version);
                else f::binaryDxf(input, cache, format.version);
#else
                f::binaryDxf(input, cache, format.version);
#endif
                const auto outcome = stagedImport(input, format.lane);
                REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
                REQUIRE(outcome.graphic);
                CHECK(outcome.admitted == (shouldPromote ? 1u : 0u));
                CHECK(outcome.downgraded == (shouldPromote ? 0u : 1u));
                const auto* imported = f::sole(*outcome.graphic);
                REQUIRE(imported);
                const auto* nativeOffset = dynamic_cast<const LC_OffsetCurve*>(imported);
                CHECK((nativeOffset != nullptr) == shouldPromote);
                const auto* importedSpline = dynamic_cast<const RS_Spline*>(imported);
                REQUIRE(importedSpline);
                CHECK(RS_Spline::geometryFingerprint(importedSpline->getData()) ==
                      RS_Spline::geometryFingerprint(*expectedCache));
                const auto expectedSource = values::encodeSourceDefinition(definition);
                REQUIRE(expectedSource);
                if (shouldPromote) {
                    REQUIRE(nativeOffset);
                    REQUIRE(nativeOffset->sourceDefinition());
                    const auto importedSource = values::encodeSourceDefinition(
                        *nativeOffset->sourceDefinition());
                    REQUIRE(importedSource);
                    CHECK(QCborValue(*expectedSource).toCbor() ==
                          QCborValue(*importedSource).toCbor());
                } else {
                    const auto parsed = xd::parseAndStrip(
                        imported->getDrwExtData(), encoding);
                    REQUIRE(parsed.status == xd::Status::Schema2);
                    REQUIRE(parsed.definition);
                    REQUIRE(parsed.definition->source);
                    const auto importedSource = values::encodeSourceDefinition(
                        *parsed.definition->source);
                    REQUIRE(importedSource);
                    CHECK(QCborValue(*expectedSource).toCbor() ==
                          QCborValue(*importedSource).toCbor());
                }

                if (shouldPromote) {
                    const auto nativeOutput = directory.file(
                        std::to_string(sourceIndex) + "-" +
                        std::to_string(signedDistance) + "-native-parabola-" +
                        format.suffix);
                    REQUIRE(lc::test::exportDrawing(*outcome.graphic, nativeOutput,
                                                     ::format(format.lane)));
                    const auto nativeRoundTrip = stagedImport(nativeOutput, format.lane);
                    INFO("native parabola writer round-trip " << format.suffix);
                    REQUIRE(nativeRoundTrip.status ==
                            LC_OffsetImportSession::Status::Complete);
                    REQUIRE(nativeRoundTrip.graphic);
                    CHECK(nativeRoundTrip.admitted == 1);
                    CHECK(nativeRoundTrip.downgraded == 0);
                    const auto* roundTripped = dynamic_cast<const LC_OffsetCurve*>(
                        f::sole(*nativeRoundTrip.graphic));
                    REQUIRE(roundTripped);
                    REQUIRE(roundTripped->sourceDefinition());
                    const auto roundTripSource = values::encodeSourceDefinition(
                        *roundTripped->sourceDefinition());
                    REQUIRE(roundTripSource);
                    CHECK(QCborValue(*expectedSource).toCbor() ==
                          QCborValue(*roundTripSource).toCbor());
                    CHECK(RS_Spline::geometryFingerprint(roundTripped->getData()) ==
                          RS_Spline::geometryFingerprint(*expectedCache));
                    ++parabolaNativeWriterCases;
                }
            }
        }
        CHECK(sourcePromoted);
        CHECK(sourceDowngraded);
    }
    CHECK(promotedCases > 0);
    CHECK(downgradedCases > 0);
#ifdef DWGSUPPORT
    CHECK(parabolaNativeWriterCases > formats.size());
#else
    CHECK(parabolaNativeWriterCases > formats.size() - 2);
#endif
}

TEST_CASE("closed mixed-bulge polyline promotes only its representable side across file lanes",
          "[lc-offsetcurve][import-session][typed-source][polyline][cache-promotion][persistence]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-mixed-bulge-polyline-promotion");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    RS_Polyline sourcePolyline;
    sourcePolyline.addVertex({0, 0}, 0.2);
    sourcePolyline.addVertex({8, 0}, -0.2);
    sourcePolyline.addVertex({8, 6});
    sourcePolyline.addVertex({0, 6});
    sourcePolyline.setClosed(true);
    sourcePolyline.endPolyline();
    const auto source = LC_OffsetSourceGeometry::capture(sourcePolyline);
    REQUIRE(source);

    struct Format {
        Lane lane;
        bool dwg;
        DRW::Version version;
        const char* suffix;
    };
    const std::array<Format, 5> formats{
        {{Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
         {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
         {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
         {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
         {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};

    for (const double signedDistance : {-.25, .25}) {
        CAPTURE(signedDistance);
        const LC_OffsetSourceDefinition definition{*source, signedDistance, 1e-4,
                                                   false};
        auto sourceEntity = definition.source.makeEntity();
        REQUIRE(sourceEntity);
        const double magnitude = std::abs(signedDistance);
        const auto options = LC_OffsetWorkerTolerance::directOptions(
            *sourceEntity, magnitude, definition.relativeTolerance).value();
        const auto generated = LC_CurveOffset::buildDirectBranches(
            *sourceEntity,
            LC_CurveOffset::makeSideRequest(signedDistance < 0
                                                ? LC_CurveOffsetSide::Right
                                                : LC_CurveOffsetSide::Left,
                                            magnitude),
            options, LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
        const auto materialized = LC_CurveOffset::materializeBranches(
            *sourceEntity, generated, options,
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
        if (signedDistance < 0)
            REQUIRE(materialized.entities.size() == 1);
        else
            REQUIRE(materialized.entities.size() > 1);

        const auto* cacheEntity =
            dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
        REQUIRE(cacheEntity);
        REQUIRE(cacheEntity->validate());
        const RS_SplineData expectedCache = cacheEntity->getData();

        for (const auto& format : formats) {
#ifndef DWGSUPPORT
            if (format.dwg)
                continue;
#endif
            xd::Definition metadata;
            metadata.source = definition;
            xd::Options encoding;
            if (format.dwg) {
                encoding.eed = true;
                encoding.textEncoding = xd::TextEncoding::Utf16;
            } else {
                encoding.asciiDxf = false;
            }
            const auto tags = xd::forExport(
                {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
                metadata, encoding);
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

            const auto input = directory.file(
                std::string(signedDistance < 0 ? "right-" : "left-") + format.suffix);
#ifdef DWGSUPPORT
            if (format.dwg)
                f::dwgFile(input, cache, format.version);
            else
                f::binaryDxf(input, cache, format.version);
#else
            f::binaryDxf(input, cache, format.version);
#endif
            const auto outcome = stagedImport(input, format.lane);
            INFO(format.suffix << " " << signedDistance);
            REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
            REQUIRE(outcome.graphic);
            if (signedDistance < 0) {
                CHECK(outcome.admitted == 1);
                CHECK(outcome.downgraded == 0);
                const auto* imported = dynamic_cast<const LC_OffsetCurve*>(
                    f::sole(*outcome.graphic));
                REQUIRE(imported);
                REQUIRE(imported->sourceDefinition());
                const auto expectedSource = values::encodeSourceDefinition(definition);
                const auto actualSource = values::encodeSourceDefinition(
                    *imported->sourceDefinition());
                REQUIRE(expectedSource);
                REQUIRE(actualSource);
                CHECK(QCborValue(*expectedSource).toCbor() ==
                      QCborValue(*actualSource).toCbor());
                CHECK(RS_Spline::geometryFingerprint(imported->getData()) ==
                      RS_Spline::geometryFingerprint(expectedCache));
            } else {
                CHECK(outcome.admitted == 0);
                CHECK(outcome.downgraded == 1);
                const auto* fallback = f::sole(*outcome.graphic);
                REQUIRE(fallback);
                CHECK(dynamic_cast<const LC_OffsetCurve*>(fallback) == nullptr);
                const auto preserved = xd::parseAndStrip(
                    fallback->getDrwExtData(), encoding);
                CHECK(preserved.status == xd::Status::Schema2);
                REQUIRE(preserved.definition);
                REQUIRE(preserved.definition->source);
                const auto expectedSource = values::encodeSourceDefinition(definition);
                const auto preservedSource = values::encodeSourceDefinition(
                    *preserved.definition->source);
                REQUIRE(expectedSource);
                REQUIRE(preservedSource);
                CHECK(QCborValue(*expectedSource).toCbor() ==
                      QCborValue(*preservedSource).toCbor());
            }
        }
    }
}

TEST_CASE("open mixed-bulge polyline promotes complete worker caches across file lanes",
          "[lc-offsetcurve][import-session][typed-source][polyline][cache-promotion][persistence]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-open-mixed-bulge-polyline-promotion");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    RS_Polyline sourcePolyline;
    sourcePolyline.addVertex({0, 0}, .2);
    sourcePolyline.addVertex({8, 0});
    sourcePolyline.addVertex({8, 6});
    sourcePolyline.endPolyline();
    const auto source = LC_OffsetSourceGeometry::capture(sourcePolyline);
    REQUIRE(source);
    REQUIRE(source->kind() == LC_OffsetSourceGeometry::Kind::Polyline);

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
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};

    LC_OffsetWorkerClient generationWorker;
    qint64 nextRequestId = 1;
    std::size_t multiBranchPromotions = 0;
    for (const double signedDistance : {-.25, .25}) {
        CAPTURE(signedDistance);
        const LC_OffsetSourceDefinition definition{*source, signedDistance, 1e-4, false};
        auto sourceEntity = definition.source.makeEntity();
        REQUIRE(sourceEntity);
        const double magnitude = std::abs(signedDistance);
        const auto options = LC_OffsetWorkerTolerance::directOptions(
            *sourceEntity, magnitude, definition.relativeTolerance).value();
        const auto side = signedDistance < 0 ? LC_CurveOffsetSide::Right
                                             : LC_CurveOffsetSide::Left;
        const auto generated = LC_CurveOffset::buildDirectBranches(
            *sourceEntity, LC_CurveOffset::makeSideRequest(side, magnitude), options,
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
        const auto materialized = LC_CurveOffset::materializeBranches(
            *sourceEntity, generated, options, LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
        REQUIRE_FALSE(materialized.entities.empty());

        std::vector<LC_OffsetWorkerRequest::Cutter> branches;
        for (const auto& entity : materialized.entities) {
            if (const auto* spline = dynamic_cast<const RS_Spline*>(entity.get())) {
                branches.emplace_back(spline->getData());
            } else if (const auto* segment = dynamic_cast<const RS_Line*>(entity.get())) {
                branches.emplace_back(LC_OffsetWorkerRequest::Segment{
                    segment->getStartpoint(), segment->getEndpoint()});
            } else {
                FAIL("unexpected materialized offset branch type " << entity->rtti());
            }
        }
        REQUIRE_FALSE(branches.empty());

        LC_OffsetWorkerRequest::Request request;
        request.token = {nextRequestId++, 1, 1, 1};
        request.operation = LC_OffsetWorkerRequest::GenerateSource{definition, {}};
        std::optional<LC_OffsetWorkerClient::Outcome> generatedOutcome;
        REQUIRE(generationWorker.submit(request, LC_OffsetWorkerClient::Mode::Commit,
            [&](LC_OffsetWorkerClient::Outcome outcome) {
                generatedOutcome.emplace(std::move(outcome));
            }));
        QElapsedTimer elapsed;
        elapsed.start();
        while (!generatedOutcome) {
            REQUIRE(elapsed.elapsed() < LC_OffsetWorkerClient::Deadlines::commit + 5000);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QThread::msleep(1);
        }
        REQUIRE(generatedOutcome->status == LC_OffsetWorkerClient::Status::Completed);
        REQUIRE(generatedOutcome->completed);
        REQUIRE(generatedOutcome->completed->isJoined());
        const auto& workerReply = generatedOutcome->completed->reply();
        REQUIRE(workerReply.operation == 7);
        REQUIRE(std::holds_alternative<LC_OffsetWorkerResult::Native>(workerReply.result));
        const auto& workerGenerated = std::get<LC_OffsetWorkerResult::Native>(workerReply.result);
        REQUIRE(workerGenerated.ordinaryStatus ==
                LC_OffsetWorkerResult::OrdinaryStatus::Complete);
        auto workerNative = generatedOutcome->completed->nativeSource(*sourceEntity);

        RS_SplineData expectedCache;
        const bool cacheRepresentable = static_cast<bool>(workerNative);
        if (workerNative) {
            expectedCache = workerNative->getData();
            if (workerGenerated.ordinary.size() > 1) ++multiBranchPromotions;
        } else {
            REQUIRE(LC_OffsetWorkerSourceCache::combine({branches.front()}, expectedCache));
        }
        RS_Spline cacheCheck{nullptr, expectedCache};
        REQUIRE(cacheCheck.validate());

        for (const auto& format : formats) {
#ifndef DWGSUPPORT
            if (format.dwg) continue;
#endif
            CAPTURE(format.suffix);
            xd::Definition metadata;
            metadata.source = definition;
            xd::Options encoding;
            if (format.dwg) {
                encoding.eed = true;
                encoding.textEncoding = xd::TextEncoding::Utf16;
            } else {
                encoding.asciiDxf = false;
            }
            const auto tags = xd::forExport(
                {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))}, metadata,
                encoding);
            REQUIRE(tags);
            DRW_Spline carrier = f::raw(1, *tags);
            carrier.controllist.clear();
            for (const auto& point : expectedCache.controlPoints)
                carrier.controllist.push_back(std::make_shared<DRW_Coord>(point.x, point.y, 0));
            carrier.knotslist = expectedCache.knotslist;
            carrier.weightlist = expectedCache.weights;
            carrier.degree = expectedCache.degree;
            carrier.flags = 8;
            carrier.m_scenario = 1;
            carrier.ncontrol = carrier.controllist.size();
            carrier.nknots = carrier.knotslist.size();
            carrier.nfit = 0;

            const auto path = directory.file(
                std::string(signedDistance < 0 ? "right-" : "left-") + format.suffix);
#ifdef DWGSUPPORT
            if (format.dwg) f::dwgFile(path, carrier, format.version);
            else f::binaryDxf(path, carrier, format.version);
#else
            f::binaryDxf(path, carrier, format.version);
#endif
            const auto outcome = stagedImport(path, format.lane);
            INFO(format.suffix << " " << signedDistance);
            REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
            REQUIRE(outcome.graphic);
            CAPTURE(branches.size(), workerGenerated.ordinary.size(), cacheRepresentable);
            for (const auto& diagnostic : outcome.diagnostics)
                INFO("import diagnostic kind=" << static_cast<int>(diagnostic.kind)
                     << " reason=" << diagnostic.reason << " detail=" << diagnostic.detail);
            if (cacheRepresentable) {
                CHECK(outcome.admitted == 1);
                CHECK(outcome.downgraded == 0);
                const auto* imported = dynamic_cast<const LC_OffsetCurve*>(
                    f::sole(*outcome.graphic));
                REQUIRE(imported);
                REQUIRE(imported->sourceDefinition());
                const auto expectedSource = values::encodeSourceDefinition(definition);
                const auto actualSource = values::encodeSourceDefinition(
                    *imported->sourceDefinition());
                REQUIRE(expectedSource);
                REQUIRE(actualSource);
                CHECK(QCborValue(*expectedSource).toCbor() == QCborValue(*actualSource).toCbor());

                const std::string roundTripName = std::string("native-") +
                    (signedDistance < 0 ? "right-" : "left-") + format.suffix;
                const auto nativeExport = directory.file(roundTripName);
                REQUIRE(lc::test::exportDrawing(*outcome.graphic, nativeExport,
                                                 ::format(format.lane)));
                const auto nativeRoundTrip = stagedImport(nativeExport, format.lane);
                INFO("native export round-trip " << roundTripName);
                CAPTURE(nativeRoundTrip.diagnostics.size(),
                        nativeRoundTrip.suppressedDiagnostics);
                const int diagnosticKind = nativeRoundTrip.diagnostics.empty()
                    ? -1 : static_cast<int>(nativeRoundTrip.diagnostics.front().kind);
                const int diagnosticReason = nativeRoundTrip.diagnostics.empty()
                    ? -1 : nativeRoundTrip.diagnostics.front().reason;
                const int diagnosticDetail = nativeRoundTrip.diagnostics.empty()
                    ? -1 : nativeRoundTrip.diagnostics.front().detail;
                CAPTURE(diagnosticKind, diagnosticReason, diagnosticDetail);
                REQUIRE(nativeRoundTrip.status == LC_OffsetImportSession::Status::Complete);
                REQUIRE(nativeRoundTrip.graphic);
                CHECK(nativeRoundTrip.admitted == 1);
                CHECK(nativeRoundTrip.downgraded == 0);
                const auto* roundTripped = dynamic_cast<const LC_OffsetCurve*>(
                    f::sole(*nativeRoundTrip.graphic));
                REQUIRE(roundTripped);
                REQUIRE(roundTripped->sourceDefinition());
                const auto roundTripSource = values::encodeSourceDefinition(
                    *roundTripped->sourceDefinition());
                REQUIRE(roundTripSource);
                CHECK(QCborValue(*expectedSource).toCbor() ==
                      QCborValue(*roundTripSource).toCbor());
                CHECK(RS_Spline::geometryFingerprint(roundTripped->getData()) ==
                      RS_Spline::geometryFingerprint(expectedCache));
            } else {
                CHECK(outcome.admitted == 0);
                CHECK(outcome.downgraded == 1);
                const auto* fallback = f::sole(*outcome.graphic);
                REQUIRE(fallback);
                CHECK(dynamic_cast<const LC_OffsetCurve*>(fallback) == nullptr);
                const auto preserved = xd::parseAndStrip(fallback->getDrwExtData(), encoding);
                CHECK(preserved.status == xd::Status::Schema2);
                REQUIRE(preserved.definition);
                REQUIRE(preserved.definition->source);
                const auto expectedSource = values::encodeSourceDefinition(definition);
                const auto preservedSource = values::encodeSourceDefinition(
                    *preserved.definition->source);
                REQUIRE(expectedSource);
                REQUIRE(preservedSource);
                CHECK(QCborValue(*expectedSource).toCbor() == QCborValue(*preservedSource).toCbor());
            }
        }
    }
    CHECK(multiBranchPromotions > 0);
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
      const auto options = LC_OffsetWorkerTolerance::directOptions(
          *source, magnitude, definition.relativeTolerance).value();
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

        const auto nativeOutput = directory.file(std::string("native-control-points-") +
            (signedDistance < 0 ? "right-" : "left-") +
            std::to_string(magnitude) + "-" + formatSuffix);
        REQUIRE(lc::test::exportDrawing(*outcome.graphic, nativeOutput, ::format(lane)));
        const auto nativeRoundTrip = stagedImport(nativeOutput, lane);
        INFO("native writer round-trip " << formatSuffix << " " << signedDistance);
        REQUIRE(nativeRoundTrip.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(nativeRoundTrip.graphic);
        CHECK(nativeRoundTrip.admitted == 1);
        CHECK(nativeRoundTrip.downgraded == 0);
        const auto* roundTripped = dynamic_cast<const LC_OffsetCurve*>(
            f::sole(*nativeRoundTrip.graphic));
        REQUIRE(roundTripped);
        REQUIRE(roundTripped->sourceDefinition());
        const auto roundTripSource = values::encodeSourceDefinition(
            *roundTripped->sourceDefinition());
        REQUIRE(roundTripSource);
        CHECK(QCborValue(*expectedSource).toCbor() ==
              QCborValue(*roundTripSource).toCbor());
        CHECK(RS_Spline::geometryFingerprint(roundTripped->getData()) ==
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

TEST_CASE("closed major-minor bulge imports generate and validate typed offset caches",
          "[lc-offsetcurve][import-session][typed-source][polyline][major-bulge]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-imported-major-minor-bulge-worker");
    namespace req = LC_OffsetWorkerRequest;
    namespace result = LC_OffsetWorkerResult;

    DRW_LWPolyline raw;
    raw.flags = 1;
    raw.vertexnum = 4;
    raw.addVertex(DRW_Vertex2D(0, 0, 2.0));
    raw.addVertex(DRW_Vertex2D(10, 0, -0.2));
    raw.addVertex(DRW_Vertex2D(10, 10, 0));
    raw.addVertex(DRW_Vertex2D(0, 10, 0));

    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};

    LC_OffsetWorkerClient worker;
    qint64 requestId = 1;
    const auto runWorker = [&](req::Request request) {
        std::optional<LC_OffsetWorkerClient::Outcome> outcome;
        REQUIRE(worker.submit(request, LC_OffsetWorkerClient::Mode::Commit,
            [&](LC_OffsetWorkerClient::Outcome completed) { outcome.emplace(std::move(completed)); }));
        QElapsedTimer elapsed;
        elapsed.start();
        while (!outcome) {
            REQUIRE(elapsed.elapsed() < LC_OffsetWorkerClient::Deadlines::commit + 5000);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QThread::msleep(1);
        }
        REQUIRE(outcome->status == LC_OffsetWorkerClient::Status::Completed);
        REQUIRE(outcome->completed);
        CHECK(outcome->completed->isJoined());
        CHECK(worker.healthyAndIdle());
        return std::move(outcome->completed);
    };
    const auto asSpline = [](const req::Cutter& item) {
        if (const auto* spline = std::get_if<RS_SplineData>(&item)) return *spline;
        const auto& segment = std::get<req::Segment>(item);
        RS_SplineData line{1, false};
        line.controlPoints = {segment.start, segment.end};
        line.knotslist = {0, 0, 1, 1};
        line.weights = {1, 1};
        return line;
    };

    for (const auto& format : formats) {
#ifndef DWGSUPPORT
        if (format.dwg) continue;
#endif
        CAPTURE(format.dwg, format.version);
        const auto path = directory.file(std::string("imported-source-") + format.suffix);
#ifdef DWGSUPPORT
        if (format.dwg) f::dwgPolylineFile(path, raw, format.version);
        else f::binaryDxfPolyline(path, raw, format.version);
#else
        f::binaryDxfPolyline(path, raw, format.version);
#endif
        const auto imported = stagedImport(path, format.lane);
        REQUIRE(imported.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(imported.graphic);
        const auto* polyline = dynamic_cast<const RS_Polyline*>(f::sole(*imported.graphic));
        REQUIRE(polyline);
        const auto source = LC_OffsetSourceGeometry::capture(*polyline);
        REQUIRE(source);
        REQUIRE(source->kind() == LC_OffsetSourceGeometry::Kind::Polyline);
        const auto& data = std::get<LC_OffsetPolylineData>(source->value);
        REQUIRE(data.closed);
        REQUIRE(data.segments.size() == 4);
        const auto* positiveMajor = std::get_if<RS_ArcData>(&data.segments[0]);
        const auto* negativeMinor = std::get_if<RS_ArcData>(&data.segments[1]);
        REQUIRE(positiveMajor);
        REQUIRE(negativeMinor);
        CHECK(positiveMajor->reversed == false);
        CHECK(negativeMinor->reversed == true);
        CHECK(std::holds_alternative<RS_LineData>(data.segments[2]));
        CHECK(std::holds_alternative<RS_LineData>(data.segments[3]));

        for (const double signedDistance : {-0.25, 0.25}) {
            CAPTURE(signedDistance);
            const LC_OffsetSourceDefinition definition{*source, signedDistance, 1e-4, false};
            const auto generationCompletion = runWorker({
                {requestId++, 1, 1, 1}, req::GenerateSource{definition, {}}});
            const auto& generationReply = generationCompletion->reply();
            REQUIRE(generationReply.failure == result::Failure::None);
            REQUIRE(generationReply.operation == 7);
            REQUIRE(std::holds_alternative<result::Native>(generationReply.result));
            const auto& generated = std::get<result::Native>(generationReply.result);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE_FALSE(generated.ordinary.empty());
            for (const auto& item : generated.ordinary) {
                auto cache = asSpline(item);
                const auto fingerprint = RS_Spline::geometryFingerprint(cache);
                const auto validationCompletion = runWorker({
                    {requestId++, 1, 1, 1}, req::ValidateSourceCache{cache, definition, {}}});
                const auto& validationReply = validationCompletion->reply();
                REQUIRE(validationReply.failure == result::Failure::None);
                REQUIRE(validationReply.operation == 8);
                REQUIRE(std::holds_alternative<result::Native>(validationReply.result));
                const auto& validation = std::get<result::Native>(validationReply.result);
                if (generated.ordinary.size() == 1) {
                    CHECK(validation.refusal == LC_OffsetCurveRefusal::None);
                    CHECK(validation.changed);
                } else {
                    CHECK(validation.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                    CHECK_FALSE(validation.changed);
                    CHECK(RS_Spline::geometryFingerprint(cache) == fingerprint);
                }
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

    for (const auto &format : bulgedFormats) {
#ifndef DWGSUPPORT
      if (format.dwg)
        continue;
#endif
      DRW_LWPolyline reflectedBulge = bulged;
      reflectedBulge.extPoint = {0.6, 0.0, -0.8};
      reflectedBulge.vertlist.front()->bulge = std::tan(M_PI / 8.0);
      const auto path = directory.file(std::string("reflected-bulge-") +
                                       format.suffix);
#ifdef DWGSUPPORT
      if (format.dwg)
        f::dwgPolylineFile(path, reflectedBulge, format.version);
      else
        f::binaryDxfPolyline(path, reflectedBulge, format.version);
#else
      f::binaryDxfPolyline(path, reflectedBulge, format.version);
#endif
      const auto imported = stagedImport(path, format.lane);
      INFO(format.suffix);
      REQUIRE(imported.status == LC_OffsetImportSession::Status::Complete);
      REQUIRE(imported.graphic);
      REQUIRE(std::distance(imported.graphic->begin(),
                            imported.graphic->end()) == 2);
      const RS_Ellipse *reflectedEllipse = nullptr;
      const RS_Line *reflectedLine = nullptr;
      for (const RS_Entity *entity : *imported.graphic) {
        CHECK(hasFragmentXData(*entity));
        CHECK(hasProjectionApp(*entity, "LibreCAD_POLYLINE_OCS"));
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
        if (const auto *ellipse = dynamic_cast<const RS_Ellipse *>(entity)) {
          reflectedEllipse = ellipse;
          CHECK(hasProjectionApp(*entity, "LibreCAD_CONIC_OCS"));
        }
        if (const auto *line = dynamic_cast<const RS_Line *>(entity))
          reflectedLine = line;
      }
      REQUIRE(reflectedEllipse);
      REQUIRE(reflectedLine);
      CHECK(reflectedEllipse->isReversed());
      CHECK(reflectedEllipse->getAngleLength() == Catch::Approx(M_PI / 2.0));
      CHECK(std::isfinite(reflectedEllipse->getCenter().x));
      CHECK(std::isfinite(reflectedEllipse->getCenter().y));
      CHECK(std::hypot(reflectedEllipse->getEndpoint().x -
                       reflectedLine->getStartpoint().x,
                       reflectedEllipse->getEndpoint().y -
                       reflectedLine->getStartpoint().y) < 1e-8);
    }

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

    for (const auto &format : bulgeFormats) {
#ifndef DWGSUPPORT
      if (format.dwg)
        continue;
#endif
      DRW_LWPolyline fixture = bulged;
      fixture.flags = 1;
      fixture.vertlist.front()->bulge = 2.0;
      const auto path = directory.file(std::string("closed-major-") +
                                       format.suffix);
#ifdef DWGSUPPORT
      if (format.dwg)
        f::dwgPolylineFile(path, fixture, format.version);
      else
        f::binaryDxfPolyline(path, fixture, format.version);
#else
      f::binaryDxfPolyline(path, fixture, format.version);
#endif
      const auto imported = stagedImport(path, format.lane);
      INFO(format.suffix);
      REQUIRE(imported.status == LC_OffsetImportSession::Status::Complete);
      REQUIRE(imported.graphic);
      REQUIRE(std::distance(imported.graphic->begin(),
                            imported.graphic->end()) == 3);
      const RS_Ellipse *ellipse = nullptr;
      std::vector<const RS_Line *> lines;
      for (const RS_Entity *entity : *imported.graphic) {
        CHECK(hasFragmentXData(*entity));
        CHECK(hasProjectionApp(*entity, "LibreCAD_POLYLINE_OCS"));
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
        if (const auto *candidate = dynamic_cast<const RS_Ellipse *>(entity)) {
          ellipse = candidate;
          CHECK(hasProjectionApp(*entity, "LibreCAD_CONIC_OCS"));
        }
        if (const auto *candidate = dynamic_cast<const RS_Line *>(entity))
          lines.push_back(candidate);
      }
      REQUIRE(ellipse);
      REQUIRE(lines.size() == 2);
      CHECK(ellipse->getAngleLength() == Catch::Approx(4.0 * std::atan(2.0)));
      CHECK(samePoint(ellipse->getStartpoint(), RS_Vector{1.8, 0.0}));
      CHECK(samePoint(ellipse->getEndpoint(), RS_Vector{1.8, 2.0}));
      const std::array<std::pair<RS_Vector, RS_Vector>, 2> expectedLines{{
          {{1.8, 2.0}, {0.2, 2.0}},
          {{0.2, 2.0}, {1.8, 0.0}}}};
      for (const auto &expected : expectedLines) {
        CHECK(std::any_of(lines.begin(), lines.end(), [&](const RS_Line *line) {
          return samePoint(line->getStartpoint(), expected.first) &&
                 samePoint(line->getEndpoint(), expected.second);
        }));
      }
    }

    const std::array<std::pair<const char *, double>, 2> closedNegativeBulges{{
        {"negative-minor", -std::tan(M_PI / 8.0)},
        {"negative-major", -2.0}}};
    for (const auto &variant : closedNegativeBulges) {
    for (const auto &format : bulgeFormats) {
#ifndef DWGSUPPORT
      if (format.dwg)
        continue;
#endif
      DRW_LWPolyline fixture = bulged;
      fixture.flags = 1;
      const double bulge = variant.second;
      fixture.vertlist.front()->bulge = bulge;
      const auto path = directory.file(std::string("closed-") + variant.first +
                                       "-" +
                                       format.suffix);
#ifdef DWGSUPPORT
      if (format.dwg)
        f::dwgPolylineFile(path, fixture, format.version);
      else
        f::binaryDxfPolyline(path, fixture, format.version);
#else
      f::binaryDxfPolyline(path, fixture, format.version);
#endif
      const auto imported = stagedImport(path, format.lane);
      INFO(format.suffix);
      REQUIRE(imported.status == LC_OffsetImportSession::Status::Complete);
      REQUIRE(imported.graphic);
      REQUIRE(std::distance(imported.graphic->begin(),
                            imported.graphic->end()) == 3);
      const RS_Ellipse *ellipse = nullptr;
      std::vector<const RS_Line *> lines;
      for (const RS_Entity *entity : *imported.graphic) {
        CHECK(hasFragmentXData(*entity));
        CHECK(hasProjectionApp(*entity, "LibreCAD_POLYLINE_OCS"));
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
        if (const auto *candidate = dynamic_cast<const RS_Ellipse *>(entity)) {
          ellipse = candidate;
          CHECK(hasProjectionApp(*entity, "LibreCAD_CONIC_OCS"));
        }
        if (const auto *candidate = dynamic_cast<const RS_Line *>(entity))
          lines.push_back(candidate);
      }
      REQUIRE(ellipse);
      REQUIRE(lines.size() == 2);
      CHECK(ellipse->getAngleLength() ==
            Catch::Approx(std::abs(4.0 * std::atan(bulge))));
      CHECK(samePoint(ellipse->getStartpoint(), RS_Vector{1.8, 0.0}));
      CHECK(samePoint(ellipse->getEndpoint(), RS_Vector{1.8, 2.0}));
      const std::array<std::pair<RS_Vector, RS_Vector>, 2> expectedLines{{
          {{1.8, 2.0}, {0.2, 2.0}},
          {{0.2, 2.0}, {1.8, 0.0}}}};
      for (const auto &expected : expectedLines) {
        CHECK(std::any_of(lines.begin(), lines.end(), [&](const RS_Line *line) {
          return samePoint(line->getStartpoint(), expected.first) &&
                 samePoint(line->getEndpoint(), expected.second);
        }));
      }
    }
    }

    for (const auto &format : bulgeFormats) {
#ifndef DWGSUPPORT
      if (format.dwg)
        continue;
#endif
      DRW_LWPolyline fixture = bulged;
      fixture.extPoint = {0.6, 0.0, -0.8};
      fixture.flags = 1;
      fixture.vertlist.front()->bulge = 2.0;
      const auto path = directory.file(std::string("closed-reflected-major-") +
                                       format.suffix);
#ifdef DWGSUPPORT
      if (format.dwg)
        f::dwgPolylineFile(path, fixture, format.version);
      else
        f::binaryDxfPolyline(path, fixture, format.version);
#else
      f::binaryDxfPolyline(path, fixture, format.version);
#endif
      const auto imported = stagedImport(path, format.lane);
      INFO(format.suffix);
      REQUIRE(imported.status == LC_OffsetImportSession::Status::Complete);
      REQUIRE(imported.graphic);
      REQUIRE(std::distance(imported.graphic->begin(),
                            imported.graphic->end()) == 3);
      const RS_Ellipse *ellipse = nullptr;
      std::vector<const RS_Line *> lines;
      for (const RS_Entity *entity : *imported.graphic) {
        CHECK(hasFragmentXData(*entity));
        CHECK(hasProjectionApp(*entity, "LibreCAD_POLYLINE_OCS"));
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
        if (const auto *candidate = dynamic_cast<const RS_Ellipse *>(entity)) {
          ellipse = candidate;
          CHECK(hasProjectionApp(*entity, "LibreCAD_CONIC_OCS"));
        }
        if (const auto *candidate = dynamic_cast<const RS_Line *>(entity))
          lines.push_back(candidate);
      }
      REQUIRE(ellipse);
      REQUIRE(lines.size() == 2);
      CHECK(ellipse->isReversed());
      CHECK(ellipse->getAngleLength() ==
            Catch::Approx(4.0 * std::atan(2.0)));
      CHECK(std::isfinite(ellipse->getCenter().x));
      CHECK(std::isfinite(ellipse->getCenter().y));
      const auto isClosedConnection = [&](const RS_Line *first,
                                          const RS_Line *second) {
        const auto matchesEllipseEnds = [&](const RS_Vector &left,
                                            const RS_Vector &right) {
          return (samePoint(left, ellipse->getStartpoint()) &&
                  samePoint(right, ellipse->getEndpoint())) ||
                 (samePoint(right, ellipse->getStartpoint()) &&
                  samePoint(left, ellipse->getEndpoint()));
        };
        return (samePoint(first->getStartpoint(), second->getStartpoint()) &&
                matchesEllipseEnds(first->getEndpoint(), second->getEndpoint())) ||
               (samePoint(first->getStartpoint(), second->getEndpoint()) &&
                matchesEllipseEnds(first->getEndpoint(), second->getStartpoint())) ||
               (samePoint(first->getEndpoint(), second->getStartpoint()) &&
                matchesEllipseEnds(first->getStartpoint(), second->getEndpoint())) ||
               (samePoint(first->getEndpoint(), second->getEndpoint()) &&
                matchesEllipseEnds(first->getStartpoint(), second->getStartpoint()));
      };
      CHECK(isClosedConnection(lines[0], lines[1]));
    }

    const std::array<std::pair<const char *, double>, 2>
        reflectedClosedNegativeBulges{{
            {"negative-minor", -std::tan(M_PI / 8.0)},
            {"negative-major", -2.0}}};
    for (const auto &variant : reflectedClosedNegativeBulges) {
    for (const auto &format : bulgeFormats) {
#ifndef DWGSUPPORT
      if (format.dwg)
        continue;
#endif
      DRW_LWPolyline fixture = bulged;
      fixture.extPoint = {0.6, 0.0, -0.8};
      fixture.flags = 1;
      const double bulge = variant.second;
      fixture.vertlist.front()->bulge = bulge;
      const auto path = directory.file(
          std::string("closed-reflected-") + variant.first + "-" +
          format.suffix);
#ifdef DWGSUPPORT
      if (format.dwg)
        f::dwgPolylineFile(path, fixture, format.version);
      else
        f::binaryDxfPolyline(path, fixture, format.version);
#else
      f::binaryDxfPolyline(path, fixture, format.version);
#endif
      const auto imported = stagedImport(path, format.lane);
      INFO(variant.first << " " << format.suffix);
      REQUIRE(imported.status == LC_OffsetImportSession::Status::Complete);
      REQUIRE(imported.graphic);
      REQUIRE(std::distance(imported.graphic->begin(),
                            imported.graphic->end()) == 3);
      const RS_Ellipse *ellipse = nullptr;
      std::vector<const RS_Line *> lines;
      for (const RS_Entity *entity : *imported.graphic) {
        CHECK(hasFragmentXData(*entity));
        CHECK(hasProjectionApp(*entity, "LibreCAD_POLYLINE_OCS"));
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
        if (const auto *candidate = dynamic_cast<const RS_Ellipse *>(entity)) {
          ellipse = candidate;
          CHECK(hasProjectionApp(*entity, "LibreCAD_CONIC_OCS"));
        }
        if (const auto *candidate = dynamic_cast<const RS_Line *>(entity))
          lines.push_back(candidate);
      }
      REQUIRE(ellipse);
      REQUIRE(lines.size() == 2);
      CHECK_FALSE(ellipse->isReversed());
      CHECK(ellipse->getAngleLength() ==
            Catch::Approx(std::abs(4.0 * std::atan(bulge))));
      CHECK(std::isfinite(ellipse->getCenter().x));
      CHECK(std::isfinite(ellipse->getCenter().y));
      const auto matchesEllipseEnds = [&](const RS_Vector &left,
                                          const RS_Vector &right) {
        return (samePoint(left, ellipse->getStartpoint()) &&
                samePoint(right, ellipse->getEndpoint())) ||
               (samePoint(right, ellipse->getStartpoint()) &&
                samePoint(left, ellipse->getEndpoint()));
      };
      const auto connected = [&](const RS_Line *first, const RS_Line *second) {
        return (samePoint(first->getStartpoint(), second->getStartpoint()) &&
                matchesEllipseEnds(first->getEndpoint(), second->getEndpoint())) ||
               (samePoint(first->getStartpoint(), second->getEndpoint()) &&
                matchesEllipseEnds(first->getEndpoint(), second->getStartpoint())) ||
               (samePoint(first->getEndpoint(), second->getStartpoint()) &&
                matchesEllipseEnds(first->getStartpoint(), second->getEndpoint())) ||
               (samePoint(first->getEndpoint(), second->getEndpoint()) &&
                matchesEllipseEnds(first->getStartpoint(), second->getStartpoint()));
      };
      CHECK(connected(lines[0], lines[1]));
    }
    }

    for (const auto &format : bulgeFormats) {
#ifndef DWGSUPPORT
      if (format.dwg)
        continue;
#endif
      DRW_LWPolyline fixture = raw;
      fixture.extPoint = {0.6, 0.0, -0.8};
      fixture.flags = 1;
      fixture.vertlist[0]->bulge = std::tan(M_PI / 8.0);
      fixture.vertlist[1]->bulge = -2.0;
      fixture.extData.push_back(std::make_shared<DRW_Variant>(
          1001, std::string("FixtureSourceData")));
      fixture.extData.push_back(std::make_shared<DRW_Variant>(
          1000, std::string("preserve-this-xdata")));
      const auto path = directory.file(
          std::string("closed-mixed-bulges-") + format.suffix);
#ifdef DWGSUPPORT
      if (format.dwg)
        f::dwgPolylineFile(path, fixture, format.version);
      else
        f::binaryDxfPolyline(path, fixture, format.version);
#else
      f::binaryDxfPolyline(path, fixture, format.version);
#endif
      const auto imported = stagedImport(path, format.lane);
      INFO(format.suffix);
      REQUIRE(imported.status == LC_OffsetImportSession::Status::Complete);
      REQUIRE(imported.graphic);
      REQUIRE(std::distance(imported.graphic->begin(),
                            imported.graphic->end()) == 3);

      std::vector<std::pair<RS_Vector, RS_Vector>> fragments;
      std::vector<const RS_Ellipse *> ellipses;
      std::vector<const RS_Line *> lines;
      for (const RS_Entity *entity : *imported.graphic) {
        CHECK(hasFragmentXData(*entity));
        CHECK(hasProjectionApp(*entity, "LibreCAD_POLYLINE_OCS"));
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
        if (const auto *ellipse = dynamic_cast<const RS_Ellipse *>(entity)) {
          ellipses.push_back(ellipse);
          fragments.emplace_back(ellipse->getStartpoint(), ellipse->getEndpoint());
          CHECK(hasProjectionApp(*entity, "LibreCAD_CONIC_OCS"));
        } else if (const auto *line = dynamic_cast<const RS_Line *>(entity)) {
          lines.push_back(line);
          fragments.emplace_back(line->getStartpoint(), line->getEndpoint());
        }
      }
      REQUIRE(ellipses.size() == 2);
      REQUIRE(lines.size() == 1);
      REQUIRE(fragments.size() == 3);
      std::array<double, 2> expectedSweeps{
          std::abs(4.0 * std::atan(fixture.vertlist[0]->bulge)),
          std::abs(4.0 * std::atan(fixture.vertlist[1]->bulge))};
      std::array<double, 2> actualSweeps{
          ellipses[0]->getAngleLength(), ellipses[1]->getAngleLength()};
      std::sort(expectedSweeps.begin(), expectedSweeps.end());
      std::sort(actualSweeps.begin(), actualSweeps.end());
      CHECK(actualSweeps[0] == Catch::Approx(expectedSweeps[0]));
      CHECK(actualSweeps[1] == Catch::Approx(expectedSweeps[1]));
      for (const auto *ellipse : ellipses) {
        CHECK(std::isfinite(ellipse->getCenter().x));
        CHECK(std::isfinite(ellipse->getCenter().y));
      }
      CHECK(std::count_if(ellipses.begin(), ellipses.end(),
                          [](const RS_Ellipse *ellipse) {
                            return ellipse->isReversed();
                          }) == 1);

      // A closed source chain must project to a cycle: each fragment endpoint
      // has exactly one matching endpoint on another fragment.
      for (std::size_t i = 0; i < fragments.size(); ++i) {
        for (const auto &endpoint : {fragments[i].first, fragments[i].second}) {
          std::size_t peers = 0;
          for (std::size_t j = 0; j < fragments.size(); ++j) {
            if (i == j) continue;
            peers += samePoint(endpoint, fragments[j].first) ? 1 : 0;
            peers += samePoint(endpoint, fragments[j].second) ? 1 : 0;
          }
          CHECK(peers == 1);
        }
      }
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

    DRW_Polyline closedBulged;
    closedBulged.basePoint = raw.basePoint;
    closedBulged.extPoint = raw.extPoint;
    closedBulged.flags = 1;
    closedBulged.addVertex(DRW_Vertex(0.0, 0.0, 0.0, 0.0));
    closedBulged.addVertex(DRW_Vertex(2.0, 0.0, 0.0, 0.0));
    closedBulged.addVertex(DRW_Vertex(2.0, 2.0, 0.0, 0.0));
    closedBulged.addVertex(
        DRW_Vertex(0.0, 2.0, 0.0, std::tan(M_PI / 16.0)));
    closedBulged.extData.push_back(std::make_shared<DRW_Variant>(
        1001, std::string("FixtureSourceData")));
    closedBulged.extData.push_back(std::make_shared<DRW_Variant>(
        1000, std::string("preserve-this-xdata")));
    for (const auto &format : formats) {
#ifndef DWGSUPPORT
      if (format.dwg)
        continue;
#endif
      DRW_Polyline fixture = closedBulged;
      for (auto &vertex : fixture.vertlist)
        vertex = std::make_shared<DRW_Vertex>(*vertex);
      const auto path = directory.file(std::string("closed-bulged-") +
                                       format.suffix);
#ifdef DWGSUPPORT
      if (format.dwg)
        f::dwg2dPolylineFile(path, fixture, format.version);
      else
        f::binaryDxf2dPolyline(path, fixture, format.version);
#else
      f::binaryDxf2dPolyline(path, fixture, format.version);
#endif
      const auto imported = stagedImport(path, format.lane);
      INFO(format.suffix);
      REQUIRE(imported.status == LC_OffsetImportSession::Status::Complete);
      REQUIRE(imported.graphic);
      REQUIRE(std::distance(imported.graphic->begin(),
                            imported.graphic->end()) == 4);
      const RS_Ellipse *ellipse = nullptr;
      std::vector<const RS_Line *> lines;
      for (const RS_Entity *entity : *imported.graphic) {
        CHECK(hasFixtureXData(*entity));
        CHECK(hasProjectionApp(*entity, "LibreCAD_POLYLINE_OCS"));
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(*entity));
        if (const auto *candidate = dynamic_cast<const RS_Ellipse *>(entity)) {
          ellipse = candidate;
          CHECK(hasProjectionApp(*entity, "LibreCAD_CONIC_OCS"));
        }
        if (const auto *candidate = dynamic_cast<const RS_Line *>(entity))
          lines.push_back(candidate);
      }
      REQUIRE(ellipse);
      REQUIRE(lines.size() == 3);
      CHECK(ellipse->getAngleLength() ==
            Catch::Approx(M_PI / 4.0));
      CHECK(samePoint(ellipse->getStartpoint(), RS_Vector{0.2, 0.0}));
      CHECK(samePoint(ellipse->getEndpoint(), RS_Vector{1.8, 0.0}));
      const std::array<std::pair<RS_Vector, RS_Vector>, 3> expectedLines{{
          {{1.8, 0.0}, {1.8, 2.0}},
          {{1.8, 2.0}, {0.2, 2.0}},
          {{0.2, 2.0}, {0.2, 0.0}}}};
      for (const auto &expected : expectedLines) {
        CHECK(std::any_of(lines.begin(), lines.end(), [&](const RS_Line *line) {
          return samePoint(line->getStartpoint(), expected.first) &&
                 samePoint(line->getEndpoint(), expected.second);
        }));
      }
    }

    for (const auto &format : formats) {
#ifndef DWGSUPPORT
      if (format.dwg)
        continue;
#endif
      DRW_Polyline fixture = bulged;
      fixture.extPoint = {0.6, 0.0, -0.8};
      for (auto &vertex : fixture.vertlist)
        vertex = std::make_shared<DRW_Vertex>(*vertex);
      const auto path = directory.file(std::string("reflected-bulged-") +
                                       format.suffix);
#ifdef DWGSUPPORT
      if (format.dwg)
        f::dwg2dPolylineFile(path, fixture, format.version);
      else
        f::binaryDxf2dPolyline(path, fixture, format.version);
#else
      f::binaryDxf2dPolyline(path, fixture, format.version);
#endif
      const auto imported = stagedImport(path, format.lane);
      INFO(format.suffix);
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
      CHECK(ellipse->isReversed());
      CHECK(ellipse->getAngleLength() == Catch::Approx(M_PI / 2.0));
      CHECK(std::isfinite(ellipse->getCenter().x));
      CHECK(std::isfinite(ellipse->getCenter().y));
      CHECK(std::hypot(ellipse->getEndpoint().x - line->getStartpoint().x,
                       ellipse->getEndpoint().y - line->getStartpoint().y) <
            1e-8);
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
    const auto side = GENERATE(LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right);
    const double signedDistance = side == LC_CurveOffsetSide::Left ? .5 : -.5;
    CAPTURE(static_cast<int>(side), signedDistance);
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{unweighted}, signedDistance, 1e-4, false};
    auto source = definition.source.makeEntity();
    REQUIRE(source);
    const auto options = LC_OffsetWorkerTolerance::directOptions(
        *source, std::abs(signedDistance), definition.relativeTolerance).value();
    const auto generated = LC_CurveOffset::buildDirectBranches(*source,
        LC_CurveOffset::makeSideRequest(side, std::abs(signedDistance)), options,
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
        RS_SplineData expectedCache;
        if (line) {
            expectedCache.degree = 1;
            expectedCache.controlPoints = {line->getStartpoint(), line->getEndpoint()};
            expectedCache.knotslist = {0, 0, 1, 1};
            expectedCache.weights = {1, 1};
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
            expectedCache = data;
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

        const auto input = directory.file(std::string(side == LC_CurveOffsetSide::Left
            ? "unweighted-left-" : "unweighted-right-") + suffix);
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
        const auto expectedDefinition = values::encodeSourceDefinition(definition);
        const auto importedDefinition = values::encodeSourceDefinition(
            *imported->sourceDefinition());
        REQUIRE(expectedDefinition);
        REQUIRE(importedDefinition);
        CHECK(QCborValue(*expectedDefinition).toCbor() ==
              QCborValue(*importedDefinition).toCbor());
        REQUIRE(std::holds_alternative<RS_SplineData>(imported->sourceDefinition()->source.value));
        CHECK(std::get<RS_SplineData>(imported->sourceDefinition()->source.value).weights.empty());
        CHECK(RS_Spline::geometryFingerprint(imported->getData()) ==
              RS_Spline::geometryFingerprint(expectedCache));

        const auto nativeOutput = directory.file(std::string("native-unweighted-") +
            (side == LC_CurveOffsetSide::Left ? "left-" : "right-") + suffix);
        REQUIRE(lc::test::exportDrawing(*outcome->graphic, nativeOutput, format(lane)));
        const auto nativeRoundTrip = stagedImport(nativeOutput, lane);
        INFO("native unweighted-spline writer round-trip " << suffix);
        REQUIRE(nativeRoundTrip.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(nativeRoundTrip.graphic);
        CHECK(nativeRoundTrip.admitted == 1);
        CHECK(nativeRoundTrip.downgraded == 0);
        const auto* roundTripped = dynamic_cast<const LC_OffsetCurve*>(
            f::sole(*nativeRoundTrip.graphic));
        REQUIRE(roundTripped);
        REQUIRE(roundTripped->sourceDefinition());
        const auto roundTripDefinition = values::encodeSourceDefinition(
            *roundTripped->sourceDefinition());
        REQUIRE(roundTripDefinition);
        CHECK(QCborValue(*expectedDefinition).toCbor() ==
              QCborValue(*roundTripDefinition).toCbor());
        CHECK(RS_Spline::geometryFingerprint(roundTripped->getData()) ==
              RS_Spline::geometryFingerprint(expectedCache));
    }
}

TEST_CASE("tangent-constrained spline sources promote through operation-8 validation across file lanes",
          "[lc-offsetcurve][import-session][typed-source][spline-variant][tangents][rational][spline-points]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-rational-tangent-spline-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    struct Format {
        Lane lane;
        bool dwg;
        DRW::Version version;
        const char* suffix;
    };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14-dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000-dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000-dwg"},
        {Lane::Binary18, false, DRW::AC1032, "r2018-dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018-dwg"}
    }};

    RS_SplineData splineData{3, false};
    splineData.controlPoints = {{0, 0}, {2, 3}, {6, 4}, {9, 0}};
    splineData.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    splineData.weights = {1.0, 0.75, 0.8, 1.0};
    splineData.fitTangentsConstrained = true;
    splineData.fitTolerance = 1e-2;
    splineData.fitStartTangent = {2, 3};
    splineData.fitEndTangent = {3, -4};
    RS_Spline evaluator{nullptr, splineData};
    for (const double parameter : {0.0, 0.2, 0.5, 0.8, 1.0}) {
        LC_CurveJet jet;
        REQUIRE(evaluator.tryEvaluateJet(parameter,
            parameter == 1.0 ? LC_CurveEvaluationSide::Left
                             : LC_CurveEvaluationSide::Right, jet));
        splineData.fitPoints.push_back(jet.point);
    }
    RS_Spline sourceSpline{nullptr, splineData};
    REQUIRE(sourceSpline.fitTangentsMatchCurve());

    const auto importVariant = [&](const LC_OffsetSourceGeometry& sourceGeometry,
                                   const std::string& variantName,
                                   const LC_CurveOffsetSide side) {
    const double signedDistance = side == LC_CurveOffsetSide::Left ? .05 : -.05;
    const LC_OffsetSourceDefinition definition{
        sourceGeometry, signedDistance, 1e-4, false};
    auto source = definition.source.makeEntity();
    REQUIRE(source);
    const auto options = LC_OffsetWorkerTolerance::directOptions(
        *source, std::abs(signedDistance), definition.relativeTolerance).value();
    const auto generated = LC_CurveOffset::buildDirectBranches(*source,
        LC_CurveOffset::makeSideRequest(side, std::abs(signedDistance)), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
    auto materialized = LC_CurveOffset::materializeBranches(*source, generated, options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(materialized.entities.size() == 1);
    const auto* cacheSpline = dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
    RS_SplineData cacheData;
    if (cacheSpline) {
        cacheData = cacheSpline->getData();
    } else if (const auto* cacheLine =
                   dynamic_cast<const RS_Line*>(materialized.entities.front().get())) {
        cacheData.degree = 1;
        cacheData.controlPoints = {cacheLine->getStartpoint(), cacheLine->getEndpoint()};
        cacheData.knotslist = {0.0, 0.0, 1.0, 1.0};
        cacheData.weights = {1.0, 1.0};
    } else {
        FAIL("offset cache entity must be a spline or an exact line");
    }

    for (const auto& fileFormat : formats) {
        const auto [lane, dwg, version, suffix] = fileFormat;
#ifndef DWGSUPPORT
        if (dwg) continue;
#endif
        xd::Definition carrier;
        carrier.source = definition;
        xd::Options encoding;
        if (dwg) {
            encoding.eed = true;
            if (version == DRW::AC1015) {
                encoding.textEncoding = xd::TextEncoding::Legacy;
                encoding.legacyEncodedBytes = [](std::string_view text)
                    -> std::optional<std::size_t> { return text.size(); };
            } else {
                encoding.textEncoding = xd::TextEncoding::Utf16;
            }
        } else {
            encoding.asciiDxf = false;
        }
        const auto tags = xd::forExport(
            {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
            carrier, encoding);
        REQUIRE(tags);

        DRW_Spline cache = f::raw(1, *tags);
        cache.controllist.clear();
        for (const auto& point : cacheData.controlPoints)
            cache.controllist.push_back(
                std::make_shared<DRW_Coord>(point.x, point.y, 0));
        cache.knotslist = cacheData.knotslist;
        cache.weightlist = cacheData.weights;
        cache.degree = cacheData.degree;
        cache.flags = 8;
        cache.m_scenario = 1;
        cache.ncontrol = cache.controllist.size();
        cache.nknots = cache.knotslist.size();
        cache.nfit = 0;

        const auto input = directory.file(variantName +
            (side == LC_CurveOffsetSide::Left ? "-left-" : "-right-") + suffix +
            (dwg ? ".dwg" : ".dxf"));
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
        const auto expectedDefinition = values::encodeSourceDefinition(definition);
        const auto importedDefinition = values::encodeSourceDefinition(
            *imported->sourceDefinition());
        REQUIRE(expectedDefinition);
        REQUIRE(importedDefinition);
        CHECK(QCborValue(*expectedDefinition).toCbor() ==
              QCborValue(*importedDefinition).toCbor());
        CHECK(RS_Spline::geometryFingerprint(imported->getData()) ==
              RS_Spline::geometryFingerprint(cacheData));
        const auto& importedGeometry = imported->sourceDefinition()->source;
        REQUIRE(importedGeometry.kind() == sourceGeometry.kind());
        if (const auto* expected = std::get_if<RS_SplineData>(&sourceGeometry.value)) {
            const auto* actual = std::get_if<RS_SplineData>(&importedGeometry.value);
            REQUIRE(actual);
            CHECK(actual->weights == expected->weights);
            CHECK(actual->fitTangentsConstrained);
            CHECK(actual->fitStartTangent == expected->fitStartTangent);
            CHECK(actual->fitEndTangent == expected->fitEndTangent);
        } else if (const auto* expected =
                       std::get_if<LC_SplinePointsData>(&sourceGeometry.value)) {
            const auto* actual =
                std::get_if<LC_SplinePointsData>(&importedGeometry.value);
            REQUIRE(actual);
            CHECK(actual->fitTangentsConstrained);
            CHECK(actual->fitStartTangent == expected->fitStartTangent);
            CHECK(actual->fitEndTangent == expected->fitEndTangent);
        } else {
            FAIL("test source must be a spline or spline-points variant");
        }

        if ((variantName == "cubic" || variantName == "quadratic" ||
             variantName == "linear" || variantName == "linear-one-ended" ||
             variantName == "spline-points") && side == LC_CurveOffsetSide::Left) {
            const auto nativeOutput = directory.file(variantName + "-native-" + suffix +
                (dwg ? ".dwg" : ".dxf"));
            REQUIRE(lc::test::exportDrawing(*outcome.graphic, nativeOutput,
                                             ::format(lane)));
            const auto nativeRoundTrip = stagedImport(nativeOutput, lane);
            INFO("native tangent-spline writer round-trip " << suffix);
            REQUIRE(nativeRoundTrip.status == LC_OffsetImportSession::Status::Complete);
            REQUIRE(nativeRoundTrip.graphic);
            CHECK(nativeRoundTrip.admitted == 1);
            CHECK(nativeRoundTrip.downgraded == 0);
            const auto* roundTripped = dynamic_cast<const LC_OffsetCurve*>(
                f::sole(*nativeRoundTrip.graphic));
            REQUIRE(roundTripped);
            REQUIRE(roundTripped->sourceDefinition());
            const auto roundTripDefinition = values::encodeSourceDefinition(
                *roundTripped->sourceDefinition());
            REQUIRE(roundTripDefinition);
            CHECK(QCborValue(*expectedDefinition).toCbor() ==
                  QCborValue(*roundTripDefinition).toCbor());
            CHECK(RS_Spline::geometryFingerprint(roundTripped->getData()) ==
                  RS_Spline::geometryFingerprint(cacheData));
        }
    }
    };
    const auto forBothSides = [&](const LC_OffsetSourceGeometry& geometry,
                                  const std::string& name) {
        for (const auto side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right})
            importVariant(geometry, name, side);
    };
    forBothSides(LC_OffsetSourceGeometry{splineData}, "cubic");

    RS_SplineData quadraticData{2, false};
    quadraticData.controlPoints = {{0, 0}, {2, 3}, {5, 0}};
    quadraticData.knotslist = {0, 0, 0, 1, 1, 1};
    quadraticData.weights = {1.0, 0.7, 1.0};
    quadraticData.fitTangentsConstrained = true;
    quadraticData.fitTolerance = 1e-2;
    quadraticData.fitStartTangent = {2, 3};
    quadraticData.fitEndTangent = {3, -3};
    RS_Spline quadraticEvaluator{nullptr, quadraticData};
    for (const double parameter : {0.0, 0.2, 0.5, 0.8, 1.0}) {
        LC_CurveJet jet;
        REQUIRE(quadraticEvaluator.tryEvaluateJet(parameter,
            parameter == 1.0 ? LC_CurveEvaluationSide::Left
                             : LC_CurveEvaluationSide::Right, jet));
        quadraticData.fitPoints.push_back(jet.point);
    }
    RS_Spline quadraticSpline{nullptr, quadraticData};
    REQUIRE(quadraticSpline.fitTangentsMatchCurve());
    forBothSides(LC_OffsetSourceGeometry{quadraticData}, "quadratic");

    RS_SplineData linearData{1, false};
    linearData.controlPoints = {{0, 0}, {4, 0}};
    linearData.knotslist = {0, 0, 1, 1};
    linearData.weights = {1.0, 1.0};
    linearData.fitPoints = {{0, 0}, {2, 0}, {4, 0}};
    linearData.fitTangentsConstrained = true;
    linearData.fitTolerance = 1e-8;
    linearData.fitStartTangent = {1, 0};
    linearData.fitEndTangent = {2, 0};
    RS_Spline linearSpline{nullptr, linearData};
    REQUIRE(linearSpline.fitTangentsMatchCurve());
    forBothSides(LC_OffsetSourceGeometry{linearData}, "linear");

    RS_SplineData oneEndedLinearData = linearData;
    oneEndedLinearData.fitEndTangent = {0, 0};
    RS_Spline oneEndedLinearSpline{nullptr, oneEndedLinearData};
    REQUIRE(oneEndedLinearSpline.fitTangentsMatchCurve());
    forBothSides(LC_OffsetSourceGeometry{oneEndedLinearData},
                 "linear-one-ended");

    LC_SplinePointsData tangentPointsData{false, false};
    tangentPointsData.splinePoints = {
        {0, 0}, {1, 2}, {3, 1.5}, {4, 0}};
    LC_SplinePoints tangentPoints{nullptr, tangentPointsData};
    LC_CurveJet pointsStart;
    LC_CurveJet pointsEnd;
    REQUIRE(tangentPoints.tryEvaluateJet(
        0.0, LC_CurveEvaluationSide::Right, pointsStart));
    REQUIRE(tangentPoints.tryEvaluateJet(
        static_cast<double>(tangentPoints.getSegmentCount()),
        LC_CurveEvaluationSide::Left, pointsEnd));
    tangentPoints.getData().fitTangentsConstrained = true;
    tangentPoints.getData().fitTolerance = 1e-7;
    tangentPoints.getData().fitStartTangent = pointsStart.first;
    tangentPoints.getData().fitEndTangent = pointsEnd.first;
    tangentPointsData = tangentPoints.getData();
    REQUIRE(tangentPoints.fitTangentsMatchCurve());
    forBothSides(LC_OffsetSourceGeometry{tangentPointsData}, "spline-points");
}

TEST_CASE("wrapped periodic spline source promotes on both sides across file lanes",
          "[lc-offsetcurve][import-session][typed-source][spline-variant]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-wrapped-spline-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    const int degree = GENERATE(2, 3, 4, 5, 6);
    const bool sixControlLayout = GENERATE(false, true);
    RS_Spline periodic{nullptr, RS_SplineData{degree, false}};
    if (degree <= 3 && sixControlLayout) {
        for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{30, -15},
                                       RS_Vector{60, 0}, RS_Vector{55, 35},
                                       RS_Vector{20, 50}, RS_Vector{-15, 25}})
            periodic.addControlPoint(point);
    } else if (degree <= 3) {
        for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{40, -10},
                                       RS_Vector{60, 30}, RS_Vector{20, 50},
                                       RS_Vector{-15, 25}})
            periodic.addControlPoint(point);
    } else {
        const std::size_t count = static_cast<std::size_t>(degree) +
                                  (sixControlLayout ? 3 : 2);
        for (std::size_t i = 0; i < count; ++i) {
            const double angle = 2.0 * M_PI * static_cast<double>(i) /
                                 static_cast<double>(count);
            periodic.addControlPoint({50.0 * std::cos(angle),
                                      35.0 * std::sin(angle)});
        }
    }
    periodic.setClosed(true);
    auto wrapped = periodic.getData();
    const bool rational = GENERATE(false, true);
    if (rational) {
        std::vector<double> baseWeights;
        if (degree <= 3) {
            baseWeights = sixControlLayout
                ? std::vector<double>{1.0, 0.9, 1.1, 0.95, 1.05, 0.85}
                : std::vector<double>{1.0, 0.9, 1.1, 0.95, 1.05};
        } else {
            const std::size_t count = static_cast<std::size_t>(degree) +
                                      (sixControlLayout ? 3 : 2);
            for (std::size_t i = 0; i < count; ++i)
                baseWeights.push_back(.85 + .05 * static_cast<double>(i));
        }
        REQUIRE(wrapped.controlPoints.size() == baseWeights.size() + degree);
        wrapped.weights = baseWeights;
        wrapped.weights.insert(wrapped.weights.end(), baseWeights.begin(),
                               baseWeights.begin() + degree);
    } else {
        wrapped.weights.clear();
    }
    const double scale = GENERATE(1e-3, 1.0, 1e3);
    const RS_Vector translation{1e6 * scale, -2e6 * scale};
    CAPTURE(scale, translation.x, translation.y);
    for (auto& point : wrapped.controlPoints) {
        point.x = point.x * scale + translation.x;
        point.y = point.y * scale + translation.y;
        point.z *= scale;
    }
    for (auto& point : wrapped.fitPoints) {
        point.x = point.x * scale + translation.x;
        point.y = point.y * scale + translation.y;
        point.z *= scale;
    }
    const double distanceFactor = GENERATE(0.125, 0.25);
    const auto side = GENERATE(LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right);
    const double magnitude = distanceFactor * scale;
    const double signedDistance = side == LC_CurveOffsetSide::Left ? magnitude : -magnitude;
    CAPTURE(degree, sixControlLayout, rational, distanceFactor,
            static_cast<int>(side), signedDistance);
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{wrapped}, signedDistance, 1e-4, false};
    auto source = definition.source.makeEntity();
    REQUIRE(source);
    const auto options = LC_OffsetWorkerTolerance::directOptions(
        *source, magnitude, definition.relativeTolerance).value();
    const auto generated = LC_CurveOffset::buildDirectBranches(*source,
        LC_CurveOffset::makeSideRequest(side, magnitude), options,
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

        const auto input = directory.file(std::string("scale-") +
            std::to_string(scale) + "-distance-" + std::to_string(distanceFactor) +
            "-degree-" + std::to_string(degree) +
            (degree <= 3 ? (sixControlLayout ? "-six-control" : "-five-control")
                         : (sixControlLayout ? "-plus-three-control" : "-plus-two-control")) +
            (rational ? "-rational" : "-unit") +
            (side == LC_CurveOffsetSide::Left
                ? "-wrapped-left-" : "-wrapped-right-") + suffix);
#ifdef DWGSUPPORT
        if (dwg) {
            f::dwgFile(input, cache, version);
            preserveTypedSourceDwg(input,
                "wrapped-periodic-" + input.filename().string());
        }
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

        const auto nativeOutput = directory.file("native-" + input.filename().string());
        REQUIRE(lc::test::exportDrawing(*outcome->graphic, nativeOutput, format(lane)));
        const auto nativeRoundTrip = stagedImport(nativeOutput, lane);
        INFO("native wrapped-periodic writer round-trip " << suffix);
        REQUIRE(nativeRoundTrip.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(nativeRoundTrip.graphic);
        CHECK(nativeRoundTrip.admitted == 1);
        CHECK(nativeRoundTrip.downgraded == 0);
        const auto* roundTripped = dynamic_cast<const LC_OffsetCurve*>(
            f::sole(*nativeRoundTrip.graphic));
        REQUIRE(roundTripped);
        REQUIRE(roundTripped->sourceDefinition());
        const auto roundTripSource = values::encodeSourceDefinition(
            *roundTripped->sourceDefinition());
        REQUIRE(roundTripSource);
        CHECK(QCborValue(*expectedSource).toCbor() ==
              QCborValue(*roundTripSource).toCbor());
        CHECK(RS_Spline::geometryFingerprint(roundTripped->getData()) ==
              RS_Spline::geometryFingerprint(expectedCache));

        if (dwg) {
            const char* odaDirectory =
                std::getenv("LIBRECAD_WRAPPED_PERIODIC_ODA_DXF_DIR");
            if (odaDirectory != nullptr && *odaDirectory != '\0') {
                auto odaPath = std::filesystem::path(odaDirectory) /
                    ("wrapped-periodic-" + input.filename().string());
                odaPath.replace_extension(".dxf");
                INFO(odaPath.string());
                REQUIRE(std::filesystem::is_regular_file(odaPath));
                const auto converted = f::readCodec(odaPath);
                REQUIRE(converted.splines.size() == 1);
                const auto& convertedSpline = converted.splines.front();
                REQUIRE(convertedSpline.controllist.size() ==
                        expectedCache.controlPoints.size());
                REQUIRE(convertedSpline.knotslist.size() ==
                        expectedCache.knotslist.size());
                if (convertedSpline.weightlist.empty()) {
                    CHECK(std::all_of(expectedCache.weights.begin(),
                                      expectedCache.weights.end(),
                                      [](double weight) { return weight == 1.0; }));
                } else {
                    REQUIRE(convertedSpline.weightlist.size() ==
                            expectedCache.weights.size());
                    for (std::size_t i = 0; i < expectedCache.weights.size(); ++i)
                        CHECK(convertedSpline.weightlist[i] == expectedCache.weights[i]);
                }
                for (std::size_t i = 0; i < expectedCache.controlPoints.size(); ++i) {
                    REQUIRE(convertedSpline.controllist[i]);
                    const double tolerance = 1e-9 * std::max(1.0, scale);
                    CHECK(std::hypot(
                              convertedSpline.controllist[i]->x -
                                  expectedCache.controlPoints[i].x,
                              convertedSpline.controllist[i]->y -
                                  expectedCache.controlPoints[i].y) <= tolerance);
                }
                for (std::size_t i = 0; i < expectedCache.knotslist.size(); ++i)
                    CHECK(convertedSpline.knotslist[i] == expectedCache.knotslist[i]);

                const auto deviation =
                    sampledSplineDeviation(expectedCache, convertedSpline, 128);
                REQUIRE(deviation);
                CHECK(*deviation <=
                      periodicSplineGeometryTolerance(expectedCache, scale));

                xd::Options dxfEncoding;
                dxfEncoding.asciiDxf = true;
                const auto convertedMetadata = xd::parseAndStrip(
                    convertedSpline.extData, dxfEncoding);
                REQUIRE(convertedMetadata.status == xd::Status::Schema2);
                REQUIRE(convertedMetadata.definition);
                REQUIRE(convertedMetadata.definition->source);
                const auto convertedSource = values::encodeSourceDefinition(
                    *convertedMetadata.definition->source);
                REQUIRE(convertedSource);
                CHECK(QCborValue(*expectedSource).toCbor() ==
                      QCborValue(*convertedSource).toCbor());
            }
        }
    }
}

TEST_CASE("nonuniform rational periodic spline import follows output topology",
          "[lc-offsetcurve][import-session][typed-source][spline-variant][refusal]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-periodic-spline-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    const int layout = GENERATE(0, 1, 2, 3, 4);
    const double scale = GENERATE(1e-3, 1.0, 1e3);
    const double distanceFactor = layout >= 3 ? 0.005 : GENERATE(0.0625, 0.125, 0.25);
    const int degree = layout >= 2 ? 2 : 1;
    const int controlCount = layout == 0 ? 6 : layout == 1 ? 8 : 7;
    const std::string degreeName = "degree-" + std::to_string(degree);
    const std::string profileName = layout == 3 ? "gradual" :
                                    layout == 4 ? "reversed-gradual" : "mild";
    CAPTURE(scale, distanceFactor, degree, controlCount);
    RS_Spline periodic{nullptr, RS_SplineData{degree, false}};
    const std::vector<RS_Vector> controlPoints = layout == 0
        ? std::vector<RS_Vector>{{0, 0}, {40, -10}, {60, 30}, {55, 45},
                                 {20, 50}, {-15, 25}}
        : layout == 1
            ? std::vector<RS_Vector>{{0, 0}, {30, -15}, {60, 0}, {70, 20},
                                     {55, 35}, {20, 50}, {-15, 25}, {-10, 10}}
            : std::vector<RS_Vector>{{0, 0}, {25, -15}, {55, -5}, {70, 20},
                                     {50, 45}, {15, 50}, {-15, 25}};
    REQUIRE(controlPoints.size() == static_cast<std::size_t>(controlCount));
    for (const RS_Vector& point : controlPoints)
        periodic.addControlPoint({point.x * scale + 1000.0 * scale,
                                  point.y * scale - 2000.0 * scale});
    if (layout == 0)
        periodic.setKnotVector({0, 0, .5, 1.5, 2.5, 4, 5, 5});
    else if (layout == 1)
        periodic.setKnotVector({0, 0, .4, 1.1, 1.8, 2.6, 3.4, 4.3, 5.5, 5.5});
    else
        periodic.setKnotVector({0, 0, 0, .5, 1.5, 3, 4.5, 5, 5, 5});
    REQUIRE(periodic.validate());
    periodic.setClosed(true);
    auto periodicData = periodic.getData();
    const std::vector<double> independentWeights = layout == 3
        ? std::vector<double>{1e-6, 1e-4, 1e-2, 1.0, 1e2, 1e4, 1e6}
        : layout == 4
        ? std::vector<double>{1e6, 1e4, 1e2, 1.0, 1e-2, 1e-4, 1e-6}
        : layout == 0
        ? std::vector<double>{1.0, 0.9, 1.1, 0.95, 1.05, 0.85}
        : layout == 1
            ? std::vector<double>{1.0, 0.9, 1.1, 0.95, 1.05, 0.85, 1.15, 0.8}
            : std::vector<double>{1.0, 0.9, 1.1, 0.95, 1.05, 0.85, 1.15};
    periodicData.weights = independentWeights;
    periodicData.weights.insert(periodicData.weights.end(), independentWeights.begin(),
                                independentWeights.begin() + periodicData.degree);
    RS_Spline sourceCheck{nullptr, periodicData};
    REQUIRE(sourceCheck.validate());
    REQUIRE(sourceCheck.hasWrappedControlPoints());
    double startParameter = 0.0;
    double endParameter = 0.0;
    REQUIRE(sourceCheck.getParameterDomain(startParameter, endParameter));
    LC_CurveJet startJet;
    LC_CurveJet endJet;
    REQUIRE(sourceCheck.tryEvaluateJet(startParameter, LC_CurveEvaluationSide::Right,
                                       startJet));
    REQUIRE(sourceCheck.tryEvaluateJet(endParameter, LC_CurveEvaluationSide::Left,
                                       endJet));
    CHECK(startJet.point.distanceTo(endJet.point) < 1e-8);
    if (degree > 1)
        CHECK(startJet.first.distanceTo(endJet.first) < 1e-7);
    const auto side = layout >= 3
        ? LC_CurveOffsetSide::Right
        : GENERATE(LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right);
    const double magnitude = distanceFactor * scale;
    const double signedDistance = side == LC_CurveOffsetSide::Left
        ? magnitude : -magnitude;
    CAPTURE(static_cast<int>(side), signedDistance);
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{periodicData}, signedDistance, 1e-4, false};
    auto source = definition.source.makeEntity();
    REQUIRE(source);
    const auto options = LC_OffsetWorkerTolerance::directOptions(
        *source, magnitude, definition.relativeTolerance).value();
    const auto generated = LC_CurveOffset::buildDirectBranches(
        *source, LC_CurveOffset::makeSideRequest(side, magnitude), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
    const auto materialized = LC_CurveOffset::materializeBranches(
        *source, generated, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
    REQUIRE_FALSE(materialized.entities.empty());
    const auto* firstSpline = materialized.entities.size() == 1
        ? dynamic_cast<const RS_Spline*>(materialized.entities.front().get())
        : nullptr;
    const bool singleSplineCache = firstSpline && firstSpline->validate();
    RS_SplineData carrierCache;
    if (singleSplineCache) {
        carrierCache = firstSpline->getData();
    } else {
        for (const auto& entity : materialized.entities)
            CHECK((dynamic_cast<const RS_Line*>(entity.get()) ||
                   dynamic_cast<const RS_Spline*>(entity.get())));
        if (const auto* firstLine =
                dynamic_cast<const RS_Line*>(materialized.entities.front().get())) {
            carrierCache = RS_SplineData{1, false};
            carrierCache.controlPoints = {
                firstLine->getStartpoint(), firstLine->getEndpoint()};
            carrierCache.knotslist = {0, 0, 1, 1};
            carrierCache.weights = {1, 1};
        } else {
            const auto* firstPiece =
                dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
            REQUIRE(firstPiece);
            carrierCache = firstPiece->getData();
        }
        RS_Spline cacheCheck{nullptr, carrierCache};
        REQUIRE(cacheCheck.validate());
    }

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
#ifndef DWGSUPPORT
        if (fileFormat.dwg) continue;
#endif
        xd::Definition carrier;
        carrier.source = definition;
        xd::Options encoding;
        if (fileFormat.dwg) {
            encoding.eed = true;
            encoding.textEncoding = xd::TextEncoding::Utf16;
        } else {
            encoding.asciiDxf = false;
        }
        const auto tags = xd::forExport(
            {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
            carrier, encoding);
        REQUIRE(tags);
        DRW_Spline raw = f::raw(1, *tags);
        raw.controllist.clear();
        for (const auto& point : carrierCache.controlPoints)
            raw.controllist.push_back(
                std::make_shared<DRW_Coord>(point.x, point.y, 0));
        raw.knotslist = carrierCache.knotslist;
        raw.weightlist = carrierCache.weights;
        raw.degree = carrierCache.degree;
        raw.flags = 8;
        raw.m_scenario = 1;
        raw.ncontrol = raw.controllist.size();
        raw.nknots = raw.knotslist.size();
        raw.nfit = 0;

        const auto input = directory.file(
            profileName + "-controls-" + std::to_string(controlCount) + "-scale-" +
            std::to_string(scale) + "-distance-" +
            std::to_string(distanceFactor) +
            (side == LC_CurveOffsetSide::Left ? "-left-" : "-right-") +
            fileFormat.suffix);
#ifdef DWGSUPPORT
        if (fileFormat.dwg) {
            f::dwgFile(input, raw, fileFormat.version);
            preserveTypedSourceDwg(input, degreeName + "-periodic-" + input.filename().string());
        } else f::binaryDxf(input, raw, fileFormat.version);
#else
        f::binaryDxf(input, raw, fileFormat.version);
#endif
        const auto outcome = stagedImport(input, fileFormat.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        const auto expected = values::encodeSourceDefinition(definition);
        REQUIRE(expected);
        const auto* imported = f::sole(*outcome.graphic);
        REQUIRE(imported);
        if (singleSplineCache) {
            CHECK(outcome.admitted == 1);
            CHECK(outcome.downgraded == 0);
            const auto* native = dynamic_cast<const LC_OffsetCurve*>(imported);
            REQUIRE(native);
            REQUIRE(native->sourceDefinition());
            const auto received = values::encodeSourceDefinition(
                *native->sourceDefinition());
            REQUIRE(received);
            CHECK(QCborValue(*expected).toCbor() == QCborValue(*received).toCbor());
            CHECK(RS_Spline::geometryFingerprint(native->getData()) ==
                  RS_Spline::geometryFingerprint(carrierCache));
        } else {
            CHECK(outcome.admitted == 0);
            CHECK(outcome.downgraded == 1);
            const auto* fallback = dynamic_cast<const RS_Spline*>(imported);
            REQUIRE(fallback);
            CHECK(dynamic_cast<const LC_OffsetCurve*>(fallback) == nullptr);
            CHECK(RS_Spline::geometryFingerprint(fallback->getData()) ==
                  RS_Spline::geometryFingerprint(carrierCache));
            const auto parsed = xd::parseAndStrip(fallback->getDrwExtData(), encoding);
            REQUIRE(parsed.status == xd::Status::Schema2);
            REQUIRE(parsed.definition);
            REQUIRE(parsed.definition->source);
            const auto received = values::encodeSourceDefinition(
                *parsed.definition->source);
            REQUIRE(received);
            CHECK(QCborValue(*expected).toCbor() == QCborValue(*received).toCbor());
        }

        if (fileFormat.dwg) {
            const char* odaDirectory =
                std::getenv("LIBRECAD_NONUNIFORM_PERIODIC_ODA_DXF_DIR");
            if (odaDirectory != nullptr && *odaDirectory != '\0') {
                auto odaPath = std::filesystem::path(odaDirectory) /
                    (degreeName + "-periodic-" + input.filename().string());
                odaPath.replace_extension(".dxf");
                INFO(odaPath.string());
                REQUIRE(std::filesystem::is_regular_file(odaPath));
                const auto converted = f::readCodec(odaPath);
                REQUIRE(converted.splines.size() == 1);
                const auto& convertedSpline = converted.splines.front();
                REQUIRE(convertedSpline.controllist.size() ==
                        carrierCache.controlPoints.size());
                REQUIRE(convertedSpline.knotslist.size() == carrierCache.knotslist.size());
                if (convertedSpline.weightlist.empty()) {
                    CHECK(std::all_of(carrierCache.weights.begin(),
                                      carrierCache.weights.end(),
                                      [](double weight) { return weight == 1.0; }));
                } else {
                    REQUIRE(convertedSpline.weightlist.size() ==
                            carrierCache.weights.size());
                    for (std::size_t i = 0; i < carrierCache.weights.size(); ++i)
                        CHECK(convertedSpline.weightlist[i] == carrierCache.weights[i]);
                }
                for (std::size_t i = 0; i < carrierCache.controlPoints.size(); ++i) {
                    REQUIRE(convertedSpline.controllist[i]);
                    const double tolerance = 1e-9 * std::max(1.0, scale);
                    CHECK(std::hypot(
                              convertedSpline.controllist[i]->x -
                                  carrierCache.controlPoints[i].x,
                              convertedSpline.controllist[i]->y -
                                  carrierCache.controlPoints[i].y) <= tolerance);
                }
                for (std::size_t i = 0; i < carrierCache.knotslist.size(); ++i)
                    CHECK(convertedSpline.knotslist[i] == carrierCache.knotslist[i]);

                const auto deviation =
                    sampledSplineDeviation(carrierCache, convertedSpline, 128);
                REQUIRE(deviation);
                CHECK(*deviation <=
                      periodicSplineGeometryTolerance(carrierCache, scale));

                xd::Options dxfEncoding;
                dxfEncoding.asciiDxf = true;
                const auto convertedMetadata = xd::parseAndStrip(
                    convertedSpline.extData, dxfEncoding);
                REQUIRE(convertedMetadata.status == xd::Status::Schema2);
                REQUIRE(convertedMetadata.definition);
                REQUIRE(convertedMetadata.definition->source);
                const auto convertedSource = values::encodeSourceDefinition(
                    *convertedMetadata.definition->source);
                REQUIRE(convertedSource);
                CHECK(QCborValue(*expected).toCbor() ==
                      QCborValue(*convertedSource).toCbor());
            }
        }
    }
}

TEST_CASE("high-ratio open rational spline offsets round trip through DXF and DWG",
          "[lc-offsetcurve][import-session][typed-source][spline-variant][rational][conditioned]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-conditioned-open-rational-spline");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    const std::array<std::vector<double>, 2> weightProfiles{{
        {1e-6, 3.162277660168379e-5, 1e-3, 0.03162277660168379, 1.0,
         31.622776601683793, 1e3, 31622.776601683792, 1e6},
        {1e6, 31622.776601683792, 1e3, 31.622776601683793, 1.0,
         0.03162277660168379, 1e-3, 3.162277660168379e-5, 1e-6}
    }};
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

    for (std::size_t profileIndex = 0; profileIndex < weightProfiles.size();
         ++profileIndex) {
        CAPTURE(profileIndex);
        RS_SplineData source{2, false};
        source.controlPoints = {{0, 0}, {10, 5}, {20, 12}, {30, 20},
                                {40, 28}, {50, 35}, {60, 40}, {70, 42},
                                {80, 40}};
        source.knotslist = {0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 7};
        source.weights = weightProfiles[profileIndex];
        RS_Spline sourceCheck{nullptr, source};
        REQUIRE(sourceCheck.validate());

        for (const double signedDistance : {-.005, .005}) {
            CAPTURE(signedDistance);
            const LC_OffsetSourceDefinition definition{
                LC_OffsetSourceGeometry{source}, signedDistance, 1e-4, false};
            auto sourceEntity = definition.source.makeEntity();
            REQUIRE(sourceEntity);
            const auto options = LC_OffsetWorkerTolerance::directOptions(
                *sourceEntity, std::abs(signedDistance), definition.relativeTolerance);
            REQUIRE(options);
            const auto generated = LC_CurveOffset::buildDirectBranches(
                *sourceEntity,
                LC_CurveOffset::makeSideRequest(
                    signedDistance > 0 ? LC_CurveOffsetSide::Left
                                       : LC_CurveOffsetSide::Right,
                    std::abs(signedDistance)),
                *options, LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
            const auto materialized = LC_CurveOffset::materializeBranches(
                *sourceEntity, generated, *options,
                LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
            REQUIRE(materialized.entities.size() == 1);
            const auto* cacheEntity = dynamic_cast<const RS_Spline*>(
                materialized.entities.front().get());
            REQUIRE(cacheEntity);
            REQUIRE(cacheEntity->validate());
            const RS_SplineData cache = cacheEntity->getData();

            for (const auto& format : formats) {
#ifndef DWGSUPPORT
                if (format.dwg) continue;
#endif
                xd::Definition carrier;
                carrier.source = definition;
                xd::Options encoding;
                if (format.dwg) {
                    encoding.eed = true;
                    encoding.textEncoding = xd::TextEncoding::Utf16;
                } else {
                    encoding.asciiDxf = false;
                }
                const auto tags = xd::forExport(
                    {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
                    carrier, encoding);
                REQUIRE(tags);
                DRW_Spline raw = f::raw(1, *tags);
                raw.controllist.clear();
                for (const auto& point : cache.controlPoints)
                    raw.controllist.push_back(
                        std::make_shared<DRW_Coord>(point.x, point.y, 0));
                raw.knotslist = cache.knotslist;
                raw.weightlist = cache.weights;
                raw.degree = cache.degree;
                raw.flags = 8;
                raw.m_scenario = 1;
                raw.ncontrol = raw.controllist.size();
                raw.nknots = raw.knotslist.size();
                raw.nfit = 0;

                const std::string basename = "profile-" +
                    std::to_string(profileIndex) +
                    (signedDistance > 0 ? "-left-" : "-right-");
                const auto input = directory.file(basename + format.suffix);
#ifdef DWGSUPPORT
                if (format.dwg) {
                    f::dwgFile(input, raw, format.version);
                    preserveTypedSourceDwg(input, "conditioned-open-" +
                        basename + format.suffix);
                } else {
                    f::binaryDxf(input, raw, format.version);
                }
#else
                f::binaryDxf(input, raw, format.version);
#endif
                const auto outcome = stagedImport(input, format.lane);
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
                      RS_Spline::geometryFingerprint(cache));

                const auto nativeOutput = directory.file(
                    "native-" + basename + format.suffix);
                REQUIRE(lc::test::exportDrawing(
                    *outcome.graphic, nativeOutput, ::format(format.lane)));
                const auto nativeRoundTrip = stagedImport(
                    nativeOutput, format.lane);
                INFO("conditioned open-rational native writer round-trip "
                     << basename << format.suffix);
                REQUIRE(nativeRoundTrip.status ==
                        LC_OffsetImportSession::Status::Complete);
                REQUIRE(nativeRoundTrip.graphic);
                CHECK(nativeRoundTrip.admitted == 1);
                CHECK(nativeRoundTrip.downgraded == 0);
                const auto* roundTripped = dynamic_cast<const LC_OffsetCurve*>(
                    f::sole(*nativeRoundTrip.graphic));
                REQUIRE(roundTripped);
                REQUIRE(roundTripped->sourceDefinition());
                const auto roundTripSource = values::encodeSourceDefinition(
                    *roundTripped->sourceDefinition());
                REQUIRE(roundTripSource);
                CHECK(QCborValue(*expectedSource).toCbor() ==
                      QCborValue(*roundTripSource).toCbor());
                CHECK(RS_Spline::geometryFingerprint(roundTripped->getData()) ==
                      RS_Spline::geometryFingerprint(cache));

                if (format.dwg) {
                    const char* odaDirectory = std::getenv(
                        "LIBRECAD_CONDITIONED_RATIONAL_ODA_DXF_DIR");
                    if (odaDirectory != nullptr && *odaDirectory != '\0') {
                        auto odaPath = std::filesystem::path(odaDirectory) /
                            ("conditioned-open-" + basename + format.suffix);
                        odaPath.replace_extension(".dxf");
                        INFO(odaPath.string());
                        REQUIRE(std::filesystem::is_regular_file(odaPath));
                        const auto converted = f::readCodec(odaPath);
                        REQUIRE(converted.splines.size() == 1);
                        const auto& convertedSpline = converted.splines.front();
                        const auto deviation = sampledSplineDeviation(
                            cache, convertedSpline, 256);
                        REQUIRE(deviation);
                        CHECK(*deviation <= 1e-5);

                        xd::Options dxfEncoding;
                        dxfEncoding.asciiDxf = true;
                        const auto convertedMetadata = xd::parseAndStrip(
                            convertedSpline.extData, dxfEncoding);
                        REQUIRE(convertedMetadata.status == xd::Status::Schema2);
                        REQUIRE(convertedMetadata.definition);
                        REQUIRE(convertedMetadata.definition->source);
                        const auto convertedSource = values::encodeSourceDefinition(
                            *convertedMetadata.definition->source);
                        REQUIRE(convertedSource);
                        CHECK(QCborValue(*expectedSource).toCbor() ==
                              QCborValue(*convertedSource).toCbor());
                    }
                }
            }
        }
    }
}

TEST_CASE("nonuniform periodic spline knots preserve seam through staged import",
          "[lc-offsetcurve][import-session][typed-source][spline-variant]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-nonuniform-periodic-spline-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    RS_Spline periodic{nullptr, RS_SplineData{3, false}};
    for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{30, -15},
                                   RS_Vector{60, 0}, RS_Vector{55, 35},
                                   RS_Vector{20, 50}, RS_Vector{-15, 25}})
        periodic.addControlPoint(point);
    periodic.setKnotVector({0, 0, 0, 0, 0.5, 2, 3, 3, 3, 3});
    REQUIRE(periodic.validate());
    periodic.setClosed(true);
    auto periodicData = periodic.getData();
    const std::vector<double> baseWeights{1.0, 0.9, 1.1, 0.95, 1.05, 0.85};
    periodicData.weights = baseWeights;
    periodicData.weights.insert(periodicData.weights.end(), baseWeights.begin(),
                                baseWeights.begin() + periodicData.degree);
    RS_Spline sourceCheck{nullptr, periodicData};
    REQUIRE(sourceCheck.validate());
    REQUIRE(sourceCheck.hasWrappedControlPoints());
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(sourceCheck.getParameterDomain(t0, t1));
    LC_CurveJet startJet;
    LC_CurveJet endJet;
    REQUIRE(sourceCheck.tryEvaluateJet(t0, LC_CurveEvaluationSide::Right, startJet));
    REQUIRE(sourceCheck.tryEvaluateJet(t1, LC_CurveEvaluationSide::Left, endJet));
    CHECK(startJet.point.distanceTo(endJet.point) < 1e-8);
    CHECK(startJet.first.distanceTo(endJet.first) < 1e-7);
    CHECK(periodicData.knotslist != std::vector<double>{0, 1, 2, 3, 4, 5, 6,
                                                        7, 8, 9, 10, 11, 12});

    const auto side = GENERATE(LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right);
    constexpr double magnitude = 0.125;
    const double signedDistance = side == LC_CurveOffsetSide::Left
        ? magnitude : -magnitude;
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{periodicData}, signedDistance, 1e-4, false};
    auto source = definition.source.makeEntity();
    REQUIRE(source);
    const auto options = LC_OffsetWorkerTolerance::directOptions(
        *source, magnitude, definition.relativeTolerance).value();
    const auto generated = LC_CurveOffset::buildDirectBranches(
        *source, LC_CurveOffset::makeSideRequest(side, magnitude), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
    const auto materialized = LC_CurveOffset::materializeBranches(
        *source, generated, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(materialized.entities.size() == 1);
    const auto* expectedCacheEntity =
        dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
    REQUIRE(expectedCacheEntity);
    REQUIRE(expectedCacheEntity->validate());
    const RS_SplineData expectedCache = expectedCacheEntity->getData();

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
#ifndef DWGSUPPORT
        if (fileFormat.dwg) continue;
#endif
        xd::Definition carrier;
        carrier.source = definition;
        xd::Options encoding;
        if (fileFormat.dwg) {
            encoding.eed = true;
            encoding.textEncoding = xd::TextEncoding::Utf16;
        } else {
            encoding.asciiDxf = false;
        }
        const auto tags = xd::forExport(
            {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
            carrier, encoding);
        REQUIRE(tags);
        DRW_Spline raw = f::raw(1, *tags);
        raw.controllist.clear();
        for (const auto& point : expectedCache.controlPoints)
            raw.controllist.push_back(
                std::make_shared<DRW_Coord>(point.x, point.y, 0));
        raw.knotslist = expectedCache.knotslist;
        raw.weightlist = expectedCache.weights;
        raw.degree = expectedCache.degree;
        raw.flags = 8;
        raw.m_scenario = 1;
        raw.ncontrol = raw.controllist.size();
        raw.nknots = raw.knotslist.size();
        raw.nfit = 0;

        const auto input = directory.file(
            std::string(side == LC_CurveOffsetSide::Left ? "left-" : "right-") +
            fileFormat.suffix);
#ifdef DWGSUPPORT
        if (fileFormat.dwg) {
            f::dwgFile(input, raw, fileFormat.version);
            preserveTypedSourceDwg(input, "nonuniform-periodic-" + input.filename().string());
        } else f::binaryDxf(input, raw, fileFormat.version);
#else
        f::binaryDxf(input, raw, fileFormat.version);
#endif
        const auto outcome = stagedImport(input, fileFormat.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        CHECK(outcome.admitted == 1);
        CHECK(outcome.downgraded == 0);
        const auto* imported = dynamic_cast<const LC_OffsetCurve*>(
            f::sole(*outcome.graphic));
        REQUIRE(imported);
        REQUIRE(imported->sourceDefinition());
        const auto expected = values::encodeSourceDefinition(definition);
        const auto received = values::encodeSourceDefinition(
            *imported->sourceDefinition());
        REQUIRE(expected);
        REQUIRE(received);
        CHECK(QCborValue(*expected).toCbor() == QCborValue(*received).toCbor());
        CHECK(RS_Spline::geometryFingerprint(imported->getData()) ==
              RS_Spline::geometryFingerprint(expectedCache));
    }
}

TEST_CASE("repeated-knot rational spline imports preserve exact cache eligibility",
          "[lc-offsetcurve][import-session][typed-source][spline-variant][repeated-knot]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-repeated-knot-spline-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    RS_SplineData splineData{3, false};
    splineData.controlPoints = {
        {0, 0}, {1, 2}, {2, 2}, {3, -1}, {4, -1}, {5, 1}};
    splineData.knotslist = {0, 0, 0, 0, .4, .4, 1, 1, 1, 1};
    splineData.weights = {1, .7, 1.4, .9, 1.6, 1};
    RS_Spline sourceCheck{nullptr, splineData};
    REQUIRE(sourceCheck.validate());

    const auto side = GENERATE(LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right);
    constexpr double magnitude = 0.05;
    const double signedDistance = side == LC_CurveOffsetSide::Left
        ? magnitude : -magnitude;
    CAPTURE(static_cast<int>(side), signedDistance);
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{splineData}, signedDistance, 1e-4, false};
    auto source = definition.source.makeEntity();
    REQUIRE(source);
    const auto options = LC_OffsetWorkerTolerance::directOptions(
        *source, magnitude, definition.relativeTolerance).value();
    const auto generated = LC_CurveOffset::buildDirectBranches(
        *source, LC_CurveOffset::makeSideRequest(side, magnitude), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
    const auto materialized = LC_CurveOffset::materializeBranches(
        *source, generated, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(materialized.entities.size() == 1);
    const auto* cacheEntity =
        dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
    REQUIRE(cacheEntity);
    REQUIRE(cacheEntity->validate());
    const RS_SplineData cacheData = cacheEntity->getData();

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
#ifndef DWGSUPPORT
        if (fileFormat.dwg) continue;
#endif
        xd::Definition carrier;
        carrier.source = definition;
        xd::Options encoding;
        if (fileFormat.dwg) {
            encoding.eed = true;
            encoding.textEncoding = xd::TextEncoding::Utf16;
        } else {
            encoding.asciiDxf = false;
        }
        const auto tags = xd::forExport(
            {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
            carrier, encoding);
        REQUIRE(tags);
        DRW_Spline raw = f::raw(1, *tags);
        raw.controllist.clear();
        for (const auto& point : cacheData.controlPoints)
            raw.controllist.push_back(
                std::make_shared<DRW_Coord>(point.x, point.y, 0));
        raw.knotslist = cacheData.knotslist;
        raw.weightlist = cacheData.weights;
        raw.degree = cacheData.degree;
        raw.flags = 8;
        raw.m_scenario = 1;
        raw.ncontrol = raw.controllist.size();
        raw.nknots = raw.knotslist.size();
        raw.nfit = 0;

        const auto input = directory.file(
            std::string(side == LC_CurveOffsetSide::Left ? "left-" : "right-") +
            fileFormat.suffix);
#ifdef DWGSUPPORT
        if (fileFormat.dwg) {
            f::dwgFile(input, raw, fileFormat.version);
            preserveTypedSourceDwg(input, "repeated-knot-" + input.filename().string());
        } else f::binaryDxf(input, raw, fileFormat.version);
#else
        f::binaryDxf(input, raw, fileFormat.version);
#endif
        const auto outcome = stagedImport(input, fileFormat.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        CHECK(outcome.admitted == 1);
        CHECK(outcome.downgraded == 0);
        const auto* imported = dynamic_cast<const LC_OffsetCurve*>(
            f::sole(*outcome.graphic));
        REQUIRE(imported);
        REQUIRE(imported->sourceDefinition());
        const auto expected = values::encodeSourceDefinition(definition);
        const auto received = values::encodeSourceDefinition(
            *imported->sourceDefinition());
        REQUIRE(expected);
        REQUIRE(received);
        CHECK(QCborValue(*expected).toCbor() == QCborValue(*received).toCbor());
        CHECK(RS_Spline::geometryFingerprint(imported->getData()) ==
              RS_Spline::geometryFingerprint(cacheData));

        const auto nativeOutput = directory.file(
            std::string(side == LC_CurveOffsetSide::Left ? "native-left-" : "native-right-") +
            fileFormat.suffix);
        REQUIRE(lc::test::exportDrawing(*outcome.graphic, nativeOutput,
                                        ::format(fileFormat.lane)));
        const auto roundTrip = stagedImport(nativeOutput, fileFormat.lane);
        INFO("repeated-knot native writer round-trip " << nativeOutput.filename().string());
        REQUIRE(roundTrip.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(roundTrip.graphic);
        CHECK(roundTrip.admitted == 1);
        CHECK(roundTrip.downgraded == 0);
        const auto* copy = dynamic_cast<const LC_OffsetCurve*>(
            f::sole(*roundTrip.graphic));
        REQUIRE(copy);
        REQUIRE(copy->sourceDefinition());
        const auto roundTrippedSource = values::encodeSourceDefinition(
            *copy->sourceDefinition());
        REQUIRE(roundTrippedSource);
        CHECK(QCborValue(*expected).toCbor() ==
              QCborValue(*roundTrippedSource).toCbor());
        CHECK(RS_Spline::geometryFingerprint(copy->getData()) ==
              RS_Spline::geometryFingerprint(cacheData));
    }
}

TEST_CASE("C2 rational knot cusps through degree six round-trip through native file writers",
          "[lc-offsetcurve][import-session][typed-source][spline-variant][c2-knot][persistence]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-c2-knot-native-roundtrip");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    struct Format {
        Lane lane;
        DRW::Version version;
        const char* suffix;
        bool dwg;
    };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, DRW::AC1014, "r14.dxf", false},
        {Lane::Binary2000, DRW::AC1015, "r2000.dxf", false},
        {Lane::Eed2000, DRW::AC1015, "r2000.dwg", true},
        {Lane::Binary18, DRW::AC1032, "r2018.dxf", false},
        {Lane::Eed18, DRW::AC1032, "r2018.dwg", true}
    }};

    LC_OffsetWorkerClient generationWorker;
    qint64 nextRequestId = 1;
    const std::array<double, 7> weightPattern{1.0, 1.02, .98, 1.03,
                                              .97, 1.01, .99};
    const std::array<std::pair<double, double>, 14> knotAndCurvatureCases{
        {{.125, -1.0},
         {.23, -1.0},
         {.25, -1.0},
         {.37, -1.0},
         {.5, -1.0},
         {.625, -1.0},
         {.75, -1.0},
         {.125, 1.0},
         {.23, 1.0},
         {.25, 1.0},
         {.37, 1.0},
         {.5, 1.0},
         {.625, 1.0},
         {.75, 1.0}}};
    for (const int degree : {3, 4, 5, 6}) {
      for (const auto &[knot, curvatureSign] : knotAndCurvatureCases) {
        const int firstMultiplicity = knot == .5 ? 1 : degree - 2;
        for (int multiplicity = firstMultiplicity; multiplicity <= degree - 2;
             ++multiplicity) {
          CAPTURE(degree, knot, curvatureSign, multiplicity);
          RS_SplineData sourceData{degree, false};
          const int controlCount = degree + 1 + multiplicity;
          for (int i = 0; i < controlCount; ++i) {
            const double x = static_cast<double>(i);
            const double y =
                curvatureSign * (i == degree - 3 ? -5.0 : .001 * x * x * x);
            sourceData.controlPoints.emplace_back(x, y);
            sourceData.weights.push_back(
                weightPattern[static_cast<std::size_t>(i) %
                              weightPattern.size()]);
          }
          sourceData.knotslist.assign(static_cast<std::size_t>(degree + 1),
                                      0.0);
          sourceData.knotslist.insert(sourceData.knotslist.end(),
                                      static_cast<std::size_t>(multiplicity),
                                      knot);
          sourceData.knotslist.insert(sourceData.knotslist.end(),
                                      static_cast<std::size_t>(degree + 1),
                                      1.0);
          RS_Spline sourceCheck{nullptr, sourceData};
          REQUIRE(sourceCheck.validate());
          LC_CurveJet leftJet;
          LC_CurveJet rightJet;
          REQUIRE(sourceCheck.tryEvaluateJet(knot, LC_CurveEvaluationSide::Left,
                                             leftJet));
          REQUIRE(sourceCheck.tryEvaluateJet(
              knot, LC_CurveEvaluationSide::Right, rightJet));
          const auto curvature = [](const LC_CurveJet &jet) {
            const double speedSquared =
                jet.first.x * jet.first.x + jet.first.y * jet.first.y;
            return (jet.first.x * jet.second.y - jet.first.y * jet.second.x) /
                   (speedSquared * std::sqrt(speedSquared));
          };
          const double signedDistance =
              2.0 / (curvature(leftJet) + curvature(rightJet));
          REQUIRE(std::isfinite(signedDistance));
          REQUIRE(std::abs(signedDistance) > 1e-8);

          const LC_OffsetSourceDefinition definition{
              LC_OffsetSourceGeometry{sourceData}, signedDistance, 1e-4, false};
          const auto directOptions =
              LC_OffsetWorkerTolerance::directOptions(definition, sourceCheck);
          REQUIRE(directOptions);
          LC_CurveOffsetCuspIsolationDiagnostics cuspDiagnostics;
          const auto direct = LC_CurveOffset::buildDirectBranches(
              sourceCheck,
              LC_CurveOffset::makeSideRequest(signedDistance > 0
                                                  ? LC_CurveOffsetSide::Left
                                                  : LC_CurveOffsetSide::Right,
                                              std::abs(signedDistance)),
              *directOptions, LC_CurveOffset::makeDirectSourceBudget(),
              &cuspDiagnostics);
          REQUIRE(direct.status == LC_CurveOffsetStatus::Ok);
          const double knotPosition = knot;
          const auto knotCandidate = std::find_if(
              cuspDiagnostics.candidateBoxes.begin(),
              cuspDiagnostics.candidateBoxes.end(),
              [knotPosition](const LC_CurveOffsetCuspCandidateBox &candidate) {
                return std::abs(candidate.parameter - knotPosition) < 1e-8;
              });
          REQUIRE(knotCandidate != cuspDiagnostics.candidateBoxes.end());
          CHECK(knotCandidate->kind ==
                LC_CurveOffsetCuspCandidateKind::CertifiedUniqueBracket);

          LC_OffsetWorkerRequest::Request request;
          request.token = {nextRequestId++, 1, 1, 1};
          request.operation =
              LC_OffsetWorkerRequest::GenerateSource{definition, {}};
          std::optional<LC_OffsetWorkerClient::Outcome> generatedOutcome;
          REQUIRE(generationWorker.submit(
              request, LC_OffsetWorkerClient::Mode::Commit,
              [&](LC_OffsetWorkerClient::Outcome outcome) {
                generatedOutcome.emplace(std::move(outcome));
              }));
          QElapsedTimer elapsed;
          elapsed.start();
          while (!generatedOutcome) {
            REQUIRE(elapsed.elapsed() <
                    LC_OffsetWorkerClient::Deadlines::commit + 5000);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QThread::msleep(1);
          }
          REQUIRE(generatedOutcome->status ==
                  LC_OffsetWorkerClient::Status::Completed);
          REQUIRE(generatedOutcome->completed);
          REQUIRE(generatedOutcome->completed->isJoined());
          const auto &workerReply = generatedOutcome->completed->reply();
          REQUIRE(workerReply.operation == 7);
          REQUIRE(std::holds_alternative<LC_OffsetWorkerResult::Native>(
              workerReply.result));
          const auto &generated =
              std::get<LC_OffsetWorkerResult::Native>(workerReply.result);
          CAPTURE(degree, knot, curvatureSign, signedDistance,
                  generated.engineStatus, generated.ordinaryStatus,
                  generated.refusal);
          REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
          REQUIRE(generated.ordinaryStatus ==
                  LC_OffsetWorkerResult::OrdinaryStatus::Complete);
          RS_SplineData cacheData;
          REQUIRE(LC_OffsetWorkerSourceCache::combine(generated.ordinary,
                                                      cacheData));
          RS_Spline cacheCheck{nullptr, cacheData};
          REQUIRE(cacheCheck.validate());

          for (const auto &fileFormat : formats) {
#ifndef DWGSUPPORT
            if (fileFormat.dwg)
              continue;
#endif
            CAPTURE(degree, knot, curvatureSign, signedDistance,
                    fileFormat.suffix);
            xd::Definition carrier;
            carrier.source = definition;
            xd::Options encoding;
            if (fileFormat.dwg) {
              encoding.eed = true;
              encoding.textEncoding = xd::TextEncoding::Utf16;
            } else {
              encoding.asciiDxf = false;
            }
            const auto tags = xd::forExport(
                {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
                carrier, encoding);
            REQUIRE(tags);
            DRW_Spline raw = f::raw(1, *tags);
            raw.controllist.clear();
            for (const auto &point : cacheData.controlPoints)
              raw.controllist.push_back(
                  std::make_shared<DRW_Coord>(point.x, point.y, 0));
            raw.knotslist = cacheData.knotslist;
            raw.weightlist = cacheData.weights;
            raw.degree = cacheData.degree;
            raw.flags = 8;
            raw.m_scenario = 1;
            raw.ncontrol = static_cast<std::int32_t>(raw.controllist.size());
            raw.nknots = static_cast<std::int32_t>(raw.knotslist.size());
            raw.nfit = 0;

            const std::string side =
                std::to_string(degree) + "-mult-" +
                std::to_string(multiplicity) + "-knot-" + std::to_string(knot) +
                (curvatureSign < 0 ? "-negative-" : "-positive-");
            const auto input = directory.file(side + fileFormat.suffix);
            writeInput(input, fileFormat.lane, raw);
            const auto imported = stagedImport(input, fileFormat.lane);
            REQUIRE(imported.status ==
                    LC_OffsetImportSession::Status::Complete);
            REQUIRE(imported.graphic);
            CHECK(imported.admitted == 1);
            CHECK(imported.downgraded == 0);
            const auto *native = dynamic_cast<const LC_OffsetCurve *>(
                f::sole(*imported.graphic));
            REQUIRE(native);
            REQUIRE(native->sourceDefinition());
            const auto expectedSource =
                values::encodeSourceDefinition(definition);
            const auto importedSource =
                values::encodeSourceDefinition(*native->sourceDefinition());
            REQUIRE(expectedSource);
            REQUIRE(importedSource);
            CHECK(QCborValue(*expectedSource).toCbor() ==
                  QCborValue(*importedSource).toCbor());
            CHECK(RS_Spline::geometryFingerprint(native->getData()) ==
                  RS_Spline::geometryFingerprint(cacheData));

            const auto output =
                directory.file("native-" + side + fileFormat.suffix);
            REQUIRE(lc::test::exportDrawing(*imported.graphic, output,
                                            ::format(fileFormat.lane)));
            if (fileFormat.dwg)
              preserveTypedSourceDwg(output,
                                     "c2-native-" + side + fileFormat.suffix);
            const auto roundTrip = stagedImport(output, fileFormat.lane);
            REQUIRE(roundTrip.status ==
                    LC_OffsetImportSession::Status::Complete);
            REQUIRE(roundTrip.graphic);
            CHECK(roundTrip.admitted == 1);
            CHECK(roundTrip.downgraded == 0);
            const auto *copy = dynamic_cast<const LC_OffsetCurve *>(
                f::sole(*roundTrip.graphic));
            REQUIRE(copy);
            REQUIRE(copy->sourceDefinition());
            const auto roundTrippedSource =
                values::encodeSourceDefinition(*copy->sourceDefinition());
            REQUIRE(roundTrippedSource);
            CHECK(QCborValue(*expectedSource).toCbor() ==
                  QCborValue(*roundTrippedSource).toCbor());
            CHECK(RS_Spline::geometryFingerprint(copy->getData()) ==
                  RS_Spline::geometryFingerprint(cacheData));
          }
        }
      }
    }
}

TEST_CASE("Multi-knot C2 rational splines preserve typed source through native file lanes",
          "[lc-offsetcurve][import-session][typed-source][spline-variant][multi-knot][persistence]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-multi-knot-native-roundtrip");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    struct Format {
        Lane lane;
        DRW::Version version;
        const char* suffix;
        bool dwg;
    };
    const std::array<Format, 5> formats{{{Lane::Binary14, DRW::AC1014, "r14.dxf", false},
                                         {Lane::Binary2000, DRW::AC1015, "r2000.dxf", false},
                                         {Lane::Eed2000, DRW::AC1015, "r2000.dwg", true},
                                         {Lane::Binary18, DRW::AC1032, "r2018.dxf", false},
                                         {Lane::Eed18, DRW::AC1032, "r2018.dwg", true}}};
    const std::array<double, 11> weightPattern{1.0, 1.02, .98, 1.03, .97, 1.01, .99, 1.04, .96, 1.02, .98};
    const std::array<double, 11> alternateWeightPattern{.72, 1.38, .81, 1.27, .93, 1.46, .76, 1.19, 1.08, .84, 1.31};
    const std::array<double, 11> highContrastWeightPattern{.25, 2.0, .4, 2.5, .35, 2.2, .3, 2.8, 1.7, .45, 2.4};
    const std::array<double, 11> wideRatioWeightPattern{.05, 20.0, .1, 10.0, .2, 15.0, .08, 18.0, 5.0, .12, 12.0};
    LC_OffsetWorkerClient worker;
    qint64 requestId = 1;
    const int layoutProfile = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18);
    const std::array<std::vector<double>, 4> knotLayouts{{{.25, .5}, {.25, .65}, {.2, .45, .7}, {.12, .31, .58, .82}}};
    const bool fourKnotWeightProfile = layoutProfile >= 3;
    const auto& interiorKnots = knotLayouts[static_cast<std::size_t>(fourKnotWeightProfile ? 3 : layoutProfile)];
    const std::size_t targetKnotIndex = layoutProfile == 8 || layoutProfile == 11 || layoutProfile == 12 || layoutProfile == 16 ? 0
        : layoutProfile == 6 || layoutProfile == 9 || layoutProfile == 13 || layoutProfile == 17                                ? 1
        : layoutProfile == 7 || layoutProfile == 10 || layoutProfile == 14 || layoutProfile == 18                               ? 2
                                                                                                  : interiorKnots.size() - 1;
    const double targetKnot = interiorKnots[targetKnotIndex];
    const bool alternateWeights = layoutProfile == 4 || layoutProfile == 6 || layoutProfile == 7 || layoutProfile == 11;
    const bool highContrastWeights = layoutProfile == 5 || (layoutProfile >= 12 && layoutProfile <= 14);
    const bool wideRatioWeights = layoutProfile >= 15;
    const auto& selectedWeightPattern = alternateWeights ? alternateWeightPattern
        : highContrastWeights                            ? highContrastWeightPattern
        : wideRatioWeights                               ? wideRatioWeightPattern
                                                         : weightPattern;
    const char* weightProfileName = alternateWeights ? "alternate-weights-"
        : highContrastWeights                        ? "high-contrast-weights-"
        : wideRatioWeights                           ? "wide-ratio-weights-"
                                                     : "default-weights-";

    for (const int degree : {3, 4, 5, 6}) {
        const int multiplicity = degree - 2;
        const int controlCount = degree + 1 + static_cast<int>(interiorKnots.size()) * multiplicity;
        for (const double curvatureSign : {-1.0, 1.0}) {
            CAPTURE(degree, multiplicity, curvatureSign, interiorKnots, targetKnot, weightProfileName);
            RS_SplineData sourceData{degree, false};
            const int curvatureControl = static_cast<int>(targetKnotIndex) * multiplicity + degree - 3;
            for (int i = 0; i < controlCount; ++i) {
                const double x = static_cast<double>(i);
                const double y = curvatureSign * (i == curvatureControl ? -5.0 : .001 * x * x * x);
                sourceData.controlPoints.emplace_back(x, y);
                sourceData.weights.push_back(selectedWeightPattern[static_cast<std::size_t>(i) % selectedWeightPattern.size()]);
            }
            sourceData.knotslist.assign(static_cast<std::size_t>(degree + 1), 0.0);
            for (const double knot : interiorKnots)
                sourceData.knotslist.insert(sourceData.knotslist.end(), static_cast<std::size_t>(multiplicity), knot);
            sourceData.knotslist.insert(sourceData.knotslist.end(), static_cast<std::size_t>(degree + 1), 1.0);
            RS_Spline sourceCheck{nullptr, sourceData};
            REQUIRE(sourceCheck.validate());
            LC_CurveJet leftJet;
            LC_CurveJet rightJet;
            REQUIRE(sourceCheck.tryEvaluateJet(targetKnot, LC_CurveEvaluationSide::Left, leftJet));
            REQUIRE(sourceCheck.tryEvaluateJet(targetKnot, LC_CurveEvaluationSide::Right, rightJet));
            const auto curvature = [](const LC_CurveJet& jet) {
                const double speedSquared = jet.first.x * jet.first.x + jet.first.y * jet.first.y;
                return (jet.first.x * jet.second.y - jet.first.y * jet.second.x) / (speedSquared * std::sqrt(speedSquared));
            };
            const double signedDistance = 2.0 / (curvature(leftJet) + curvature(rightJet));
            REQUIRE(std::isfinite(signedDistance));
            REQUIRE(std::abs(signedDistance) > 1e-8);

            const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{sourceData}, signedDistance, 1e-4, false};
            LC_OffsetWorkerRequest::Request request;
            request.token = {requestId++, 1, 1, 1};
            request.operation = LC_OffsetWorkerRequest::GenerateSource{definition, {}};
            std::optional<LC_OffsetWorkerClient::Outcome> outcome;
            REQUIRE(worker.submit(request, LC_OffsetWorkerClient::Mode::Commit,
                                  [&](LC_OffsetWorkerClient::Outcome value) { outcome.emplace(std::move(value)); }));
            QElapsedTimer elapsed;
            elapsed.start();
            while (!outcome) {
                REQUIRE(elapsed.elapsed() < LC_OffsetWorkerClient::Deadlines::commit + 5000);
                QCoreApplication::processEvents(QEventLoop::AllEvents);
                QThread::msleep(1);
            }
            REQUIRE(outcome->status == LC_OffsetWorkerClient::Status::Completed);
            REQUIRE(outcome->completed);
            REQUIRE(outcome->completed->isJoined());
            const auto& reply = outcome->completed->reply();
            REQUIRE(reply.operation == 7);
            REQUIRE(std::holds_alternative<LC_OffsetWorkerResult::Native>(reply.result));
            const auto& generated = std::get<LC_OffsetWorkerResult::Native>(reply.result);
            REQUIRE(generated.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(generated.ordinaryStatus == LC_OffsetWorkerResult::OrdinaryStatus::Complete);
            RS_SplineData cacheData;
            REQUIRE(LC_OffsetWorkerSourceCache::combine(generated.ordinary, cacheData));
            RS_Spline cacheCheck{nullptr, cacheData};
            REQUIRE(cacheCheck.validate());

            for (const auto& fileFormat : formats) {
#ifndef DWGSUPPORT
                if (fileFormat.dwg)
                    continue;
#endif
                CAPTURE(fileFormat.suffix);
                xd::Definition carrier;
                carrier.source = definition;
                xd::Options encoding;
                if (fileFormat.dwg) {
                    encoding.eed = true;
                    encoding.textEncoding = xd::TextEncoding::Utf16;
                }
                else {
                    encoding.asciiDxf = false;
                }
                const auto tags = xd::forExport({std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))}, carrier, encoding);
                REQUIRE(tags);
                DRW_Spline raw = f::raw(1, *tags);
                raw.controllist.clear();
                for (const auto& point : cacheData.controlPoints)
                    raw.controllist.push_back(std::make_shared<DRW_Coord>(point.x, point.y, 0));
                raw.knotslist = cacheData.knotslist;
                raw.weightlist = cacheData.weights;
                raw.degree = cacheData.degree;
                raw.flags = 8;
                raw.m_scenario = 1;
                raw.ncontrol = static_cast<std::int32_t>(raw.controllist.size());
                raw.nknots = static_cast<std::int32_t>(raw.knotslist.size());
                raw.nfit = 0;

                std::string knotLabel;
                for (const double knot : interiorKnots)
                    knotLabel += std::to_string(static_cast<int>(std::lround(knot * 100.0))) + "-";
                const std::string fixtureName = "degree-" + std::to_string(degree) + "-knots-" + knotLabel + "target-" +
                    std::to_string(static_cast<int>(std::lround(targetKnot * 100.0))) + "-" + weightProfileName +
                    (curvatureSign < 0 ? "negative-" : "positive-");
                const auto input = directory.file(fixtureName + fileFormat.suffix);
#ifdef DWGSUPPORT
                if (fileFormat.dwg)
                    f::dwgFile(input, raw, fileFormat.version);
                else
                    f::binaryDxf(input, raw, fileFormat.version);
#else
                f::binaryDxf(input, raw, fileFormat.version);
#endif
                const auto imported = stagedImport(input, fileFormat.lane);
                REQUIRE(imported.status == LC_OffsetImportSession::Status::Complete);
                REQUIRE(imported.graphic);
                CHECK(imported.admitted == 1);
                CHECK(imported.downgraded == 0);
                const auto* native = dynamic_cast<const LC_OffsetCurve*>(f::sole(*imported.graphic));
                REQUIRE(native);
                REQUIRE(native->sourceDefinition());
                const auto expected = values::encodeSourceDefinition(definition);
                const auto received = values::encodeSourceDefinition(*native->sourceDefinition());
                REQUIRE(expected);
                REQUIRE(received);
                CHECK(QCborValue(*expected).toCbor() == QCborValue(*received).toCbor());
                CHECK(RS_Spline::geometryFingerprint(native->getData()) == RS_Spline::geometryFingerprint(cacheData));

                const auto output = directory.file("native-" + fixtureName + fileFormat.suffix);
                REQUIRE(lc::test::exportDrawing(*imported.graphic, output, ::format(fileFormat.lane)));
                if (fileFormat.dwg)
                    preserveTypedSourceDwg(output, "multi-knot-" + fixtureName + fileFormat.suffix);
                const auto reopened = stagedImport(output, fileFormat.lane);
                REQUIRE(reopened.status == LC_OffsetImportSession::Status::Complete);
                REQUIRE(reopened.graphic);
                CHECK(reopened.admitted == 1);
                CHECK(reopened.downgraded == 0);
                const auto* copy = dynamic_cast<const LC_OffsetCurve*>(f::sole(*reopened.graphic));
                REQUIRE(copy);
                REQUIRE(copy->sourceDefinition());
                const auto roundTripped = values::encodeSourceDefinition(*copy->sourceDefinition());
                REQUIRE(roundTripped);
                CHECK(QCborValue(*expected).toCbor() == QCborValue(*roundTripped).toCbor());
                CHECK(RS_Spline::geometryFingerprint(copy->getData()) == RS_Spline::geometryFingerprint(cacheData));
            }
        }
    }
}

TEST_CASE(
    "hyperbola import follows output topology on both sides across file lanes",
    "[lc-offsetcurve][import-session][typed-source][hyperbola][refusal]") {
  Setup setup;
  lc::test::OutputDirectory directory("offset-hyperbola-multibranch-import");
  namespace xd = LC_OffsetCurveXData;
  namespace values = LC_OffsetWorkerRequest::Values;

  const LC_OffsetSourceGeometry sourceGeometry{
      LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false}};
  const auto side =
      GENERATE(LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right);
  const double signedDistance = side == LC_CurveOffsetSide::Left ? 1.1 : -1.1;
  CAPTURE(static_cast<int>(side), signedDistance);
  const LC_OffsetSourceDefinition definition{sourceGeometry, signedDistance,
                                             1e-4, false};
  auto source = definition.source.makeEntity();
  REQUIRE(source);
  const double magnitude = std::abs(definition.signedDistance);
  const auto options = LC_OffsetWorkerTolerance::directOptions(
                           *source, magnitude, definition.relativeTolerance)
                           .value();
  const auto generated = LC_CurveOffset::buildDirectBranches(
      *source, LC_CurveOffset::makeSideRequest(side, magnitude), options,
      LC_CurveOffset::makeDirectSourceBudget());
  REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
  const auto materialized = LC_CurveOffset::materializeBranches(
      *source, generated, options, LC_CurveOffset::makeDirectSourceBudget());
  REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
  REQUIRE_FALSE(materialized.entities.empty());
  const bool singleCache = materialized.entities.size() == 1;
  CHECK(materialized.entities.size() ==
        (side == LC_CurveOffsetSide::Left ? std::size_t{1} : std::size_t{3}));
  const auto *firstBranch =
      dynamic_cast<const RS_Spline *>(materialized.entities.front().get());
  REQUIRE(firstBranch);
  const RS_SplineData firstBranchCache = firstBranch->getData();
  REQUIRE(firstBranch->validate());

  struct Format {
    Lane lane;
    bool dwg;
    DRW::Version version;
    const char *suffix;
  };
  const std::array<Format, 5> formats{
      {{Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
       {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
       {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
       {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
       {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};
  for (const auto &format : formats) {
#ifndef DWGSUPPORT
        if (format.dwg) continue;
#endif
        xd::Definition metadata;
        metadata.source = definition;
        xd::Options encoding;
        if (format.dwg) {
            encoding.eed = true;
            encoding.textEncoding = xd::TextEncoding::Utf16;
        } else {
            encoding.asciiDxf = false;
        }
        const auto tags = xd::forExport(
            {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
            metadata, encoding);
        REQUIRE(tags);
        DRW_Spline cache = f::raw(1, *tags);
        cache.controllist.clear();
        for (const auto& point : firstBranchCache.controlPoints)
            cache.controllist.push_back(
                std::make_shared<DRW_Coord>(point.x, point.y, 0));
        cache.knotslist = firstBranchCache.knotslist;
        cache.weightlist = firstBranchCache.weights;
        cache.degree = firstBranchCache.degree;
        cache.flags = 8;
        cache.m_scenario = 1;
        cache.ncontrol = cache.controllist.size();
        cache.nknots = cache.knotslist.size();
        cache.nfit = 0;

        const auto input = directory.file(std::string(side == LC_CurveOffsetSide::Left
            ? "left-" : "right-") + format.suffix);
#ifdef DWGSUPPORT
        if (format.dwg) {
            f::dwgFile(input, cache, format.version);
            preserveTypedSourceDwg(input, std::string("hyperbola-") +
                (side == LC_CurveOffsetSide::Left ? "left-" : "right-") + format.suffix);
        }
        else f::binaryDxf(input, cache, format.version);
#else
        f::binaryDxf(input, cache, format.version);
#endif
        const auto outcome = stagedImport(input, format.lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        const auto expected = values::encodeSourceDefinition(definition);
        REQUIRE(expected);
        CHECK(outcome.admitted == (singleCache ? 1 : 0));
        CHECK(outcome.downgraded == (singleCache ? 0 : 1));
        const auto* importedEntity = f::sole(*outcome.graphic);
        REQUIRE(importedEntity);
        if (singleCache) {
            const auto* imported = dynamic_cast<const LC_OffsetCurve*>(importedEntity);
            REQUIRE(imported);
            REQUIRE(imported->sourceDefinition());
            const auto received = values::encodeSourceDefinition(*imported->sourceDefinition());
            REQUIRE(received);
            CHECK(QCborValue(*expected).toCbor() == QCborValue(*received).toCbor());
            CHECK(RS_Spline::geometryFingerprint(imported->getData()) ==
                  RS_Spline::geometryFingerprint(firstBranchCache));

            const auto nativeOutput = directory.file(std::string("native-left-") +
                                                       format.suffix);
            REQUIRE(lc::test::exportDrawing(*outcome.graphic, nativeOutput,
                                             ::format(format.lane)));
            const auto nativeRoundTrip = stagedImport(nativeOutput, format.lane);
            INFO("native hyperbola writer round-trip " << format.suffix);
            REQUIRE(nativeRoundTrip.status == LC_OffsetImportSession::Status::Complete);
            REQUIRE(nativeRoundTrip.graphic);
            CHECK(nativeRoundTrip.admitted == 1);
            CHECK(nativeRoundTrip.downgraded == 0);
            const auto* roundTripped = dynamic_cast<const LC_OffsetCurve*>(
                f::sole(*nativeRoundTrip.graphic));
            REQUIRE(roundTripped);
            REQUIRE(roundTripped->sourceDefinition());
            const auto roundTripSource = values::encodeSourceDefinition(
                *roundTripped->sourceDefinition());
            REQUIRE(roundTripSource);
            CHECK(QCborValue(*expected).toCbor() ==
                  QCborValue(*roundTripSource).toCbor());
            CHECK(RS_Spline::geometryFingerprint(roundTripped->getData()) ==
                  RS_Spline::geometryFingerprint(firstBranchCache));
        } else {
            const auto* fallback = dynamic_cast<const RS_Spline*>(importedEntity);
            REQUIRE(fallback);
            CHECK(dynamic_cast<const LC_OffsetCurve*>(importedEntity) == nullptr);
            CHECK(RS_Spline::geometryFingerprint(fallback->getData()) ==
                  RS_Spline::geometryFingerprint(firstBranchCache));
            const auto parsed = xd::parseAndStrip(fallback->getDrwExtData(), encoding);
            REQUIRE(parsed.status == xd::Status::Schema2);
            REQUIRE(parsed.definition);
            REQUIRE(parsed.definition->source);
            const auto received = values::encodeSourceDefinition(*parsed.definition->source);
            REQUIRE(received);
            CHECK(QCborValue(*expected).toCbor() == QCborValue(*received).toCbor());
        }
    }
}

TEST_CASE("hyperbola curvature-neighborhood imports follow topology across scales",
          "[lc-offsetcurve][import-session][typed-source][hyperbola][curvature]["
          "scale]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-hyperbola-curvature-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;
    const std::array<LC_HyperbolaData, 8> variants{
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, 1.5, -1.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -.4, .4, true},    LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, true},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, .2, 1.2, false},   LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, -.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, true}};
    struct Format {
        Lane lane;
        bool dwg;
        DRW::Version version;
        const char* suffix;
    };
    const std::array<Format, 5> formats{{{Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
                                         {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
                                         {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
                                         {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
                                         {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};
    constexpr std::array<double, 4> scales{1e-3, 1.0, 1e3, 1e12};
    std::size_t promotedSettings = 0;
    std::size_t downgradedSettings = 0;
    std::array<std::size_t, scales.size()> promotedByScale{};
    std::array<std::size_t, scales.size()> downgradedByScale{};

    for (std::size_t scaleIndex = 0; scaleIndex < scales.size(); ++scaleIndex) {
        const double scale = scales[scaleIndex];
        CAPTURE(scale);
        for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
            const auto& base = variants[variantIndex];
            const LC_HyperbolaData data{{base.center.x * scale, base.center.y * scale, base.center.z},
                                        {base.majorP.x * scale, base.majorP.y * scale, base.majorP.z},
                                        base.ratio,
                                        base.angle1,
                                        base.angle2,
                                        base.reversed};
            for (const double normalizedDistance : {-1.05, 1.05, -1.08, 1.08, -1.09, 1.09}) {
                const double signedDistance = normalizedDistance * scale;
                CAPTURE(variantIndex, normalizedDistance, signedDistance);
                const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
                auto source = definition.source.makeEntity();
                REQUIRE(source);
                const double magnitude = std::abs(signedDistance);
                const auto options = LC_OffsetWorkerTolerance::directOptions(*source, magnitude, definition.relativeTolerance).value();
                const auto generated = LC_CurveOffset::buildDirectBranches(
                    *source,
                    LC_CurveOffset::makeSideRequest(signedDistance < 0 ? LC_CurveOffsetSide::Right : LC_CurveOffsetSide::Left, magnitude),
                    options, LC_CurveOffset::makeDirectSourceBudget());
                REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
                const auto materialized =
                    LC_CurveOffset::materializeBranches(*source, generated, options, LC_CurveOffset::makeDirectSourceBudget());
                REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
                REQUIRE_FALSE(materialized.entities.empty());
                const auto* firstBranch = dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
                REQUIRE(firstBranch);
                REQUIRE(firstBranch->validate());
                const RS_SplineData firstBranchCache = firstBranch->getData();
                const bool shouldPromote = materialized.entities.size() == 1;
                if (shouldPromote) {
                    ++promotedSettings;
                    ++promotedByScale[scaleIndex];
                }
                else {
                    ++downgradedSettings;
                    ++downgradedByScale[scaleIndex];
                }

                for (const auto& format : formats) {
#ifndef DWGSUPPORT
                    if (format.dwg)
                        continue;
#endif
                    xd::Definition metadata;
                    metadata.source = definition;
                    xd::Options encoding;
                    if (format.dwg) {
                        encoding.eed = true;
                        encoding.textEncoding = xd::TextEncoding::Utf16;
                    }
                    else {
                        encoding.asciiDxf = false;
                    }
                    const auto tags = xd::forExport({std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))}, metadata, encoding);
                    REQUIRE(tags);
                    DRW_Spline cache = f::raw(1, *tags);
                    cache.controllist.clear();
                    for (const auto& point : firstBranchCache.controlPoints)
                        cache.controllist.push_back(std::make_shared<DRW_Coord>(point.x, point.y, 0));
                    cache.knotslist = firstBranchCache.knotslist;
                    cache.weightlist = firstBranchCache.weights;
                    cache.degree = firstBranchCache.degree;
                    cache.flags = 8;
                    cache.m_scenario = 1;
                    cache.ncontrol = cache.controllist.size();
                    cache.nknots = cache.knotslist.size();
                    cache.nfit = 0;

                    const auto input = directory.file(std::to_string(scaleIndex) + "-" + std::to_string(variantIndex) + "-" +
                                                      std::to_string(normalizedDistance) + "-" + format.suffix);
#ifdef DWGSUPPORT
                    if (format.dwg)
                        f::dwgFile(input, cache, format.version);
                    else
                        f::binaryDxf(input, cache, format.version);
#else
                    f::binaryDxf(input, cache, format.version);
#endif
                    const auto outcome = stagedImport(input, format.lane);
                    REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
                    REQUIRE(outcome.graphic);
                    CHECK(outcome.admitted == (shouldPromote ? 1u : 0u));
                    CHECK(outcome.downgraded == (shouldPromote ? 0u : 1u));
                    const auto* imported = f::sole(*outcome.graphic);
                    REQUIRE(imported);
                    const auto* nativeOffset = dynamic_cast<const LC_OffsetCurve*>(imported);
                    CHECK((nativeOffset != nullptr) == shouldPromote);
                    const auto* importedSpline = dynamic_cast<const RS_Spline*>(imported);
                    REQUIRE(importedSpline);
                    CHECK(RS_Spline::geometryFingerprint(importedSpline->getData()) == RS_Spline::geometryFingerprint(firstBranchCache));
                    const auto expectedSource = values::encodeSourceDefinition(definition);
                    REQUIRE(expectedSource);
                    if (shouldPromote) {
                        REQUIRE(nativeOffset);
                        REQUIRE(nativeOffset->sourceDefinition());
                        const auto receivedSource = values::encodeSourceDefinition(*nativeOffset->sourceDefinition());
                        REQUIRE(receivedSource);
                        CHECK(QCborValue(*expectedSource).toCbor() == QCborValue(*receivedSource).toCbor());

                        if (scaleIndex == 1 && std::abs(normalizedDistance) == 1.05) {
                            const auto nativeOutput = directory.file(
                                "native-" + std::to_string(scaleIndex) + "-" +
                                std::to_string(variantIndex) + "-" +
                                std::to_string(normalizedDistance) + "-" +
                                format.suffix);
                            REQUIRE(lc::test::exportDrawing(
                                *outcome.graphic, nativeOutput,
                                ::format(format.lane)));
                            const auto nativeRoundTrip = stagedImport(
                                nativeOutput, format.lane);
                            INFO("native hyperbola matrix writer round-trip "
                                 << variantIndex << " " << normalizedDistance
                                 << " " << format.suffix);
                            REQUIRE(nativeRoundTrip.status ==
                                    LC_OffsetImportSession::Status::Complete);
                            REQUIRE(nativeRoundTrip.graphic);
                            CHECK(nativeRoundTrip.admitted == 1);
                            CHECK(nativeRoundTrip.downgraded == 0);
                            const auto* roundTripped =
                                dynamic_cast<const LC_OffsetCurve*>(
                                    f::sole(*nativeRoundTrip.graphic));
                            REQUIRE(roundTripped);
                            REQUIRE(roundTripped->sourceDefinition());
                            const auto roundTripSource =
                                values::encodeSourceDefinition(
                                    *roundTripped->sourceDefinition());
                            REQUIRE(roundTripSource);
                            CHECK(QCborValue(*expectedSource).toCbor() ==
                                  QCborValue(*roundTripSource).toCbor());
                            CHECK(RS_Spline::geometryFingerprint(
                                      roundTripped->getData()) ==
                                  RS_Spline::geometryFingerprint(
                                      firstBranchCache));
                        }
                    }
                    else {
                        const auto parsed = xd::parseAndStrip(imported->getDrwExtData(), encoding);
                        REQUIRE(parsed.status == xd::Status::Schema2);
                        REQUIRE(parsed.definition);
                        REQUIRE(parsed.definition->source);
                        const auto receivedSource = values::encodeSourceDefinition(*parsed.definition->source);
                        REQUIRE(receivedSource);
                        CHECK(QCborValue(*expectedSource).toCbor() == QCborValue(*receivedSource).toCbor());
                    }
                }
            }
        }
    }
    CHECK(promotedSettings > 0);
    CHECK(downgradedSettings > 0);
    for (std::size_t scaleIndex = 0; scaleIndex < scales.size(); ++scaleIndex) {
        CAPTURE(scales[scaleIndex]);
        CHECK(promotedByScale[scaleIndex] > 0);
        CHECK(downgradedByScale[scaleIndex] > 0);
    }
}

TEST_CASE("translated ellipse scale matrix preserves typed source and cache through file import",
          "[lc-offsetcurve][import-session][typed-source][ellipse][precision][combined]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-ellipse-translated-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;
    constexpr double tau = 2.0 * M_PI;
    const std::array<RS_EllipseData, 25> variants{
        RS_EllipseData{{2, -3}, {4, 0}, .6, .2, 2.1, false},
        RS_EllipseData{{-1, 2}, {3, 1}, .8, 1.8, .1, true},
        RS_EllipseData{{3, 1}, {2, -1}, 1.4, .3, 2.3, false},
        RS_EllipseData{{0, 0}, {2, 0}, .2, 0, tau, false},
        RS_EllipseData{{0, 0}, {2, 0}, .2, 0, tau, true},
        RS_EllipseData{{0, 0}, {std::sqrt(3.0), 1}, .2, 0, tau, false},
        RS_EllipseData{{0, 0}, {std::sqrt(3.0), 1}, .2, 0, tau, true},
        RS_EllipseData{{0, 0}, {1, std::sqrt(3.0)}, .2, 0, tau, false},
        RS_EllipseData{{0, 0}, {1, std::sqrt(3.0)}, .2, 0, tau, true},
        RS_EllipseData{{0, 0}, {2, 0}, .2, .2, M_PI - .2, false},
        RS_EllipseData{{0, 0}, {2, 0}, .2, .2, M_PI - .2, true},
        RS_EllipseData{{0, 0}, {2, 0}, .2, .2, M_PI + .2, false},
        RS_EllipseData{{0, 0}, {2, 0}, .2, .2, M_PI + .2, true},
        RS_EllipseData{{0, 0}, {2, 0}, .2, tau - .2, .4, false},
        RS_EllipseData{{0, 0}, {2, 0}, .2, tau - .2, .4, true},
        RS_EllipseData{{0, 0}, {2, 0}, .2, tau - .2, M_PI + .2, false},
        RS_EllipseData{{0, 0}, {2, 0}, .2, tau - .2, M_PI + .2, true},
        RS_EllipseData{{0, 0}, {std::sqrt(3.0), 1}, .2, .2, M_PI - .2, false},
        RS_EllipseData{{0, 0}, {std::sqrt(3.0), 1}, .2, .2, M_PI - .2, true},
        RS_EllipseData{{0, 0}, {std::sqrt(3.0), 1}, .2, .2, M_PI + .2, false},
        RS_EllipseData{{0, 0}, {std::sqrt(3.0), 1}, .2, .2, M_PI + .2, true},
        RS_EllipseData{{0, 0}, {1, std::sqrt(3.0)}, .2, tau - .2, .4, false},
        RS_EllipseData{{0, 0}, {1, std::sqrt(3.0)}, .2, tau - .2, .4, true},
        RS_EllipseData{{0, 0}, {1, std::sqrt(3.0)}, .2, tau - .2, M_PI + .2, false},
        RS_EllipseData{{0, 0}, {1, std::sqrt(3.0)}, .2, tau - .2, M_PI + .2, true}};
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
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};
    constexpr std::array<double, 3> scales{1e-3, 1.0, 1e12};
    constexpr std::array<double, 3> translationRatios{1e6, 1e9, 1e12};
    constexpr std::array<double, 4> ordinaryDistances{-.4, -.1, .1, .4};
    constexpr std::array<double, 8> thinDistances{-.09, -.08, -.07, -.05, .05, .07, .08, .09};
    constexpr std::array<double, 6> partialThinDistances{-.09, -.08, -.07, .07, .08, .09};

    const auto buildSingleCache = [](const LC_OffsetSourceDefinition& definition,
                                     double& requestedTolerance) -> std::optional<RS_SplineData> {
        LC_OffsetSourceDefinition workingDefinition = definition;
        RS_Vector outputTranslation{0, 0};
        if (auto* ellipse = std::get_if<RS_EllipseData>(&workingDefinition.source.value)) {
            outputTranslation = {ellipse->center.x, ellipse->center.y};
            ellipse->center.x = 0.0;
            ellipse->center.y = 0.0;
        }
        auto source = workingDefinition.source.makeEntity();
        if (!source) return std::nullopt;
        const double magnitude = std::abs(workingDefinition.signedDistance);
        const auto options = LC_OffsetWorkerTolerance::directOptions(
            *source, magnitude, workingDefinition.relativeTolerance);
        if (!options) return std::nullopt;
        requestedTolerance = options->tolerance.requestedGeometry;
        const auto generated = LC_CurveOffset::buildDirectBranches(
            *source,
            LC_CurveOffset::makeSideRequest(workingDefinition.signedDistance < 0
                ? LC_CurveOffsetSide::Right : LC_CurveOffsetSide::Left, magnitude),
            *options, LC_CurveOffset::makeDirectSourceBudget());
        if (generated.status != LC_CurveOffsetStatus::Ok) return std::nullopt;
        auto materialized = LC_CurveOffset::materializeBranches(
            *source, generated, *options, LC_CurveOffset::makeDirectSourceBudget());
        if (materialized.status != LC_CurveOffsetStatus::Ok || materialized.entities.size() != 1)
            return std::nullopt;
        if (const auto* spline = dynamic_cast<const RS_Spline*>(materialized.entities.front().get())) {
            if (!spline->validate()) return std::nullopt;
            RS_SplineData cache = spline->getData();
            for (RS_Vector& point : cache.controlPoints) {
                point.x += outputTranslation.x;
                point.y += outputTranslation.y;
            }
            for (RS_Vector& point : cache.fitPoints) {
                point.x += outputTranslation.x;
                point.y += outputTranslation.y;
            }
            return cache;
        }
        if (const auto* line = dynamic_cast<const RS_Line*>(materialized.entities.front().get())) {
            RS_SplineData cache{1, false};
            RS_Vector start = line->getStartpoint();
            RS_Vector end = line->getEndpoint();
            start.x += outputTranslation.x;
            start.y += outputTranslation.y;
            end.x += outputTranslation.x;
            end.y += outputTranslation.y;
            cache.controlPoints = {start, end};
            cache.knotslist = {0, 0, 1, 1};
            cache.weights = {1, 1};
            return cache;
        }
        return std::nullopt;
    };

    std::size_t importedCases = 0;
    std::size_t refusedCases = 0;
    std::size_t ellipseNativeWriterCases = 0;
    std::size_t libreDwgCases = 0;
    QString libreDwgReader = qEnvironmentVariable("DWGREAD");
    if (libreDwgReader.isEmpty())
        libreDwgReader = QStandardPaths::findExecutable("dwgread");
    std::array<std::size_t, scales.size()> importsByScale{};
    std::array<std::size_t, scales.size()> refusalsByScale{};
    for (std::size_t scaleIndex = 0; scaleIndex < scales.size(); ++scaleIndex) {
        const double scale = scales[scaleIndex];
        CAPTURE(scale);
        for (std::size_t ratioIndex = 0; ratioIndex < translationRatios.size(); ++ratioIndex) {
          const double translationRatio = translationRatios[ratioIndex];
          CAPTURE(translationRatio);
          const RS_Vector translation{translationRatio * scale, -translationRatio * scale};
          for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
            const auto& base = variants[variantIndex];
            const RS_EllipseData centeredData{{base.center.x * scale, base.center.y * scale, base.center.z},
                                              {base.majorP.x * scale, base.majorP.y * scale, base.majorP.z},
                                              base.ratio, base.angle1, base.angle2, base.reversed};
            const RS_EllipseData translatedData{{centeredData.center.x + translation.x,
                                                 centeredData.center.y + translation.y, centeredData.center.z},
                                                centeredData.majorP, centeredData.ratio,
                                                centeredData.angle1, centeredData.angle2, centeredData.reversed};
            const LC_OffsetSourceGeometry centered{centeredData};
            const LC_OffsetSourceGeometry translated{translatedData};
            REQUIRE(centered.isWellFormed());
            REQUIRE(translated.isWellFormed());
            const bool thin = base.ratio == .2;
            const bool thinFull = thin && base.angle1 == 0.0 &&
                (base.angle2 == tau || base.angle2 == 0.0);
            std::vector<double> distances;
            if (thinFull) {
                distances.assign(thinDistances.begin(), thinDistances.end());
            } else if (thin) {
                distances.assign(partialThinDistances.begin(), partialThinDistances.end());
            } else {
                distances.assign(ordinaryDistances.begin(), ordinaryDistances.end());
            }
            for (std::size_t distanceIndex = 0; distanceIndex < distances.size(); ++distanceIndex) {
                const double normalizedDistance = distances[distanceIndex];
                const double signedDistance = normalizedDistance * scale;
                CAPTURE(variantIndex, normalizedDistance);
                const LC_OffsetSourceDefinition centeredDefinition{centered, signedDistance, 1e-4, false};
                const LC_OffsetSourceDefinition translatedDefinition{translated, signedDistance, 1e-4, false};
                double centeredTolerance = 0;
                double translatedTolerance = 0;
                const auto centeredCache = buildSingleCache(centeredDefinition, centeredTolerance);
                const auto translatedCache = buildSingleCache(translatedDefinition, translatedTolerance);
                if (!centeredCache || !translatedCache) {
                    ++refusedCases;
                    ++refusalsByScale[scaleIndex];
                    continue;
                }

                RS_Spline expectedSpline{nullptr, *translatedCache};
                bool allLanesAdmitted = true;
                for (const auto& fileFormat : formats) {
#ifndef DWGSUPPORT
                    if (fileFormat.dwg) continue;
#endif
                    xd::Definition metadata;
                    metadata.source = translatedDefinition;
                    xd::Options encoding;
                    if (fileFormat.dwg) {
                        encoding.eed = true;
                        encoding.textEncoding = xd::TextEncoding::Utf16;
                    }
                    else {
                        encoding.asciiDxf = false;
                    }
                    const auto tags = xd::forExport(
                        {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
                        metadata, encoding);
                    REQUIRE(tags);
                    DRW_Spline carrier = f::raw(1, *tags);
                    carrier.controllist.clear();
                    for (const auto& point : translatedCache->controlPoints)
                        carrier.controllist.push_back(std::make_shared<DRW_Coord>(point.x, point.y, 0));
                    carrier.knotslist = translatedCache->knotslist;
                    carrier.weightlist = translatedCache->weights;
                    carrier.degree = translatedCache->degree;
                    carrier.flags = 8;
                    carrier.m_scenario = 1;
                    carrier.ncontrol = carrier.controllist.size();
                    carrier.nknots = carrier.knotslist.size();
                    carrier.nfit = 0;

                    const auto input = directory.file(std::to_string(scaleIndex) + "-" +
                        std::to_string(ratioIndex) + "-" + std::to_string(variantIndex) + "-" +
                        std::to_string(distanceIndex) + "-" + fileFormat.suffix);
#ifdef DWGSUPPORT
                    if (fileFormat.dwg) f::dwgFile(input, carrier, fileFormat.version);
                    else f::binaryDxf(input, carrier, fileFormat.version);
#else
                    f::binaryDxf(input, carrier, fileFormat.version);
#endif
                    const auto outcome = stagedImport(input, fileFormat.lane);
                    REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
                    REQUIRE(outcome.graphic);
                    const auto* imported = f::sole(*outcome.graphic);
                    REQUIRE(imported);
                    const auto* nativeOffset = dynamic_cast<const LC_OffsetCurve*>(imported);
                    CHECK(outcome.admitted == (nativeOffset ? 1u : 0u));
                    CHECK(outcome.downgraded == (nativeOffset ? 0u : 1u));
                    allLanesAdmitted &= nativeOffset != nullptr;
                    const auto expectedSource = values::encodeSourceDefinition(translatedDefinition);
                    REQUIRE(expectedSource);
                    if (nativeOffset) {
                        REQUIRE(nativeOffset->sourceDefinition());
                        const auto receivedSource = values::encodeSourceDefinition(*nativeOffset->sourceDefinition());
                        REQUIRE(receivedSource);
                        CHECK(QCborValue(*expectedSource).toCbor() == QCborValue(*receivedSource).toCbor());

                        const bool roundTripNativeWriter = distanceIndex == 0 ||
                            (scaleIndex == 1 && ratioIndex == 0);
                        if (roundTripNativeWriter) {
                            const auto nativeOutput = directory.file(
                                "native-" + std::to_string(scaleIndex) + "-" +
                                std::to_string(ratioIndex) + "-" +
                                std::to_string(variantIndex) + "-" +
                                std::to_string(distanceIndex) + "-" +
                                fileFormat.suffix);
                            REQUIRE(lc::test::exportDrawing(
                                *outcome.graphic, nativeOutput,
                                ::format(fileFormat.lane)));
#ifdef DWGSUPPORT
                            if (!libreDwgReader.isEmpty() && fileFormat.dwg &&
                                scaleIndex == 1 && ratioIndex == 0 && variantIndex == 0 &&
                                distanceIndex == 0) {
                                QProcess reader;
                                reader.start(libreDwgReader,
                                    {"-O", "minJSON", lc::test::toQString(nativeOutput)});
                                REQUIRE(reader.waitForStarted());
                                REQUIRE(reader.waitForFinished(30000));
                                INFO(reader.readAllStandardError().toStdString());
                                CHECK(reader.exitStatus() == QProcess::NormalExit);
                                CHECK(reader.exitCode() == 0);
                                const QByteArray output = reader.readAllStandardOutput();
                                CHECK_FALSE(output.isEmpty());
                                QJsonParseError parseError;
                                const QJsonDocument document = QJsonDocument::fromJson(
                                    output, &parseError);
                                REQUIRE(parseError.error == QJsonParseError::NoError);
                                REQUIRE(document.isObject());
                                const QJsonArray objects = document.object()
                                    .value("OBJECTS").toArray();
                                QJsonObject externalSpline;
                                for (const QJsonValue& object : objects) {
                                    const QJsonObject candidate = object.toObject();
                                    if (candidate.value("entity").toString() == "SPLINE") {
                                        externalSpline = candidate;
                                        break;
                                    }
                                }
                                REQUIRE(!externalSpline.isEmpty());
                                CHECK(externalSpline.value("degree").toInt() ==
                                      translatedCache->degree);
                                const QJsonArray knots =
                                    externalSpline.value("knots").toArray();
                                REQUIRE(knots.size() == static_cast<qsizetype>(
                                    translatedCache->knotslist.size()));
                                for (qsizetype i = 0; i < knots.size(); ++i)
                                    CHECK(knots[i].toDouble() == Catch::Approx(
                                        translatedCache->knotslist[static_cast<std::size_t>(i)])
                                        .margin(1e-12));
                                const QJsonArray controls =
                                    externalSpline.value("ctrl_pts").toArray();
                                REQUIRE(controls.size() == static_cast<qsizetype>(
                                    translatedCache->controlPoints.size()));
                                for (qsizetype i = 0; i < controls.size(); ++i) {
                                    const auto& expected = translatedCache->controlPoints[
                                        static_cast<std::size_t>(i)];
                                    const QJsonObject actual = controls[i].toObject();
                                    CHECK(actual.value("x").toDouble() ==
                                          Catch::Approx(expected.x).margin(1e-11));
                                    CHECK(actual.value("y").toDouble() ==
                                          Catch::Approx(expected.y).margin(1e-11));
                                    CHECK(actual.value("z").toDouble() ==
                                          Catch::Approx(expected.z).margin(1e-11));
                                }
                                ++libreDwgCases;
                            }
#endif
                            const auto nativeRoundTrip = stagedImport(
                                nativeOutput, fileFormat.lane);
                            INFO("native translated-ellipse writer round-trip "
                                 << variantIndex << " " << fileFormat.suffix);
                            REQUIRE(nativeRoundTrip.status ==
                                    LC_OffsetImportSession::Status::Complete);
                            REQUIRE(nativeRoundTrip.graphic);
                            CHECK(nativeRoundTrip.admitted == 1);
                            CHECK(nativeRoundTrip.downgraded == 0);
                            const auto* roundTripped =
                                dynamic_cast<const LC_OffsetCurve*>(
                                    f::sole(*nativeRoundTrip.graphic));
                            REQUIRE(roundTripped);
                            REQUIRE(roundTripped->sourceDefinition());
                            const auto roundTripSource =
                                values::encodeSourceDefinition(
                                    *roundTripped->sourceDefinition());
                            REQUIRE(roundTripSource);
                            CHECK(QCborValue(*expectedSource).toCbor() ==
                                  QCborValue(*roundTripSource).toCbor());
                            CHECK(RS_Spline::geometryFingerprint(
                                      roundTripped->getData()) ==
                                  RS_Spline::geometryFingerprint(
                                      *translatedCache));
                            ++ellipseNativeWriterCases;
                        }
                    } else {
                        CHECK_FALSE(outcome.diagnostics.empty());
                        const auto parsed = xd::parseAndStrip(imported->getDrwExtData(), encoding);
                        REQUIRE(parsed.status == xd::Status::Schema2);
                        REQUIRE(parsed.definition);
                        REQUIRE(parsed.definition->source);
                        const auto receivedSource = values::encodeSourceDefinition(*parsed.definition->source);
                        REQUIRE(receivedSource);
                        CHECK(QCborValue(*expectedSource).toCbor() == QCborValue(*receivedSource).toCbor());
                    }
                    const auto* importedSpline = dynamic_cast<const RS_Spline*>(imported);
                    REQUIRE(importedSpline);
                    CHECK(RS_Spline::geometryFingerprint(importedSpline->getData()) ==
                          RS_Spline::geometryFingerprint(*translatedCache));
                    double maximumDeviation = 0;
                    for (int sample = 0; sample <= 128; ++sample) {
                        const double parameter = static_cast<double>(sample) / 128.0;
                        const RS_Vector expected = expectedSpline.getPointAt(parameter);
                        const RS_Vector actual = importedSpline->getPointAt(parameter);
                        REQUIRE(expected.valid);
                        REQUIRE(actual.valid);
                        maximumDeviation = std::max(maximumDeviation,
                            std::hypot(actual.x - expected.x, actual.y - expected.y));
                    }
                    const double coordinateUlpAllowance = 64.0 * std::numeric_limits<double>::epsilon() *
                        std::max(std::abs(translation.x), std::abs(translation.y));
                    CHECK(maximumDeviation <= 4.0 * translatedTolerance + coordinateUlpAllowance);
                }
                if (allLanesAdmitted) {
                    ++importedCases;
                    ++importsByScale[scaleIndex];
                } else {
                    ++refusedCases;
                    ++refusalsByScale[scaleIndex];
                }
            }
          }
        }
    }
    CHECK(importedCases > 0);
    CHECK(refusedCases > 0);
    CHECK(ellipseNativeWriterCases > 0);
#ifdef DWGSUPPORT
    if (!libreDwgReader.isEmpty()) CHECK(libreDwgCases == 2);
#endif
    const std::size_t cellsPerRatio = 3 * ordinaryDistances.size() + 6 * thinDistances.size() +
        16 * partialThinDistances.size();
    const std::size_t expectedCells = scales.size() * translationRatios.size() * cellsPerRatio;
    CHECK(importedCases + refusedCases == expectedCells);
    for (std::size_t i = 0; i < scales.size(); ++i) {
        CAPTURE(scales[i]);
        CHECK(importsByScale[i] + refusalsByScale[i] == translationRatios.size() * cellsPerRatio);
    }
}

TEST_CASE("translated hyperbola imports preserve local cache and typed source",
          "[lc-offsetcurve][import-session][typed-source][hyperbola][precision]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-hyperbola-translated-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;
    const RS_Vector translation{1e9, -1e9};
    const std::array<LC_HyperbolaData, 8> variants{
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, 1.5, -1.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -.4, .4, true},    LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, true},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, .2, 1.2, false},   LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, -.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, true}};
    struct Format {
        Lane lane;
        bool dwg;
        DRW::Version version;
        const char* suffix;
    };
    const std::array<Format, 5> formats{{{Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
                                         {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
                                         {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
                                         {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
                                         {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};
    std::size_t promotedCases = 0;
    std::size_t fallbackCases = 0;

    for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
        const auto& base = variants[variantIndex];
        const LC_HyperbolaData translatedData{{base.center.x + translation.x, base.center.y + translation.y, base.center.z},
                                              base.majorP,
                                              base.ratio,
                                              base.angle1,
                                              base.angle2,
                                              base.reversed};
        for (const double signedDistance : {-1.05, 1.05, -1.08, 1.08, -1.09, 1.09}) {
            CAPTURE(variantIndex, signedDistance);
            const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{translatedData}, signedDistance, 1e-4, false};
            auto source = definition.source.makeEntity();
            REQUIRE(source);
            const double magnitude = std::abs(signedDistance);
            const auto options = LC_OffsetWorkerTolerance::directOptions(*source, magnitude, definition.relativeTolerance).value();
            const auto generated = LC_CurveOffset::buildDirectBranches(
                *source,
                LC_CurveOffset::makeSideRequest(signedDistance < 0 ? LC_CurveOffsetSide::Right : LC_CurveOffsetSide::Left, magnitude),
                options, LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
            const auto materialized =
                LC_CurveOffset::materializeBranches(*source, generated, options, LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
            REQUIRE_FALSE(materialized.entities.empty());
            const auto* firstBranch = dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
            REQUIRE(firstBranch);
            REQUIRE(firstBranch->validate());
            const RS_SplineData expectedCache = firstBranch->getData();
            const RS_Spline expectedSpline{nullptr, expectedCache};
            const bool shouldPromote = materialized.entities.size() == 1;
            if (shouldPromote)
                ++promotedCases;
            else
                ++fallbackCases;

            for (const auto& format : formats) {
#ifndef DWGSUPPORT
                if (format.dwg)
                    continue;
#endif
                xd::Definition metadata;
                metadata.source = definition;
                xd::Options encoding;
                if (format.dwg) {
                    encoding.eed = true;
                    encoding.textEncoding = xd::TextEncoding::Utf16;
                }
                else {
                    encoding.asciiDxf = false;
                }
                const auto tags = xd::forExport({std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))}, metadata, encoding);
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

                const auto input =
                    directory.file(std::to_string(variantIndex) + "-" + std::to_string(signedDistance) + "-" + format.suffix);
#ifdef DWGSUPPORT
                if (format.dwg)
                    f::dwgFile(input, cache, format.version);
                else
                    f::binaryDxf(input, cache, format.version);
#else
                f::binaryDxf(input, cache, format.version);
#endif
                const auto outcome = stagedImport(input, format.lane);
                REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
                REQUIRE(outcome.graphic);
                CHECK(outcome.admitted == (shouldPromote ? 1u : 0u));
                CHECK(outcome.downgraded == (shouldPromote ? 0u : 1u));
                auto* imported = f::sole(*outcome.graphic);
                REQUIRE(imported);
                const auto* nativeOffset = dynamic_cast<const LC_OffsetCurve*>(imported);
                CHECK((nativeOffset != nullptr) == shouldPromote);
                const auto* importedSpline = dynamic_cast<const RS_Spline*>(imported);
                REQUIRE(importedSpline);
                const RS_Spline actualSpline{nullptr, importedSpline->getData()};
                double maximumDeviation = 0.0;
                for (int sample = 0; sample <= 128; ++sample) {
                    const double parameter = static_cast<double>(sample) / 128.0;
                    const RS_Vector expected = expectedSpline.getPointAt(parameter);
                    const RS_Vector actual = actualSpline.getPointAt(parameter);
                    REQUIRE(expected.valid);
                    REQUIRE(actual.valid);
                    maximumDeviation = std::max(maximumDeviation, std::hypot(expected.x - actual.x, expected.y - actual.y));
                }
                const double roundingAllowance =
                    64.0 * std::numeric_limits<double>::epsilon() * std::max(std::abs(translation.x), std::abs(translation.y));
                CHECK(maximumDeviation <= 4.0 * options.tolerance.requestedGeometry + roundingAllowance);

                const auto expectedSource = values::encodeSourceDefinition(definition);
                REQUIRE(expectedSource);
                std::optional<LC_OffsetSourceDefinition> receivedDefinition;
                if (shouldPromote) {
                    REQUIRE(nativeOffset);
                    REQUIRE(nativeOffset->sourceDefinition());
                    receivedDefinition = *nativeOffset->sourceDefinition();
                }
                else {
                    const auto parsed = xd::parseAndStrip(imported->getDrwExtData(), encoding);
                    REQUIRE(parsed.status == xd::Status::Schema2);
                    REQUIRE(parsed.definition);
                    REQUIRE(parsed.definition->source);
                    receivedDefinition = *parsed.definition->source;
                }
                const auto receivedSource = values::encodeSourceDefinition(*receivedDefinition);
                REQUIRE(receivedSource);
                CHECK(QCborValue(*expectedSource).toCbor() == QCborValue(*receivedSource).toCbor());
            }
        }
    }
    CHECK(promotedCases > 0);
    CHECK(fallbackCases > 0);
}

TEST_CASE("combined scale and translation hyperbola imports preserve typed cache behavior",
          "[lc-offsetcurve][import-session][typed-source][hyperbola][precision][combined]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-hyperbola-combined-scale-translation-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;
    const std::array<LC_HyperbolaData, 8> variants{
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, 1.5, -1.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -.4, .4, true},    LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, true},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, .2, 1.2, false},   LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, -.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, true}};
    struct Format {
        Lane lane;
        bool dwg;
        DRW::Version version;
        const char* suffix;
    };
    const std::array<Format, 5> formats{{{Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
                                         {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
                                         {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
                                         {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
                                         {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};
    constexpr std::array<double, 4> scales{1e-3, 1.0, 1e3, 1e12};
    constexpr std::array<double, 6> normalizedDistances{-1.05, 1.05, -1.08, 1.08, -1.09, 1.09};
    std::array<std::size_t, scales.size()> promotedByScale{};
    std::array<std::size_t, scales.size()> fallbackByScale{};

    for (std::size_t scaleIndex = 0; scaleIndex < scales.size(); ++scaleIndex) {
        const double scale = scales[scaleIndex];
        CAPTURE(scale);
        const RS_Vector translation{1e9 * scale, -1e9 * scale};
        for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
            const auto& base = variants[variantIndex];
            const LC_HyperbolaData data{{base.center.x * scale + translation.x, base.center.y * scale + translation.y, base.center.z},
                                        {base.majorP.x * scale, base.majorP.y * scale, base.majorP.z},
                                        base.ratio,
                                        base.angle1,
                                        base.angle2,
                                        base.reversed};
            for (const double normalizedDistance : normalizedDistances) {
                const double signedDistance = normalizedDistance * scale;
                CAPTURE(variantIndex, normalizedDistance, signedDistance);
                const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
                auto source = definition.source.makeEntity();
                REQUIRE(source);
                const double magnitude = std::abs(signedDistance);
                const auto options = LC_OffsetWorkerTolerance::directOptions(*source, magnitude, definition.relativeTolerance);
                REQUIRE(options);
                const auto generated = LC_CurveOffset::buildDirectBranches(
                    *source,
                    LC_CurveOffset::makeSideRequest(signedDistance < 0 ? LC_CurveOffsetSide::Right : LC_CurveOffsetSide::Left, magnitude),
                    *options, LC_CurveOffset::makeDirectSourceBudget());
                REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
                const auto materialized =
                    LC_CurveOffset::materializeBranches(*source, generated, *options, LC_CurveOffset::makeDirectSourceBudget());
                REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
                REQUIRE_FALSE(materialized.entities.empty());
                const auto* firstBranch = dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
                REQUIRE(firstBranch);
                REQUIRE(firstBranch->validate());
                const RS_SplineData expectedCache = firstBranch->getData();
                const RS_Spline expectedSpline{nullptr, expectedCache};
                const bool shouldPromote = materialized.entities.size() == 1;
                if (shouldPromote)
                    ++promotedByScale[scaleIndex];
                else
                    ++fallbackByScale[scaleIndex];

                for (const auto& format : formats) {
#ifndef DWGSUPPORT
                    if (format.dwg)
                        continue;
#endif
                    xd::Definition metadata;
                    metadata.source = definition;
                    xd::Options encoding;
                    if (format.dwg) {
                        encoding.eed = true;
                        encoding.textEncoding = xd::TextEncoding::Utf16;
                    }
                    else {
                        encoding.asciiDxf = false;
                    }
                    const auto tags = xd::forExport({std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))}, metadata, encoding);
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

                    const auto input = directory.file(std::to_string(scaleIndex) + "-" + std::to_string(variantIndex) + "-" +
                                                      std::to_string(normalizedDistance) + "-" + format.suffix);
#ifdef DWGSUPPORT
                    if (format.dwg)
                        f::dwgFile(input, cache, format.version);
                    else
                        f::binaryDxf(input, cache, format.version);
#else
                    f::binaryDxf(input, cache, format.version);
#endif
                    const auto outcome = stagedImport(input, format.lane);
                    REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
                    REQUIRE(outcome.graphic);
                    CHECK(outcome.admitted == (shouldPromote ? 1u : 0u));
                    CHECK(outcome.downgraded == (shouldPromote ? 0u : 1u));
                    const auto* imported = f::sole(*outcome.graphic);
                    REQUIRE(imported);
                    const auto* nativeOffset = dynamic_cast<const LC_OffsetCurve*>(imported);
                    CHECK((nativeOffset != nullptr) == shouldPromote);
                    const auto* importedSpline = dynamic_cast<const RS_Spline*>(imported);
                    REQUIRE(importedSpline);
                    const RS_Spline actualSpline{nullptr, importedSpline->getData()};
                    double maximumDeviation = 0.0;
                    for (int sample = 0; sample <= 128; ++sample) {
                        const double parameter = static_cast<double>(sample) / 128.0;
                        const RS_Vector expected = expectedSpline.getPointAt(parameter);
                        const RS_Vector actual = actualSpline.getPointAt(parameter);
                        REQUIRE(expected.valid);
                        REQUIRE(actual.valid);
                        maximumDeviation = std::max(maximumDeviation, std::hypot(expected.x - actual.x, expected.y - actual.y));
                    }
                    const double roundingAllowance =
                        64.0 * std::numeric_limits<double>::epsilon() * std::max(std::abs(translation.x), std::abs(translation.y));
                    CHECK(maximumDeviation <= 4.0 * options->tolerance.requestedGeometry + roundingAllowance);

                    const auto expectedSource = values::encodeSourceDefinition(definition);
                    REQUIRE(expectedSource);
                    std::optional<LC_OffsetSourceDefinition> receivedDefinition;
                    if (shouldPromote) {
                        REQUIRE(nativeOffset);
                        REQUIRE(nativeOffset->sourceDefinition());
                        receivedDefinition = *nativeOffset->sourceDefinition();
                    }
                    else {
                        const auto parsed = xd::parseAndStrip(imported->getDrwExtData(), encoding);
                        REQUIRE(parsed.status == xd::Status::Schema2);
                        REQUIRE(parsed.definition);
                        REQUIRE(parsed.definition->source);
                        receivedDefinition = *parsed.definition->source;
                    }
                    const auto receivedSource = values::encodeSourceDefinition(*receivedDefinition);
                    REQUIRE(receivedSource);
                    CHECK(QCborValue(*expectedSource).toCbor() == QCborValue(*receivedSource).toCbor());
                }
            }
        }
        CHECK(promotedByScale[scaleIndex] > 0);
        CHECK(fallbackByScale[scaleIndex] > 0);
    }
}

TEST_CASE("hyperbola relative-translation boundaries round-trip admissible caches",
          "[lc-offsetcurve][import-session][typed-source][hyperbola][precision][translation-boundary]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-hyperbola-relative-translation-boundary-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;
    const std::array<LC_HyperbolaData, 8> variants{
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, 1.5, -1.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -.4, .4, true},    LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, 1.5, true},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, .2, 1.2, false},   LC_HyperbolaData{{0, 0}, {3, 0}, .6, -1.2, -.2, false},
        LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, false}, LC_HyperbolaData{{0, 0}, {3, 0}, .6, -3.0, 3.0, true}};
    struct Format {
        Lane lane;
        bool dwg;
        DRW::Version version;
        const char* suffix;
    };
    const std::array<Format, 5> formats{{{Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
                                         {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
                                         {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
                                         {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
                                         {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};
    constexpr std::array<double, 3> scales{1e-3, 1.0, 1e12};
    constexpr std::array<double, 3> translationRatios{1e6, 1e9, 1e12};
    constexpr std::array<double, 6> normalizedDistances{-1.05, 1.05, -1.08, 1.08, -1.09, 1.09};
    std::array<std::array<std::size_t, translationRatios.size()>, scales.size()> importedByCell{};
    std::array<std::array<std::size_t, translationRatios.size()>, scales.size()> refusedByCell{};

    for (std::size_t scaleIndex = 0; scaleIndex < scales.size(); ++scaleIndex) {
        const double scale = scales[scaleIndex];
        CAPTURE(scale);
        for (std::size_t ratioIndex = 0; ratioIndex < translationRatios.size(); ++ratioIndex) {
            const double translationRatio = translationRatios[ratioIndex];
            const RS_Vector translation{translationRatio * scale, -translationRatio * scale};
            CAPTURE(translationRatio);
            for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
                const auto& base = variants[variantIndex];
                const LC_HyperbolaData data{{base.center.x * scale + translation.x, base.center.y * scale + translation.y, base.center.z},
                                            {base.majorP.x * scale, base.majorP.y * scale, base.majorP.z},
                                            base.ratio,
                                            base.angle1,
                                            base.angle2,
                                            base.reversed};
                for (const double normalizedDistance : normalizedDistances) {
                    const double signedDistance = normalizedDistance * scale;
                    CAPTURE(variantIndex, normalizedDistance);
                    const LC_OffsetSourceDefinition definition{LC_OffsetSourceGeometry{data}, signedDistance, 1e-4, false};
                    auto source = definition.source.makeEntity();
                    REQUIRE(source);
                    const double magnitude = std::abs(signedDistance);
                    const auto options = LC_OffsetWorkerTolerance::directOptions(*source, magnitude, definition.relativeTolerance);
                    if (!options) {
                        ++refusedByCell[scaleIndex][ratioIndex];
                        continue;
                    }
                    const auto generated = LC_CurveOffset::buildDirectBranches(
                        *source,
                        LC_CurveOffset::makeSideRequest(signedDistance < 0 ? LC_CurveOffsetSide::Right : LC_CurveOffsetSide::Left,
                                                        magnitude),
                        *options, LC_CurveOffset::makeDirectSourceBudget());
                    if (generated.status != LC_CurveOffsetStatus::Ok) {
                        CHECK(generated.status != LC_CurveOffsetStatus::Ok);
                        CHECK(generated.branches.empty());
                        ++refusedByCell[scaleIndex][ratioIndex];
                        continue;
                    }
                    const auto materialized =
                        LC_CurveOffset::materializeBranches(*source, generated, *options, LC_CurveOffset::makeDirectSourceBudget());
                    REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
                    REQUIRE_FALSE(materialized.entities.empty());
                    const auto* firstBranch = dynamic_cast<const RS_Spline*>(materialized.entities.front().get());
                    REQUIRE(firstBranch);
                    REQUIRE(firstBranch->validate());
                    const RS_SplineData expectedCache = firstBranch->getData();
                    const RS_Spline expectedSpline{nullptr, expectedCache};
                    const bool shouldPromote = materialized.entities.size() == 1;
                    ++importedByCell[scaleIndex][ratioIndex];

                    for (const auto& format : formats) {
#ifndef DWGSUPPORT
                        if (format.dwg)
                            continue;
#endif
                        xd::Definition metadata;
                        metadata.source = definition;
                        xd::Options encoding;
                        if (format.dwg) {
                            encoding.eed = true;
                            encoding.textEncoding = xd::TextEncoding::Utf16;
                        }
                        else {
                            encoding.asciiDxf = false;
                        }
                        const auto tags = xd::forExport({std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))}, metadata, encoding);
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

                        const auto input =
                            directory.file(std::to_string(scaleIndex) + "-" + std::to_string(ratioIndex) + "-" +
                                           std::to_string(variantIndex) + "-" + std::to_string(normalizedDistance) + "-" + format.suffix);
#ifdef DWGSUPPORT
                        if (format.dwg)
                            f::dwgFile(input, cache, format.version);
                        else
                            f::binaryDxf(input, cache, format.version);
#else
                        f::binaryDxf(input, cache, format.version);
#endif
                        const auto outcome = stagedImport(input, format.lane);
                        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
                        REQUIRE(outcome.graphic);
                        CHECK(outcome.admitted == (shouldPromote ? 1u : 0u));
                        CHECK(outcome.downgraded == (shouldPromote ? 0u : 1u));
                        const auto* imported = f::sole(*outcome.graphic);
                        REQUIRE(imported);
                        const auto* nativeOffset = dynamic_cast<const LC_OffsetCurve*>(imported);
                        CHECK((nativeOffset != nullptr) == shouldPromote);
                        const auto* importedSpline = dynamic_cast<const RS_Spline*>(imported);
                        REQUIRE(importedSpline);
                        const RS_Spline actualSpline{nullptr, importedSpline->getData()};
                        double maximumDeviation = 0.0;
                        for (int sample = 0; sample <= 128; ++sample) {
                            const double parameter = static_cast<double>(sample) / 128.0;
                            const RS_Vector expected = expectedSpline.getPointAt(parameter);
                            const RS_Vector actual = actualSpline.getPointAt(parameter);
                            REQUIRE(expected.valid);
                            REQUIRE(actual.valid);
                            maximumDeviation = std::max(maximumDeviation, std::hypot(expected.x - actual.x, expected.y - actual.y));
                        }
                        const double roundingAllowance =
                            64.0 * std::numeric_limits<double>::epsilon() * std::max(std::abs(translation.x), std::abs(translation.y));
                        CHECK(maximumDeviation <= 4.0 * options->tolerance.requestedGeometry + roundingAllowance);

                        const auto expectedSource = values::encodeSourceDefinition(definition);
                        REQUIRE(expectedSource);
                        std::optional<LC_OffsetSourceDefinition> receivedDefinition;
                        if (shouldPromote) {
                            REQUIRE(nativeOffset);
                            REQUIRE(nativeOffset->sourceDefinition());
                            receivedDefinition = *nativeOffset->sourceDefinition();
                        }
                        else {
                            const auto parsed = xd::parseAndStrip(imported->getDrwExtData(), encoding);
                            REQUIRE(parsed.status == xd::Status::Schema2);
                            REQUIRE(parsed.definition);
                            REQUIRE(parsed.definition->source);
                            receivedDefinition = *parsed.definition->source;
                        }
                        const auto receivedSource = values::encodeSourceDefinition(*receivedDefinition);
                        REQUIRE(receivedSource);
                        CHECK(QCborValue(*expectedSource).toCbor() == QCborValue(*receivedSource).toCbor());
                    }
                }
            }
            CHECK(importedByCell[scaleIndex][ratioIndex] + refusedByCell[scaleIndex][ratioIndex] ==
                  variants.size() * normalizedDistances.size());
        }
    }
    std::size_t importedCases = 0;
    std::size_t refusedCases = 0;
    for (const auto& row : importedByCell)
        importedCases += std::accumulate(row.begin(), row.end(), std::size_t{0});
    for (const auto& row : refusedByCell)
        refusedCases += std::accumulate(row.begin(), row.end(), std::size_t{0});
    CHECK(importedCases > 0);
    CHECK(refusedCases > 0);
}

TEST_CASE("multi-branch rational spline import refuses partial promotion on both sides",
          "[lc-offsetcurve][import-session][typed-source][spline-variant]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-rational-knot-spline-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;

    RS_SplineData rational{2, false};
    rational.controlPoints = {{0, 0}, {1, 2}, {2, -1}, {3, 2}, {4, 0}};
    rational.knotslist = {0, 0, 0, .3, .7, 1, 1, 1};
    rational.weights = {1, .6, 1.8, .8, 1};
    const auto side = GENERATE(LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right);
    const double signedDistance = side == LC_CurveOffsetSide::Left ? .2 : -.2;
    CAPTURE(side, signedDistance);
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{rational}, signedDistance, 1e-4, false};
    auto source = definition.source.makeEntity();
    REQUIRE(source);
    const double magnitude = std::abs(signedDistance);
    const auto options = LC_OffsetWorkerTolerance::directOptions(
        *source, magnitude, definition.relativeTolerance).value();
    const auto generated = LC_CurveOffset::buildDirectBranches(
        *source, LC_CurveOffset::makeSideRequest(side, magnitude),
                                                               options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
    auto materialized = LC_CurveOffset::materializeBranches(*source, generated, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
    REQUIRE_FALSE(materialized.entities.empty());
    const bool singleCache = materialized.entities.size() == 1;
    CHECK(singleCache == (side == LC_CurveOffsetSide::Right));
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
    const std::array<Format, 5> formats{{{Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
                                         {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
                                         {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
                                         {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
                                         {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};
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

        const auto input = directory.file(std::string(
            side == LC_CurveOffsetSide::Left ? "left-rational-" : "right-rational-") + suffix);
#ifdef DWGSUPPORT
        if (dwg) {
            f::dwgFile(input, cache, version);
            preserveTypedSourceDwg(input, std::string("rational-") +
                (side == LC_CurveOffsetSide::Left ? "left-" : "right-") + suffix);
        }
        else f::binaryDxf(input, cache, version);
#else
        f::binaryDxf(input, cache, version);
#endif
        const auto outcome = stagedImport(input, lane);
        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
        REQUIRE(outcome.graphic);
        CHECK(outcome.admitted == (singleCache ? 1 : 0));
        CHECK(outcome.downgraded == (singleCache ? 0 : 1));
        const auto* imported = f::sole(*outcome.graphic);
        REQUIRE(imported);
        const auto* importedOffset = dynamic_cast<const LC_OffsetCurve*>(imported);
        CHECK((importedOffset != nullptr) == singleCache);
        const auto* importedSpline = dynamic_cast<const RS_Spline*>(imported);
        REQUIRE(importedSpline);
        CHECK(RS_Spline::geometryFingerprint(importedSpline->getData()) ==
              RS_Spline::geometryFingerprint(expectedCache));
        const auto expectedSource = values::encodeSourceDefinition(definition);
        REQUIRE(expectedSource);
        std::optional<LC_OffsetSourceDefinition> actualDefinition;
        if (singleCache) {
            REQUIRE(importedOffset);
            REQUIRE(importedOffset->sourceDefinition());
            actualDefinition = *importedOffset->sourceDefinition();
        } else {
            const auto parsed = xd::parseAndStrip(imported->getDrwExtData(), encoding);
            REQUIRE(parsed.status == xd::Status::Schema2);
            REQUIRE(parsed.definition);
            REQUIRE(parsed.definition->source);
            actualDefinition = *parsed.definition->source;
        }
        const auto actualSource = values::encodeSourceDefinition(*actualDefinition);
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
    const auto side = GENERATE(LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right);
    const double signedDistance = side == LC_CurveOffsetSide::Left ? .05 : -.05;
    CAPTURE(static_cast<int>(side), signedDistance);
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{rational}, signedDistance, 1e-4, false};
    auto source = definition.source.makeEntity();
    REQUIRE(source);
    const auto options = LC_OffsetWorkerTolerance::directOptions(
        *source, std::abs(signedDistance), definition.relativeTolerance).value();
    const auto generated = LC_CurveOffset::buildDirectBranches(*source,
        LC_CurveOffset::makeSideRequest(side, std::abs(signedDistance)), options,
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

        const auto input = directory.file(std::string(side == LC_CurveOffsetSide::Left
            ? "single-cache-left-" : "single-cache-right-") + suffix);
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

TEST_CASE("knot-inserted rational tangent splines promote from DXF and DWG",
          "[lc-offsetcurve][import-session][typed-source][spline-variant][rational][tangent-root][knot]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-knot-inserted-rational-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;
    namespace req = LC_OffsetWorkerRequest;
    namespace result = LC_OffsetWorkerResult;

    struct Fixture {
        double insertion;
        double middleWeight;
        std::size_t multiplicity;
        double secondInsertion{};
        bool asymmetric{};
        int degree{2};
        double thirdInsertion{};
        double fourthInsertion{};
    };
    const std::array<Fixture, 37> fixtures{{
        {0.25, 0.25, 1}, {0.25, 0.5, 1}, {0.25, 2.0, 1}, {0.25, 4.0, 1},
        {0.5, 0.25, 1}, {0.5, 0.5, 1}, {0.5, 2.0, 1}, {0.5, 4.0, 1},
        {0.25, 0.25, 2}, {0.25, 0.5, 2}, {0.25, 2.0, 2}, {0.25, 4.0, 2},
        {0.5, 0.25, 2}, {0.5, 0.5, 2}, {0.5, 2.0, 2}, {0.5, 4.0, 2},
        {0.25, 0.5, 1, 0.75}, {0.25, 2.0, 1, 0.75},
        {0.75, 0.5, 1, 0.25}, {0.75, 2.0, 1, 0.25},
        {0.125, 0.25, 1, 0.625}, {0.125, 4.0, 1, 0.625},
        {0.2, 0.25, 1, 0.8}, {0.2, 4.0, 1, 0.8},
        {0.15, 0.5, 1, 0.7, true}, {0.15, 2.0, 1, 0.7, true},
        {0.3, 0.5, 1, 0.85, true}, {0.3, 2.0, 1, 0.85, true},
        {0.2, 0.75, 1, 0.7, true, 3},
        {0.2, 1.5, 1, 0.7, true, 3},
        {0.4, 0.75, 2, 0.0, true, 3},
        {0.4, 1.5, 2, 0.0, true, 3},
        {0.4, 0.75, 3, 0.0, true, 3},
        {0.4, 1.5, 3, 0.0, true, 3},
        {0.91, 0.4, 1, 0.07, true, 3, 0.37},
        {0.62, 2.5, 1, 0.16, true, 3, 0.84},
        {0.93, 0.85, 1, 0.08, true, 3, 0.72, 0.31}}};
    const auto insertKnot = [](RS_SplineData& data, double knot) {
        struct HomogeneousPoint {
            double x;
            double y;
            double weight;
        };
        const int degree = data.degree;
        const int n = static_cast<int>(data.controlPoints.size()) - 1;
        if (degree < 1 || n < degree ||
            data.weights.size() != data.controlPoints.size() ||
            data.knotslist.size() != data.controlPoints.size() + degree + 1 ||
            !std::isfinite(knot) || knot <= data.knotslist[degree] ||
            knot >= data.knotslist[n + 1]) {
            return false;
        }
        int span = degree;
        while (span < n && data.knotslist[span + 1] <= knot) ++span;
        const int multiplicity = static_cast<int>(std::count(
            data.knotslist.begin(), data.knotslist.end(), knot));
        if (multiplicity >= degree) return false;

        std::vector<HomogeneousPoint> oldPoints;
        oldPoints.reserve(data.controlPoints.size());
        for (std::size_t i = 0; i < data.controlPoints.size(); ++i) {
            const auto& point = data.controlPoints[i];
            const double weight = data.weights[i];
            if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
                !std::isfinite(weight) || weight <= 0.0) {
                return false;
            }
            oldPoints.push_back({point.x * weight, point.y * weight, weight});
        }
        std::vector<HomogeneousPoint> newPoints(data.controlPoints.size() + 1);
        for (int i = 0; i <= span - degree; ++i)
            newPoints[i] = oldPoints[i];
        for (int i = span - multiplicity; i <= n; ++i)
            newPoints[i + 1] = oldPoints[i];
        for (int i = span - degree + 1; i <= span - multiplicity; ++i) {
            const double denominator = data.knotslist[i + degree] - data.knotslist[i];
            if (denominator <= 0.0) return false;
            const double alpha = (knot - data.knotslist[i]) / denominator;
            const auto& previous = oldPoints[i - 1];
            const auto& current = oldPoints[i];
            newPoints[i] = {
                alpha * current.x + (1.0 - alpha) * previous.x,
                alpha * current.y + (1.0 - alpha) * previous.y,
                alpha * current.weight + (1.0 - alpha) * previous.weight};
        }

        data.controlPoints.clear();
        data.weights.clear();
        data.controlPoints.reserve(newPoints.size());
        data.weights.reserve(newPoints.size());
        for (const auto& point : newPoints) {
            if (!std::isfinite(point.weight) || point.weight <= 0.0) return false;
            data.controlPoints.emplace_back(point.x / point.weight,
                                            point.y / point.weight);
            data.weights.push_back(point.weight);
        }
        data.knotslist.insert(data.knotslist.begin() + span + 1, knot);
        return true;
    };
    constexpr std::array<double, 3> scales{0.5, 1.0, 2.0};
    constexpr std::array<std::pair<LC_CurveOffsetSide, double>, 6> offsets{{
        {LC_CurveOffsetSide::Left, 0.5},
        {LC_CurveOffsetSide::Left, 1.0},
        {LC_CurveOffsetSide::Left, 2.0},
        {LC_CurveOffsetSide::Right, 0.5},
        {LC_CurveOffsetSide::Right, 1.0},
        {LC_CurveOffsetSide::Right, 2.0}}};
    const std::array<std::pair<Lane, bool>, 5> lanes{{
        {Lane::Binary14, false}, {Lane::Binary2000, false}, {Lane::Eed2000, true},
        {Lane::Binary18, false}, {Lane::Eed18, true}}};
    LC_OffsetWorkerClient worker;
    qint64 requestId = 1;
    const auto runWorker = [&](req::Request request) {
        std::optional<LC_OffsetWorkerClient::Outcome> outcome;
        REQUIRE(worker.submit(request, LC_OffsetWorkerClient::Mode::Commit,
            [&](LC_OffsetWorkerClient::Outcome completed) {
                outcome.emplace(std::move(completed));
            }));
        QElapsedTimer elapsed;
        elapsed.start();
        while (!outcome) {
            REQUIRE(elapsed.elapsed() < LC_OffsetWorkerClient::Deadlines::commit + 5000);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QThread::msleep(1);
        }
        REQUIRE(outcome->status == LC_OffsetWorkerClient::Status::Completed);
        REQUIRE(outcome->completed);
        CHECK(outcome->completed->isJoined());
        CHECK(worker.healthyAndIdle());
        return std::move(outcome->completed);
    };

    for (std::size_t fixtureIndex = 0; fixtureIndex < fixtures.size(); ++fixtureIndex) {
        const auto [insertion, middleWeight, multiplicity, secondInsertion,
                    asymmetric, degree, thirdInsertion, fourthInsertion] =
            fixtures[fixtureIndex];
        for (const double scale : scales) {
        CAPTURE(fixtureIndex, insertion, middleWeight, multiplicity,
                secondInsertion, asymmetric, degree, thirdInsertion,
                fourthInsertion, scale);
        RS_SplineData sourceData{degree, false};
        RS_SplineData originalData{degree, false};
        if (degree == 3) {
            originalData.controlPoints = {{-2 * scale, 0},
                                          {-1.5 * scale, 2.25 * scale},
                                          {1.25 * scale, 2.5 * scale},
                                          {2.5 * scale, 0.75 * scale}};
            originalData.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
            originalData.weights = {0.8, middleWeight, 1.35, 0.9};
        } else {
            originalData.controlPoints = asymmetric
                ? std::vector<RS_Vector>{{-2 * scale, 0},
                                        {-0.25 * scale, 2.25 * scale},
                                        {2.5 * scale, 0.75 * scale}}
                : std::vector<RS_Vector>{{-2 * scale, 0}, {0, 2 * scale},
                                         {2 * scale, 0}};
            originalData.knotslist = {0, 0, 0, 1, 1, 1};
            originalData.weights = asymmetric
                ? std::vector<double>{0.75, middleWeight, 1.25}
                : std::vector<double>{1, middleWeight, 1};
        }
        RS_Spline originalSource{nullptr, originalData};
        REQUIRE(originalSource.validate());
        sourceData = originalData;
        REQUIRE(insertKnot(sourceData, insertion));
        for (std::size_t i = 1; i < multiplicity; ++i)
            REQUIRE(insertKnot(sourceData, insertion));
        if (secondInsertion > 0.0) REQUIRE(insertKnot(sourceData, secondInsertion));
        if (thirdInsertion > 0.0) REQUIRE(insertKnot(sourceData, thirdInsertion));
        if (fourthInsertion > 0.0) REQUIRE(insertKnot(sourceData, fourthInsertion));
        RS_Spline insertedSource{nullptr, sourceData};
        REQUIRE(insertedSource.validate());
        for (const double parameter : {0.125, 0.25, 0.375, 0.4, 0.5,
                                       0.625, 0.75, 0.875, insertion,
                                       secondInsertion, thirdInsertion,
                                       fourthInsertion}) {
            LC_CurveJet insertedJet;
            LC_CurveJet originalJet;
            REQUIRE(insertedSource.tryEvaluateJet(
                parameter, LC_CurveEvaluationSide::Interior, insertedJet));
            REQUIRE(originalSource.tryEvaluateJet(
                parameter, LC_CurveEvaluationSide::Interior, originalJet));
            CHECK(insertedJet.point.distanceTo(originalJet.point) < 1e-12 * scale);
        }

        for (const auto& [side, distanceScale] : offsets) {
            CAPTURE(distanceScale);
            const double signedDistance =
                (side == LC_CurveOffsetSide::Left ? 0.05 : -0.05) *
                scale * distanceScale;
            const LC_OffsetSourceDefinition definition{
                LC_OffsetSourceGeometry{sourceData}, signedDistance, 1e-4 * scale, false};
            auto source = definition.source.makeEntity();
            REQUIRE(source);
            const auto options = LC_OffsetWorkerTolerance::directOptions(
                *source, std::abs(signedDistance), definition.relativeTolerance);
            REQUIRE(options);
            const auto generated = LC_CurveOffset::buildDirectBranches(
                *source,
                LC_CurveOffset::makeSideRequest(side, std::abs(signedDistance)),
                *options, LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
            const auto materialized = LC_CurveOffset::materializeBranches(
                *source, generated, *options, LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
            REQUIRE(materialized.entities.size() == 1);
            const auto* expectedSpline = dynamic_cast<const RS_Spline*>(
                materialized.entities.front().get());
            REQUIRE(expectedSpline);
            const RS_SplineData expectedCache = expectedSpline->getData();

            const auto generation = runWorker({
                {requestId++, 1, 1, 1}, req::GenerateSource{definition, {}}});
            const auto& generationReply = generation->reply();
            REQUIRE(generationReply.failure == result::Failure::None);
            REQUIRE(generationReply.operation == 7);
            REQUIRE(std::holds_alternative<result::Native>(generationReply.result));
            const auto& native = std::get<result::Native>(generationReply.result);
            REQUIRE(native.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(native.ordinaryStatus == result::OrdinaryStatus::Complete);
            REQUIRE(native.ordinary.size() == 1);
            REQUIRE(std::holds_alternative<RS_SplineData>(native.ordinary.front()));
            CHECK(RS_Spline::geometryFingerprint(
                      std::get<RS_SplineData>(native.ordinary.front())) ==
                  RS_Spline::geometryFingerprint(expectedCache));

            for (const auto& [lane, dwg] : lanes) {
#ifndef DWGSUPPORT
                if (dwg) continue;
#endif
                xd::Definition carrier;
                carrier.source = definition;
                xd::Options encoding;
                if (dwg) {
                    encoding.eed = true;
                    encoding.textEncoding = xd::TextEncoding::Utf16;
                } else {
                    encoding.asciiDxf = false;
                }
                const auto tags = xd::forExport(
                    {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
                    carrier, encoding);
                REQUIRE(tags);
                DRW_Spline cache = f::raw(1, *tags);
                cache.controllist.clear();
                cache.knotslist.clear();
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

                const auto input = directory.file(
                    std::to_string(fixtureIndex) + "-" +
                    std::to_string(scale) + "-" +
                    std::to_string(distanceScale) + "-" +
                    (side == LC_CurveOffsetSide::Left ? "left-" : "right-") +
                    std::to_string(static_cast<int>(lane)) + (dwg ? ".dwg" : ".dxf"));
#ifdef DWGSUPPORT
                if (dwg) f::dwgFile(input, cache, lane == Lane::Eed18
                    ? DRW::AC1032 : DRW::AC1015);
                else f::binaryDxf(input, cache, lane == Lane::Binary14
                    ? DRW::AC1014 : lane == Lane::Binary2000
                    ? DRW::AC1015 : DRW::AC1032);
#else
                f::binaryDxf(input, cache, lane == Lane::Binary14
                    ? DRW::AC1014 : lane == Lane::Binary2000
                    ? DRW::AC1015 : DRW::AC1032);
#endif
                const auto outcome = stagedImport(input, lane);
                CAPTURE(fixtureIndex, insertion, middleWeight, multiplicity,
                        secondInsertion, asymmetric, degree, scale,
                        static_cast<int>(side),
                        static_cast<int>(lane));
                REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
                REQUIRE(outcome.graphic);
                CHECK(outcome.admitted == 1);
                CHECK(outcome.downgraded == 0);
                const auto* imported = dynamic_cast<const LC_OffsetCurve*>(
                    f::sole(*outcome.graphic));
                REQUIRE(imported);
                REQUIRE(imported->sourceDefinition());
                const auto expectedSource = values::encodeSourceDefinition(definition);
                const auto actualSource = values::encodeSourceDefinition(
                    *imported->sourceDefinition());
                REQUIRE(expectedSource);
                REQUIRE(actualSource);
                CHECK(QCborValue(*expectedSource).toCbor() ==
                      QCborValue(*actualSource).toCbor());
                CHECK(RS_Spline::geometryFingerprint(imported->getData()) ==
                      RS_Spline::geometryFingerprint(expectedCache));
            }
        }
    }
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
    fitVariants.back().fitStartTangent = {1.0, 0.0};
    fitVariants.back().fitEndTangent = {0.0, -1.0};

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
        const bool tangentConstrained = fitVariants[variant].fitTangentsConstrained;
        LC_SplinePointsData displayedGeometry = fitVariants[variant];
        displayedGeometry.fitTangentsConstrained = false;
        auto source = LC_OffsetSourceGeometry{displayedGeometry}.makeEntity();
        REQUIRE(source);
        const auto options = LC_OffsetWorkerTolerance::directOptions(
            *source, magnitude, definition.relativeTolerance).value();
        const auto generated = LC_CurveOffset::buildDirectBranches(*source,
            LC_CurveOffset::makeSideRequest(side, magnitude), options,
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(generated.status == LC_CurveOffsetStatus::Ok);
        auto materialized = LC_CurveOffset::materializeBranches(*source, generated, options,
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(materialized.status == LC_CurveOffsetStatus::Ok);
        REQUIRE_FALSE(materialized.entities.empty());
        const bool singleCache = !tangentConstrained && materialized.entities.size() == 1;
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
                CHECK(fitSource->fitStartTangent.x ==
                      fitVariants[variant].fitStartTangent.x);
                CHECK(fitSource->fitStartTangent.y ==
                      fitVariants[variant].fitStartTangent.y);
                CHECK(fitSource->fitEndTangent.x ==
                      fitVariants[variant].fitEndTangent.x);
                CHECK(fitSource->fitEndTangent.y ==
                      fitVariants[variant].fitEndTangent.y);
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
                REQUIRE(preserved.status == (tangentConstrained
                    ? xd::Status::Schema3 : xd::Status::Schema2));
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
            if (singleCache) {
                const auto nativeOutput = directory.file("native-fit-points-" +
                    suffix + sideName + distanceName + version + extension);
                REQUIRE(lc::test::exportDrawing(*outcome.graphic, nativeOutput,
                                                 ::format(lane)));
                const auto nativeRoundTrip = stagedImport(nativeOutput, lane);
                INFO("native fit-point writer round-trip " << version << extension);
                REQUIRE(nativeRoundTrip.status == LC_OffsetImportSession::Status::Complete);
                REQUIRE(nativeRoundTrip.graphic);
                CHECK(nativeRoundTrip.admitted == 1);
                CHECK(nativeRoundTrip.downgraded == 0);
                const auto* roundTripped = dynamic_cast<const LC_OffsetCurve*>(
                    f::sole(*nativeRoundTrip.graphic));
                REQUIRE(roundTripped);
                REQUIRE(roundTripped->sourceDefinition());
                const auto roundTripSource = values::encodeSourceDefinition(
                    *roundTripped->sourceDefinition());
                REQUIRE(roundTripSource);
                CHECK(QCborValue(*expectedSource).toCbor() ==
                      QCborValue(*roundTripSource).toCbor());
                CHECK(RS_Spline::geometryFingerprint(roundTripped->getData()) ==
                      RS_Spline::geometryFingerprint(expectedCache));
            }
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

TEST_CASE("scaled translated spline caches persist through staged DXF and DWG import",
          "[lc-offsetcurve][import-session][typed-source][spline-variant][precision][combined]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-spline-translated-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;
    namespace req = LC_OffsetWorkerRequest;
    namespace result = LC_OffsetWorkerResult;

    RS_SplineData rational{2, false};
    rational.controlPoints = {{0, 0}, {1, 2}, {2, -1}, {3, 2}, {4, 0}};
    rational.knotslist = {0, 0, 0, .3, .7, 1, 1, 1};
    rational.weights = {1, .6, 1.8, .8, 1};
    RS_SplineData cubic{3, false};
    cubic.controlPoints = {{0, 0}, {1, 2}, {2, 2}, {3, -1}, {4, -1}, {5, 1}};
    cubic.knotslist = {0, 0, 0, 0, .4, .4, 1, 1, 1, 1};
    cubic.weights = {1, .7, 1.4, .9, 1.6, 1};
    LC_SplinePointsData fitPoints{false, false};
    fitPoints.splinePoints = {{0, 0}, {1, 2}, {2, 2}, {3, -1}, {4, 0}};
    fitPoints.fitTolerance = 1e-6;
    const std::array<LC_OffsetSourceGeometry, 3> variants{
        LC_OffsetSourceGeometry{rational}, LC_OffsetSourceGeometry{cubic},
        LC_OffsetSourceGeometry{fitPoints}};
    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};
    constexpr std::array<double, 2> scales{1.0, 1e3};
    constexpr std::array<double, 2> translationRatios{1e6, 1e9};
    constexpr std::array<double, 4> normalizedDistances{-.4, -.1, .1, .4};
    LC_OffsetWorkerClient worker;
    qint64 nextRequestId = 1;
    const auto runWorker = [&](req::Request request) {
        std::optional<LC_OffsetWorkerClient::Outcome> outcome;
        REQUIRE(worker.submit(request, LC_OffsetWorkerClient::Mode::Commit,
            [&](LC_OffsetWorkerClient::Outcome completed) { outcome.emplace(std::move(completed)); }));
        QElapsedTimer elapsed;
        elapsed.start();
        while (!outcome) {
            REQUIRE(elapsed.elapsed() < LC_OffsetWorkerClient::Deadlines::commit + 5000);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QThread::msleep(1);
        }
        REQUIRE(outcome->status == LC_OffsetWorkerClient::Status::Completed);
        REQUIRE(outcome->completed);
        CHECK(outcome->completed->isJoined());
        CHECK(worker.healthyAndIdle());
        return std::move(outcome->completed);
    };
    const auto transform = [](LC_OffsetSourceGeometry source, const double scale,
                              const RS_Vector& translation) {
        const auto point = [&](RS_Vector& p) {
            p.x = p.x * scale + translation.x;
            p.y = p.y * scale + translation.y;
            p.z *= scale;
        };
        std::visit([&](auto& data) {
            using T = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<T, RS_SplineData>) {
                for (auto& p : data.controlPoints) point(p);
                for (auto& p : data.fitPoints) point(p);
            } else if constexpr (std::is_same_v<T, LC_SplinePointsData>) {
                for (auto& p : data.controlPoints) point(p);
                for (auto& p : data.splinePoints) point(p);
            }
        }, source.value);
        return source;
    };
    const auto asCache = [](const req::Cutter& item) -> std::optional<RS_SplineData> {
        if (const auto* spline = std::get_if<RS_SplineData>(&item)) return *spline;
        if (const auto* segment = std::get_if<req::Segment>(&item)) {
            RS_SplineData line{1, false};
            line.controlPoints = {segment->start, segment->end};
            line.knotslist = {0, 0, 1, 1};
            line.weights = {1, 1};
            return line;
        }
        return std::nullopt;
    };

    std::size_t generatedCells = 0;
    std::size_t admittedCells = 0;
    std::size_t refusedCells = 0;
    std::size_t importedFiles = 0;
    std::size_t promotedFiles = 0;
    std::size_t fallbackFiles = 0;
    for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
        for (const double scale : scales) {
            for (const double ratio : translationRatios) {
                const RS_Vector translation{ratio * scale, -ratio * scale};
                const auto source = transform(variants[variantIndex], scale, translation);
                REQUIRE(source.isWellFormed());
                for (const double normalizedDistance : normalizedDistances) {
                    ++generatedCells;
                    CAPTURE(variantIndex, scale, ratio, normalizedDistance);
                    const LC_OffsetSourceDefinition definition{
                        source, normalizedDistance * scale, 1e-4, false};
                    auto generatedCompletion = runWorker({
                        {nextRequestId++, 1, 1, 1}, req::GenerateSource{definition, {}}});
                    const auto& generatedReply = generatedCompletion->reply();
                    REQUIRE(generatedReply.failure == result::Failure::None);
                    REQUIRE(generatedReply.operation == 7);
                    REQUIRE(std::holds_alternative<result::Native>(generatedReply.result));
                    const auto& generated = std::get<result::Native>(generatedReply.result);
                    if (generated.engineStatus != LC_CurveOffsetStatus::Ok ||
                        generated.ordinaryStatus != result::OrdinaryStatus::Complete ||
                        generated.ordinary.size() != 1) {
                        CHECK((generated.ordinary.empty() || generated.ordinary.size() > 1));
                        ++refusedCells;
                        continue;
                    }
                    auto cache = asCache(generated.ordinary.front());
                    REQUIRE(cache);
                    RS_Spline expectedSpline{nullptr, *cache};
                    REQUIRE(expectedSpline.validate());
                    const auto fingerprint = RS_Spline::geometryFingerprint(*cache);
                    auto validationCompletion = runWorker({
                        {nextRequestId++, 1, 1, 1}, req::ValidateSourceCache{*cache, definition, {}}});
                    const auto& validationReply = validationCompletion->reply();
                    REQUIRE(validationReply.failure == result::Failure::None);
                    REQUIRE(validationReply.operation == 8);
                    REQUIRE(std::holds_alternative<result::Native>(validationReply.result));
                    const auto& validation = std::get<result::Native>(validationReply.result);
                    if (validation.refusal != LC_OffsetCurveRefusal::None || !validation.changed) {
                        CHECK_FALSE(validation.changed);
                        CHECK(RS_Spline::geometryFingerprint(*cache) == fingerprint);
                        ++refusedCells;
                        continue;
                    }
                    ++admittedCells;
                    const auto sourceEntity = source.makeEntity();
                    REQUIRE(sourceEntity);
                    const auto options = LC_OffsetWorkerTolerance::directOptions(definition, *sourceEntity);
                    REQUIRE(options);
                    const double ulp = std::max(
                        std::nextafter(std::abs(translation.x),
                                       std::numeric_limits<double>::infinity()) - std::abs(translation.x),
                        std::nextafter(std::abs(translation.y),
                                       std::numeric_limits<double>::infinity()) - std::abs(translation.y));
                    const double deviationLimit = 4.0 * options->tolerance.requestedGeometry + 64.0 * ulp;
                    const auto expectedSource = values::encodeSourceDefinition(definition);
                    REQUIRE(expectedSource);

                    for (const auto& fileFormat : formats) {
#ifndef DWGSUPPORT
                        if (fileFormat.dwg) continue;
#endif
                        xd::Definition metadata;
                        metadata.source = definition;
                        xd::Options encoding;
                        if (fileFormat.dwg) {
                            encoding.eed = true;
                            if (fileFormat.version == DRW::AC1015) {
                                encoding.textEncoding = xd::TextEncoding::Legacy;
                                encoding.legacyEncodedBytes = [](std::string_view text)
                                    -> std::optional<std::size_t> { return text.size(); };
                            } else {
                                encoding.textEncoding = xd::TextEncoding::Utf16;
                            }
                        } else {
                            encoding.asciiDxf = false;
                        }
                        const auto tags = xd::forExport(
                            {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
                            metadata, encoding);
                        REQUIRE(tags);
                        DRW_Spline carrier = f::raw(1, *tags);
                        carrier.controllist.clear();
                        for (const auto& p : cache->controlPoints)
                            carrier.controllist.push_back(std::make_shared<DRW_Coord>(p.x, p.y, 0));
                        carrier.knotslist = cache->knotslist;
                        carrier.weightlist = cache->weights;
                        carrier.degree = cache->degree;
                        carrier.flags = 8;
                        carrier.m_scenario = 1;
                        carrier.ncontrol = carrier.controllist.size();
                        carrier.nknots = carrier.knotslist.size();
                        carrier.nfit = 0;
                        const auto input = directory.file(std::to_string(variantIndex) + "-" +
                            std::to_string(scale) + "-" + std::to_string(ratio) + "-" +
                            std::to_string(normalizedDistance) + "-" + fileFormat.suffix);
#ifdef DWGSUPPORT
                        if (fileFormat.dwg) f::dwgFile(input, carrier, fileFormat.version);
                        else f::binaryDxf(input, carrier, fileFormat.version);
#else
                        f::binaryDxf(input, carrier, fileFormat.version);
#endif
                        const auto outcome = stagedImport(input, fileFormat.lane);
                        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
                        REQUIRE(outcome.graphic);
                        REQUIRE(outcome.admitted + outcome.downgraded == 1);
                        auto* imported = f::sole(*outcome.graphic);
                        REQUIRE(imported);
                        const auto* native = dynamic_cast<const LC_OffsetCurve*>(imported);
                        CHECK((native != nullptr) == (outcome.admitted == 1));
                        CHECK((native == nullptr) == (outcome.downgraded == 1));
                        std::optional<QCborArray> actualSource;
                        if (native) {
                            REQUIRE(native->sourceDefinition());
                            actualSource = values::encodeSourceDefinition(*native->sourceDefinition());
                            ++promotedFiles;
                        } else {
                            REQUIRE_FALSE(outcome.diagnostics.empty());
                            const auto parsed = xd::parseAndStrip(imported->getDrwExtData(), encoding);
                            REQUIRE(parsed.status == xd::Status::Schema2);
                            REQUIRE(parsed.definition);
                            REQUIRE(parsed.definition->source);
                            actualSource = values::encodeSourceDefinition(*parsed.definition->source);
                            ++fallbackFiles;
                        }
                        REQUIRE(actualSource);
                        CHECK(QCborValue(*expectedSource).toCbor() ==
                              QCborValue(*actualSource).toCbor());
                        const auto* importedSpline = dynamic_cast<const RS_Spline*>(imported);
                        REQUIRE(importedSpline);
                        for (int sample = 0; sample <= 128; ++sample) {
                            const double t = static_cast<double>(sample) / 128.0;
                            const RS_Vector expected = expectedSpline.getPointAt(t);
                            const RS_Vector actual = importedSpline->getPointAt(t);
                            REQUIRE(expected.valid);
                            REQUIRE(actual.valid);
                            CHECK(std::hypot(actual.x - expected.x, actual.y - expected.y) <=
                                  deviationLimit);
                        }
                        ++importedFiles;
                    }
                }
            }
        }
    }
    CHECK(generatedCells == variants.size() * scales.size() * translationRatios.size() *
          normalizedDistances.size());
    CHECK(admittedCells + refusedCells == generatedCells);
#ifndef DWGSUPPORT
    CHECK(importedFiles == admittedCells * 3);
#else
    CHECK(importedFiles == admittedCells * formats.size());
#endif
    CHECK(promotedFiles + fallbackFiles == importedFiles);
    CHECK(promotedFiles > 0);
}

TEST_CASE("scaled translated parabola caches persist through staged DXF and DWG import",
          "[lc-offsetcurve][import-session][typed-source][parabola][precision][combined]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-parabola-translated-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;
    namespace req = LC_OffsetWorkerRequest;
    namespace result = LC_OffsetWorkerResult;

    const std::array<LC_ParabolaData, 3> variants{
        LC_ParabolaData{{RS_Vector{0, 0}, RS_Vector{5, 10}, RS_Vector{10, 0}}},
        LC_ParabolaData{{RS_Vector{10, 0}, RS_Vector{5, 10}, RS_Vector{0, 0}}},
        LC_ParabolaData{{RS_Vector{3, -7}, RS_Vector{2.3301270189221936, 4.160254037844386},
                         RS_Vector{11.660254037844386, -2}}}};
    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};
    constexpr std::array<double, 3> scales{1e-3, 1.0, 1e3};
    constexpr std::array<double, 2> translationRatios{1e6, 1e9};
    constexpr std::array<double, 6> normalizedDistances{-3.0, -2.5, -2.0, 2.0, 2.5, 3.0};

    LC_OffsetWorkerClient worker;
    qint64 nextRequestId = 1;
    const auto runWorker = [&](req::Request request) {
        std::optional<LC_OffsetWorkerClient::Outcome> outcome;
        REQUIRE(worker.submit(request, LC_OffsetWorkerClient::Mode::Commit,
            [&](LC_OffsetWorkerClient::Outcome completed) { outcome.emplace(std::move(completed)); }));
        QElapsedTimer elapsed;
        elapsed.start();
        while (!outcome) {
            REQUIRE(elapsed.elapsed() < LC_OffsetWorkerClient::Deadlines::commit + 5000);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QThread::msleep(1);
        }
        REQUIRE(outcome->status == LC_OffsetWorkerClient::Status::Completed);
        REQUIRE(outcome->completed);
        CHECK(outcome->completed->isJoined());
        CHECK(worker.healthyAndIdle());
        return std::move(outcome->completed);
    };
    const auto transform = [](const LC_ParabolaData& source, const double scale,
                              const RS_Vector& translation) {
        auto points = source.m_controlPoints;
        for (auto& point : points) {
            point.x = point.x * scale + translation.x;
            point.y = point.y * scale + translation.y;
            point.z *= scale;
        }
        return LC_OffsetSourceGeometry{LC_ParabolaData{points}};
    };
    const auto asSpline = [](const req::Cutter& item) {
        if (const auto* spline = std::get_if<RS_SplineData>(&item)) return *spline;
        const auto& segment = std::get<req::Segment>(item);
        RS_SplineData line{1, false};
        line.controlPoints = {segment.start, segment.end};
        line.knotslist = {0, 0, 1, 1};
        line.weights = {1, 1};
        return line;
    };
    const auto coordinateUlp = [](const double value) {
        return std::nextafter(std::abs(value), std::numeric_limits<double>::infinity()) -
               std::abs(value);
    };

    std::size_t cells = 0;
    std::size_t generatedCells = 0;
    std::size_t generationRefusedCells = 0;
    std::size_t admittedTranslatedCaches = 0;
    std::size_t refusedTranslatedCacheCells = 0;
    std::size_t validatedBranches = 0;
    std::size_t expectedValidationBranches = 0;
    std::size_t centeredCacheAdmittedCells = 0;
    std::size_t centeredCacheRefusedCells = 0;
    std::size_t importedFiles = 0;
    std::size_t promotedFiles = 0;
    std::size_t fallbackFiles = 0;

    for (std::size_t variantIndex = 0; variantIndex < variants.size(); ++variantIndex) {
        for (const double scale : scales) {
            for (const double ratio : translationRatios) {
                const RS_Vector translation{ratio * scale, -ratio * scale};
                const auto centeredSource = transform(variants[variantIndex], scale, {});
                const auto translatedSource = transform(variants[variantIndex], scale, translation);
                REQUIRE(centeredSource.isWellFormed());
                REQUIRE(translatedSource.isWellFormed());
                for (const double normalizedDistance : normalizedDistances) {
                    ++cells;
                    CAPTURE(variantIndex, scale, ratio, normalizedDistance);
                    const double signedDistance = normalizedDistance * scale;
                    const LC_OffsetSourceDefinition centeredDefinition{
                        centeredSource, signedDistance, 1e-4, false};
                    const LC_OffsetSourceDefinition translatedDefinition{
                        translatedSource, signedDistance, 1e-4, false};

                    const auto centeredCompletion = runWorker({
                        {nextRequestId++, 1, 1, 1}, req::GenerateSource{centeredDefinition, {}}});
                    const auto translatedCompletion = runWorker({
                        {nextRequestId++, 1, 1, 1}, req::GenerateSource{translatedDefinition, {}}});
                    const auto& centeredReply = centeredCompletion->reply();
                    const auto& translatedReply = translatedCompletion->reply();
                    REQUIRE(centeredReply.failure == result::Failure::None);
                    REQUIRE(translatedReply.failure == result::Failure::None);
                    REQUIRE(centeredReply.operation == 7);
                    REQUIRE(translatedReply.operation == 7);
                    REQUIRE(std::holds_alternative<result::Native>(centeredReply.result));
                    REQUIRE(std::holds_alternative<result::Native>(translatedReply.result));
                    const auto& centered = std::get<result::Native>(centeredReply.result);
                    const auto& translated = std::get<result::Native>(translatedReply.result);
                    const auto generated = [](const result::Native& output) {
                        return output.engineStatus == LC_CurveOffsetStatus::Ok &&
                            output.ordinaryStatus == result::OrdinaryStatus::Complete &&
                            !output.ordinary.empty();
                    };
                    const bool centeredGenerated = generated(centered);
                    const bool translatedGenerated = generated(translated);
                    CHECK(centered.engineStatus == translated.engineStatus);
                    CHECK(centered.ordinaryStatus == translated.ordinaryStatus);
                    CHECK(centered.ordinary.size() == translated.ordinary.size());
                    REQUIRE(centeredGenerated == translatedGenerated);
                    if (!centeredGenerated) {
                        CHECK(centered.ordinary.empty());
                        CHECK(translated.ordinary.empty());
                        CHECK_FALSE(centered.changed);
                        CHECK_FALSE(translated.changed);
                        ++generationRefusedCells;
                        continue;
                    }
                    ++generatedCells;
                    REQUIRE(centered.ordinary.size() == translated.ordinary.size());

                    const auto centeredEntity = centeredSource.makeEntity();
                    const auto translatedEntity = translatedSource.makeEntity();
                    REQUIRE(centeredEntity);
                    REQUIRE(translatedEntity);
                    const auto centeredOptions = LC_OffsetWorkerTolerance::directOptions(
                        centeredDefinition, *centeredEntity);
                    const auto translatedOptions = LC_OffsetWorkerTolerance::directOptions(
                        translatedDefinition, *translatedEntity);
                    REQUIRE(centeredOptions);
                    REQUIRE(translatedOptions);
                    const double ulpAllowance = 64.0 * std::max(
                        coordinateUlp(translation.x), coordinateUlp(translation.y));
                    const double deviationLimit = 4.0 * std::max(
                        centeredOptions->tolerance.requestedGeometry,
                        translatedOptions->tolerance.requestedGeometry) + ulpAllowance;
                    std::vector<RS_Spline> centeredSplines;
                    std::vector<RS_Spline> translatedSplines;
                    for (const auto& item : centered.ordinary)
                        centeredSplines.emplace_back(nullptr, asSpline(item));
                    for (const auto& item : translated.ordinary)
                        translatedSplines.emplace_back(nullptr, asSpline(item));
                    std::vector<bool> matched(translatedSplines.size(), false);
                    const auto samples = [&](const RS_Spline& spline, const RS_Vector& offset,
                                             const int subdivisions) {
                        double first = 0.0;
                        double last = 0.0;
                        REQUIRE(spline.getParameterDomain(first, last));
                        std::vector<RS_Vector> points;
                        points.reserve(static_cast<std::size_t>(subdivisions) + 1);
                        for (int sample = 0; sample <= subdivisions; ++sample) {
                            const double fraction = static_cast<double>(sample) / subdivisions;
                            const double parameter = first + (last - first) * fraction;
                            RS_Vector point = spline.getPointAt(parameter);
                            REQUIRE(point.valid);
                            point.x -= offset.x;
                            point.y -= offset.y;
                            points.push_back(point);
                        }
                        return points;
                    };
                    const auto directedDistance = [](const std::vector<RS_Vector>& points,
                                                     const std::vector<RS_Vector>& segments) {
                        double maximum = 0.0;
                        for (const auto& point : points) {
                            double minimum = std::numeric_limits<double>::infinity();
                            for (std::size_t i = 1; i < segments.size(); ++i) {
                                const RS_Vector direction = segments[i] - segments[i - 1];
                                const RS_Vector relative = point - segments[i - 1];
                                const double lengthSquared = direction.x * direction.x +
                                                             direction.y * direction.y;
                                const double parameter = lengthSquared > 0.0
                                    ? std::clamp((relative.x * direction.x + relative.y * direction.y) /
                                                 lengthSquared, 0.0, 1.0)
                                    : 0.0;
                                const RS_Vector nearest = segments[i - 1] + direction * parameter;
                                minimum = std::min(minimum,
                                    std::hypot(point.x - nearest.x, point.y - nearest.y));
                            }
                            maximum = std::max(maximum, minimum);
                        }
                        return maximum;
                    };
                    const auto deviation = [&](const RS_Spline& local, const RS_Spline& world) {
                        const auto localChecks = samples(local, {}, 128);
                        const auto worldChecks = samples(world, translation, 128);
                        const auto localCurve = samples(local, {}, 2048);
                        const auto worldCurve = samples(world, translation, 2048);
                        return std::max(directedDistance(localChecks, worldCurve),
                                        directedDistance(worldChecks, localCurve));
                    };
                    for (const auto& local : centeredSplines) {
                        double bestDeviation = std::numeric_limits<double>::infinity();
                        std::size_t bestIndex = translatedSplines.size();
                        for (std::size_t candidate = 0; candidate < translatedSplines.size(); ++candidate) {
                            if (matched[candidate]) continue;
                            const double candidateDeviation = deviation(local, translatedSplines[candidate]);
                            if (candidateDeviation < bestDeviation) {
                                bestDeviation = candidateDeviation;
                                bestIndex = candidate;
                            }
                        }
                        REQUIRE(bestIndex < translatedSplines.size());
                        matched[bestIndex] = true;
                        CHECK(bestDeviation <= deviationLimit);
                    }

                    const auto validateEveryBranch = [&](const result::Native& generatedResult,
                                                         const LC_OffsetSourceDefinition& sourceDefinition) {
                        expectedValidationBranches += generatedResult.ordinary.size();
                        bool singleCacheAdmitted = false;
                        for (const auto& item : generatedResult.ordinary) {
                            auto cache = asSpline(item);
                            const auto fingerprint = RS_Spline::geometryFingerprint(cache);
                            const auto validationCompletion = runWorker({
                                {nextRequestId++, 1, 1, 1},
                                req::ValidateSourceCache{cache, sourceDefinition, {}}});
                            const auto& validationReply = validationCompletion->reply();
                            REQUIRE(validationReply.failure == result::Failure::None);
                            REQUIRE(validationReply.operation == 8);
                            REQUIRE(std::holds_alternative<result::Native>(validationReply.result));
                            const auto& validation = std::get<result::Native>(validationReply.result);
                            ++validatedBranches;
                            if (validation.refusal == LC_OffsetCurveRefusal::None) {
                                CHECK(validation.changed);
                                CHECK(generatedResult.ordinary.size() == 1);
                                singleCacheAdmitted = generatedResult.ordinary.size() == 1 &&
                                    validation.changed;
                            } else {
                                CHECK_FALSE(validation.changed);
                                CHECK(RS_Spline::geometryFingerprint(cache) == fingerprint);
                                if (generatedResult.ordinary.size() > 1)
                                    CHECK(validation.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                            }
                        }
                        return singleCacheAdmitted;
                    };
                    const bool centeredCacheAdmitted = validateEveryBranch(centered, centeredDefinition);
                    const bool translatedCacheAdmitted = validateEveryBranch(translated, translatedDefinition);
                    if (centeredCacheAdmitted) ++centeredCacheAdmittedCells;
                    else ++centeredCacheRefusedCells;
                    if (!translatedCacheAdmitted) {
                        ++refusedTranslatedCacheCells;
                        continue;
                    }
                    ++admittedTranslatedCaches;

                    const auto cache = asSpline(translated.ordinary.front());
                    RS_Spline expectedSpline{nullptr, cache};
                    REQUIRE(expectedSpline.validate());
                    const auto expectedSource = values::encodeSourceDefinition(translatedDefinition);
                    REQUIRE(expectedSource);
                    for (const auto& format : formats) {
#ifndef DWGSUPPORT
                        if (format.dwg) continue;
#endif
                        xd::Definition metadata;
                        metadata.source = translatedDefinition;
                        xd::Options encoding;
                        if (format.dwg) {
                            encoding.eed = true;
                            if (format.version == DRW::AC1015) {
                                encoding.textEncoding = xd::TextEncoding::Legacy;
                                encoding.legacyEncodedBytes = [](std::string_view text)
                                    -> std::optional<std::size_t> { return text.size(); };
                            } else {
                                encoding.textEncoding = xd::TextEncoding::Utf16;
                            }
                        } else {
                            encoding.asciiDxf = false;
                        }
                        const auto tags = xd::forExport(
                            {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
                            metadata, encoding);
                        REQUIRE(tags);
                        DRW_Spline carrier = f::raw(1, *tags);
                        carrier.controllist.clear();
                        for (const auto& point : cache.controlPoints)
                            carrier.controllist.push_back(
                                std::make_shared<DRW_Coord>(point.x, point.y, 0));
                        carrier.knotslist = cache.knotslist;
                        carrier.weightlist = cache.weights;
                        carrier.degree = cache.degree;
                        carrier.flags = 8;
                        carrier.m_scenario = 1;
                        carrier.ncontrol = carrier.controllist.size();
                        carrier.nknots = carrier.knotslist.size();
                        carrier.nfit = 0;
                        const auto input = directory.file(std::to_string(variantIndex) + "-" +
                            std::to_string(scale) + "-" + std::to_string(ratio) + "-" +
                            std::to_string(normalizedDistance) + "-" + format.suffix);
#ifdef DWGSUPPORT
                        if (format.dwg) f::dwgFile(input, carrier, format.version);
                        else f::binaryDxf(input, carrier, format.version);
#else
                        f::binaryDxf(input, carrier, format.version);
#endif
                        const auto outcome = stagedImport(input, format.lane);
                        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
                        REQUIRE(outcome.graphic);
                        REQUIRE(outcome.admitted + outcome.downgraded == 1);
                        const auto* imported = f::sole(*outcome.graphic);
                        REQUIRE(imported);
                        const auto* native = dynamic_cast<const LC_OffsetCurve*>(imported);
                        CHECK((native != nullptr) == (outcome.admitted == 1));
                        CHECK((native == nullptr) == (outcome.downgraded == 1));
                        std::optional<QCborArray> actualSource;
                        if (native) {
                            REQUIRE(native->sourceDefinition());
                            actualSource = values::encodeSourceDefinition(*native->sourceDefinition());
                            ++promotedFiles;
                        } else {
                            REQUIRE_FALSE(outcome.diagnostics.empty());
                            const auto parsed = xd::parseAndStrip(imported->getDrwExtData(), encoding);
                            REQUIRE(parsed.status == xd::Status::Schema2);
                            REQUIRE(parsed.definition);
                            REQUIRE(parsed.definition->source);
                            actualSource = values::encodeSourceDefinition(*parsed.definition->source);
                            ++fallbackFiles;
                        }
                        REQUIRE(actualSource);
                        CHECK(QCborValue(*expectedSource).toCbor() ==
                              QCborValue(*actualSource).toCbor());
                        const auto* importedSpline = dynamic_cast<const RS_Spline*>(imported);
                        REQUIRE(importedSpline);
                        for (int sample = 0; sample <= 128; ++sample) {
                            const double parameter = static_cast<double>(sample) / 128.0;
                            const RS_Vector expected = expectedSpline.getPointAt(parameter);
                            const RS_Vector actual = importedSpline->getPointAt(parameter);
                            REQUIRE(expected.valid);
                            REQUIRE(actual.valid);
                            CHECK(std::hypot(actual.x - expected.x, actual.y - expected.y) <=
                                  deviationLimit);
                        }
                        ++importedFiles;
                    }
                }
            }
        }
    }

    CHECK(cells == variants.size() * scales.size() * translationRatios.size() *
          normalizedDistances.size());
    CHECK(generatedCells + generationRefusedCells == cells);
    CHECK(admittedTranslatedCaches + refusedTranslatedCacheCells == generatedCells);
    CHECK(centeredCacheAdmittedCells + centeredCacheRefusedCells == generatedCells);
    CHECK(validatedBranches == expectedValidationBranches);
#ifndef DWGSUPPORT
    CHECK(importedFiles == admittedTranslatedCaches * 3);
#else
    CHECK(importedFiles == admittedTranslatedCaches * formats.size());
#endif
    CHECK(promotedFiles + fallbackFiles == importedFiles);
}

TEST_CASE("scaled translated polyline caches persist through staged DXF and DWG import",
          "[lc-offsetcurve][import-session][typed-source][polyline][precision][combined]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-polyline-translated-import");
    namespace xd = LC_OffsetCurveXData;
    namespace values = LC_OffsetWorkerRequest::Values;
    namespace req = LC_OffsetWorkerRequest;
    namespace result = LC_OffsetWorkerResult;

    const double pi = std::acos(-1.0);
    RS_Polyline mixedBulgeEntity;
    mixedBulgeEntity.addVertex({0, 0}, 0.2);
    mixedBulgeEntity.addVertex({8, 0}, -0.2);
    mixedBulgeEntity.addVertex({8, 6});
    mixedBulgeEntity.addVertex({0, 6});
    mixedBulgeEntity.setClosed(true);
    mixedBulgeEntity.endPolyline();
    const auto mixedBulgeSource = LC_OffsetSourceGeometry::capture(mixedBulgeEntity);
    REQUIRE(mixedBulgeSource);
    REQUIRE(mixedBulgeSource->kind() == LC_OffsetSourceGeometry::Kind::Polyline);
    RS_Polyline majorMinorBulgeEntity;
    majorMinorBulgeEntity.addVertex({0, 0}, 2.0);
    majorMinorBulgeEntity.addVertex({10, 0}, -0.2);
    majorMinorBulgeEntity.addVertex({10, 10});
    majorMinorBulgeEntity.addVertex({0, 10});
    majorMinorBulgeEntity.setClosed(true);
    majorMinorBulgeEntity.endPolyline();
    const auto majorMinorBulgeSource = LC_OffsetSourceGeometry::capture(majorMinorBulgeEntity);
    REQUIRE(majorMinorBulgeSource);
    REQUIRE(majorMinorBulgeSource->kind() == LC_OffsetSourceGeometry::Kind::Polyline);
    const auto& majorMinorBulgeData =
        std::get<LC_OffsetPolylineData>(majorMinorBulgeSource->value);
    REQUIRE(majorMinorBulgeData.closed);
    REQUIRE(majorMinorBulgeData.segments.size() == 4);
    const auto* positiveMajorArc =
        std::get_if<RS_ArcData>(&majorMinorBulgeData.segments[0]);
    const auto* negativeMinorArc =
        std::get_if<RS_ArcData>(&majorMinorBulgeData.segments[1]);
    REQUIRE(positiveMajorArc);
    REQUIRE(negativeMinorArc);
    const auto directedSweep = [](const RS_ArcData& arc) {
        double sweep = arc.angle2 - arc.angle1;
        if (arc.reversed && sweep > 0.0) sweep -= 2.0 * std::acos(-1.0);
        if (!arc.reversed && sweep < 0.0) sweep += 2.0 * std::acos(-1.0);
        return sweep;
    };
    CHECK(directedSweep(*positiveMajorArc) > std::acos(-1.0));
    CHECK(directedSweep(*negativeMinorArc) < 0.0);
    CHECK(std::abs(directedSweep(*negativeMinorArc)) < std::acos(-1.0));
    CHECK(std::holds_alternative<RS_LineData>(majorMinorBulgeData.segments[2]));
    CHECK(std::holds_alternative<RS_LineData>(majorMinorBulgeData.segments[3]));
    RS_Polyline reversedMajorMinorBulgeEntity;
    reversedMajorMinorBulgeEntity.addVertex({0, 0});
    reversedMajorMinorBulgeEntity.addVertex({0, 10});
    reversedMajorMinorBulgeEntity.addVertex({10, 10}, 0.2);
    reversedMajorMinorBulgeEntity.addVertex({10, 0}, -2.0);
    reversedMajorMinorBulgeEntity.setClosed(true);
    reversedMajorMinorBulgeEntity.endPolyline();
    const auto reversedMajorMinorBulgeSource =
        LC_OffsetSourceGeometry::capture(reversedMajorMinorBulgeEntity);
    REQUIRE(reversedMajorMinorBulgeSource);
    REQUIRE(reversedMajorMinorBulgeSource->kind() == LC_OffsetSourceGeometry::Kind::Polyline);
    const auto& reversedMajorMinorBulgeData =
        std::get<LC_OffsetPolylineData>(reversedMajorMinorBulgeSource->value);
    REQUIRE(reversedMajorMinorBulgeData.closed);
    REQUIRE(reversedMajorMinorBulgeData.segments.size() == 4);
    CHECK(std::holds_alternative<RS_LineData>(reversedMajorMinorBulgeData.segments[0]));
    CHECK(std::holds_alternative<RS_LineData>(reversedMajorMinorBulgeData.segments[1]));
    const auto* reversedPositiveMinorArc =
        std::get_if<RS_ArcData>(&reversedMajorMinorBulgeData.segments[2]);
    const auto* reversedNegativeMajorArc =
        std::get_if<RS_ArcData>(&reversedMajorMinorBulgeData.segments[3]);
    REQUIRE(reversedPositiveMinorArc);
    REQUIRE(reversedNegativeMajorArc);
    CHECK(directedSweep(*reversedPositiveMinorArc) > 0.0);
    CHECK(std::abs(directedSweep(*reversedPositiveMinorArc)) < std::acos(-1.0));
    CHECK(directedSweep(*reversedNegativeMajorArc) < -std::acos(-1.0));
    RS_Polyline rejectedClosedOppositeMajorBulges;
    rejectedClosedOppositeMajorBulges.addVertex({0, 0}, 2.0);
    rejectedClosedOppositeMajorBulges.addVertex({10, 0});
    rejectedClosedOppositeMajorBulges.addVertex({10, 100}, -2.0);
    rejectedClosedOppositeMajorBulges.addVertex({0, 100});
    rejectedClosedOppositeMajorBulges.setClosed(true);
    rejectedClosedOppositeMajorBulges.endPolyline();
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(rejectedClosedOppositeMajorBulges));
    RS_Polyline oppositeMajorBulgesEntity;
    oppositeMajorBulgesEntity.addVertex({0, 0}, 2.0);
    oppositeMajorBulgesEntity.addVertex({10, 0});
    oppositeMajorBulgesEntity.addVertex({100, 100}, -2.0);
    oppositeMajorBulgesEntity.addVertex({110, 100});
    oppositeMajorBulgesEntity.addVertex({110, -100});
    oppositeMajorBulgesEntity.endPolyline();
    const auto oppositeMajorBulgesSource =
        LC_OffsetSourceGeometry::capture(oppositeMajorBulgesEntity);
    REQUIRE(oppositeMajorBulgesSource);
    REQUIRE(oppositeMajorBulgesSource->kind() == LC_OffsetSourceGeometry::Kind::Polyline);
    const auto& oppositeMajorBulgesData =
        std::get<LC_OffsetPolylineData>(oppositeMajorBulgesSource->value);
    CHECK_FALSE(oppositeMajorBulgesData.closed);
    REQUIRE(oppositeMajorBulgesData.segments.size() == 4);
    const auto* firstPositiveMajor =
        std::get_if<RS_ArcData>(&oppositeMajorBulgesData.segments[0]);
    const auto* secondNegativeMajor =
        std::get_if<RS_ArcData>(&oppositeMajorBulgesData.segments[2]);
    REQUIRE(firstPositiveMajor);
    REQUIRE(secondNegativeMajor);
    CHECK(directedSweep(*firstPositiveMajor) > std::acos(-1.0));
    CHECK(directedSweep(*secondNegativeMajor) < -std::acos(-1.0));
    CHECK(firstPositiveMajor->center.distanceTo({5, -3.75}) < 1e-12);
    CHECK(secondNegativeMajor->center.distanceTo({105, 103.75}) < 1e-12);
    CHECK(firstPositiveMajor->radius == Catch::Approx(6.25));
    CHECK(secondNegativeMajor->radius == Catch::Approx(6.25));
    CHECK(std::holds_alternative<RS_LineData>(oppositeMajorBulgesData.segments[1]));
    CHECK(std::holds_alternative<RS_LineData>(oppositeMajorBulgesData.segments[3]));
    RS_Polyline unequalMajorBulgesEntity;
    unequalMajorBulgesEntity.addVertex({0, 0}, 3.0);
    unequalMajorBulgesEntity.addVertex({10, 0});
    unequalMajorBulgesEntity.addVertex({100, 100}, -2.0);
    unequalMajorBulgesEntity.addVertex({110, 100});
    unequalMajorBulgesEntity.addVertex({110, -100});
    unequalMajorBulgesEntity.endPolyline();
    const auto unequalMajorBulgesSource =
        LC_OffsetSourceGeometry::capture(unequalMajorBulgesEntity);
    REQUIRE(unequalMajorBulgesSource);
    const auto& unequalMajorBulgesData =
        std::get<LC_OffsetPolylineData>(unequalMajorBulgesSource->value);
    CHECK_FALSE(unequalMajorBulgesData.closed);
    REQUIRE(unequalMajorBulgesData.segments.size() == 4);
    const auto* positiveLargerMajor =
        std::get_if<RS_ArcData>(&unequalMajorBulgesData.segments[0]);
    const auto* negativeMajor =
        std::get_if<RS_ArcData>(&unequalMajorBulgesData.segments[2]);
    REQUIRE(positiveLargerMajor);
    REQUIRE(negativeMajor);
    CHECK(directedSweep(*positiveLargerMajor) > std::acos(-1.0));
    CHECK(directedSweep(*negativeMajor) < -std::acos(-1.0));
    CHECK(positiveLargerMajor->center.x == Catch::Approx(5.0));
    CHECK(positiveLargerMajor->center.y == Catch::Approx(-80.0 / 12.0));
    CHECK(positiveLargerMajor->radius == Catch::Approx(100.0 / 12.0));
    RS_Polyline reversedUnequalMajorBulgesEntity;
    reversedUnequalMajorBulgesEntity.addVertex({110, -100});
    reversedUnequalMajorBulgesEntity.addVertex({110, 100}, 2.0);
    reversedUnequalMajorBulgesEntity.addVertex({100, 100});
    reversedUnequalMajorBulgesEntity.addVertex({10, 0}, -3.0);
    reversedUnequalMajorBulgesEntity.addVertex({0, 0});
    reversedUnequalMajorBulgesEntity.endPolyline();
    const auto reversedUnequalMajorBulgesSource =
        LC_OffsetSourceGeometry::capture(reversedUnequalMajorBulgesEntity);
    REQUIRE(reversedUnequalMajorBulgesSource);
    const auto& reversedUnequalMajorBulgesData =
        std::get<LC_OffsetPolylineData>(reversedUnequalMajorBulgesSource->value);
    CHECK_FALSE(reversedUnequalMajorBulgesData.closed);
    REQUIRE(reversedUnequalMajorBulgesData.segments.size() == 4);
    CHECK(std::holds_alternative<RS_LineData>(reversedUnequalMajorBulgesData.segments[0]));
    const auto* reversedPositiveMajor =
        std::get_if<RS_ArcData>(&reversedUnequalMajorBulgesData.segments[1]);
    CHECK(std::holds_alternative<RS_LineData>(reversedUnequalMajorBulgesData.segments[2]));
    const auto* reversedNegativeLargerMajor =
        std::get_if<RS_ArcData>(&reversedUnequalMajorBulgesData.segments[3]);
    REQUIRE(reversedPositiveMajor);
    REQUIRE(reversedNegativeLargerMajor);
    CHECK(directedSweep(*reversedPositiveMajor) > std::acos(-1.0));
    CHECK(directedSweep(*reversedNegativeLargerMajor) < -std::acos(-1.0));
    CHECK(reversedPositiveMajor->center.x == Catch::Approx(105.0));
    CHECK(reversedPositiveMajor->center.y == Catch::Approx(103.75));
    CHECK(reversedPositiveMajor->radius == Catch::Approx(6.25));
    CHECK(reversedNegativeLargerMajor->center.x == Catch::Approx(5.0));
    CHECK(reversedNegativeLargerMajor->center.y == Catch::Approx(-80.0 / 12.0));
    CHECK(reversedNegativeLargerMajor->radius == Catch::Approx(100.0 / 12.0));
    RS_Polyline reflectedUnequalMajorBulgesEntity;
    reflectedUnequalMajorBulgesEntity.addVertex({0, 0}, -3.0);
    reflectedUnequalMajorBulgesEntity.addVertex({10, 0});
    reflectedUnequalMajorBulgesEntity.addVertex({100, -100}, 2.0);
    reflectedUnequalMajorBulgesEntity.addVertex({110, -100});
    reflectedUnequalMajorBulgesEntity.addVertex({110, 100});
    reflectedUnequalMajorBulgesEntity.endPolyline();
    const auto reflectedUnequalMajorBulgesSource =
        LC_OffsetSourceGeometry::capture(reflectedUnequalMajorBulgesEntity);
    REQUIRE(reflectedUnequalMajorBulgesSource);
    const auto& reflectedUnequalMajorBulgesData =
        std::get<LC_OffsetPolylineData>(reflectedUnequalMajorBulgesSource->value);
    CHECK_FALSE(reflectedUnequalMajorBulgesData.closed);
    REQUIRE(reflectedUnequalMajorBulgesData.segments.size() == 4);
    const auto* negativeLargerMajor =
        std::get_if<RS_ArcData>(&reflectedUnequalMajorBulgesData.segments[0]);
    const auto* reflectedPositiveMajor =
        std::get_if<RS_ArcData>(&reflectedUnequalMajorBulgesData.segments[2]);
    REQUIRE(negativeLargerMajor);
    REQUIRE(reflectedPositiveMajor);
    CHECK(directedSweep(*negativeLargerMajor) < -std::acos(-1.0));
    CHECK(directedSweep(*reflectedPositiveMajor) > std::acos(-1.0));
    CHECK(negativeLargerMajor->center.x == Catch::Approx(5.0));
    CHECK(negativeLargerMajor->center.y == Catch::Approx(80.0 / 12.0));
    CHECK(negativeLargerMajor->radius == Catch::Approx(100.0 / 12.0));
    CHECK(reflectedPositiveMajor->center.x == Catch::Approx(105.0));
    CHECK(reflectedPositiveMajor->center.y == Catch::Approx(-103.75));
    CHECK(reflectedPositiveMajor->radius == Catch::Approx(6.25));
    const std::array<LC_OffsetPolylineData, 11> variants{{
        {{0, 0}, {2, 2}, false,
         {RS_ArcData{{1, 0}, 1, pi, 0, false}, RS_LineData{{2, 0}, {2, 2}}}},
        {{2, 2}, {0, 0}, false,
         {RS_LineData{{2, 2}, {2, 0}}, RS_ArcData{{1, 0}, 1, 0, pi, true}}},
        {{0, 0}, {0, 0}, true,
         {RS_LineData{{0, 0}, {2, 0}}, RS_LineData{{2, 0}, {2, 2}},
          RS_LineData{{2, 2}, {0, 2}}, RS_LineData{{0, 2}, {0, 0}}}},
        {{2, 0}, {2, 0}, true,
         {RS_ArcData{{0, 0}, 2, 0, pi / 2, false},
          RS_ArcData{{0, 0}, 2, pi / 2, pi, false},
          RS_ArcData{{0, 0}, 2, pi, 3 * pi / 2, false},
          RS_ArcData{{0, 0}, 2, 3 * pi / 2, 2 * pi, false}}},
        std::get<LC_OffsetPolylineData>(mixedBulgeSource->value),
        majorMinorBulgeData,
        reversedMajorMinorBulgeData,
        oppositeMajorBulgesData,
        unequalMajorBulgesData,
        reversedUnequalMajorBulgesData,
        reflectedUnequalMajorBulgesData}};
    struct Format { Lane lane; bool dwg; DRW::Version version; const char* suffix; };
    const std::array<Format, 5> formats{{
        {Lane::Binary14, false, DRW::AC1014, "r14.dxf"},
        {Lane::Binary2000, false, DRW::AC1015, "r2000.dxf"},
        {Lane::Eed2000, true, DRW::AC1015, "r2000.dwg"},
        {Lane::Binary18, false, DRW::AC1032, "r2018.dxf"},
        {Lane::Eed18, true, DRW::AC1032, "r2018.dwg"}}};
    constexpr std::array<double, 3> scales{1e-3, 1.0, 1e3};
    constexpr std::array<double, 2> translationRatios{1e4, 1e6};
    constexpr std::array<double, 4> normalizedDistances{-.25, -.1, .1, .25};

    LC_OffsetWorkerClient worker;
    qint64 nextRequestId = 1;
    const auto runWorker = [&](req::Request request) {
        std::optional<LC_OffsetWorkerClient::Outcome> outcome;
        REQUIRE(worker.submit(request, LC_OffsetWorkerClient::Mode::Commit,
            [&](LC_OffsetWorkerClient::Outcome completed) { outcome.emplace(std::move(completed)); }));
        QElapsedTimer elapsed;
        elapsed.start();
        while (!outcome) {
            REQUIRE(elapsed.elapsed() < LC_OffsetWorkerClient::Deadlines::commit + 5000);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QThread::msleep(1);
        }
        REQUIRE(outcome->status == LC_OffsetWorkerClient::Status::Completed);
        REQUIRE(outcome->completed);
        CHECK(outcome->completed->isJoined());
        CHECK(worker.healthyAndIdle());
        return std::move(outcome->completed);
    };
    const auto transform = [](LC_OffsetPolylineData data, const double scale,
                              const RS_Vector& translation) {
        const auto point = [&](RS_Vector& value) {
            value.x = value.x * scale + translation.x;
            value.y = value.y * scale + translation.y;
            value.z *= scale;
        };
        point(data.startpoint);
        point(data.endpoint);
        for (auto& segment : data.segments) {
            std::visit([&](auto& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, RS_LineData>) {
                    point(value.startpoint);
                    point(value.endpoint);
                } else {
                    point(value.center);
                    value.radius *= scale;
                }
            }, segment);
        }
        return data;
    };
    const auto asSpline = [](const req::Cutter& item) {
        if (const auto* spline = std::get_if<RS_SplineData>(&item)) return *spline;
        const auto& segment = std::get<req::Segment>(item);
        RS_SplineData line{1, false};
        line.controlPoints = {segment.start, segment.end};
        line.knotslist = {0, 0, 1, 1};
        line.weights = {1, 1};
        return line;
    };
    const auto sameBranchGeometry = [&](const req::Cutter& centered,
                                        const req::Cutter& translated,
                                        const RS_Vector& translation,
                                        const double tolerance) {
        if (centered.index() != translated.index()) return false;
        RS_Spline centeredSpline{nullptr, asSpline(centered)};
        RS_Spline translatedSpline{nullptr, asSpline(translated)};
        double centeredFirst = 0.0, centeredLast = 0.0;
        double translatedFirst = 0.0, translatedLast = 0.0;
        if (!centeredSpline.validate() || !translatedSpline.validate() ||
            !centeredSpline.getParameterDomain(centeredFirst, centeredLast) ||
            !translatedSpline.getParameterDomain(translatedFirst, translatedLast)) return false;
        constexpr std::size_t intervals = 1024;
        const auto sampleByLength = [&](const RS_Spline& spline, const double first,
                                        const double last)
            -> std::optional<std::array<RS_Vector, 129>> {
            std::array<double, intervals + 1> parameters{};
            std::array<double, intervals + 1> lengths{};
            std::array<RS_Vector, intervals + 1> points{};
            for (std::size_t i = 0; i <= intervals; ++i) {
                const double fraction = static_cast<double>(i) /
                    static_cast<double>(intervals);
                parameters[i] = first + (last - first) * fraction;
                const auto point = spline.getPointAt(parameters[i]);
                if (!point.valid) return std::nullopt;
                points[i] = {point.x, point.y};
                if (i > 0) lengths[i] = lengths[i - 1] + std::hypot(
                    points[i].x - points[i - 1].x, points[i].y - points[i - 1].y);
            }
            if (!(lengths.back() > 0.0) || !std::isfinite(lengths.back())) return std::nullopt;
            std::array<RS_Vector, 129> samples{};
            for (std::size_t i = 0; i < samples.size(); ++i) {
                const double target = lengths.back() * static_cast<double>(i) /
                    static_cast<double>(samples.size() - 1);
                const auto upper = std::lower_bound(lengths.begin(), lengths.end(), target);
                const std::size_t high = static_cast<std::size_t>(upper - lengths.begin());
                if (high == 0) {
                    samples[i] = points.front();
                    continue;
                }
                const std::size_t low = high - 1;
                const double span = lengths[high] - lengths[low];
                const double ratio = span > 0.0 ? (target - lengths[low]) / span : 0.0;
                const double parameter = parameters[low] +
                    (parameters[high] - parameters[low]) * ratio;
                const auto point = spline.getPointAt(parameter);
                if (!point.valid) return std::nullopt;
                samples[i] = {point.x, point.y};
            }
            return samples;
        };
        const auto centeredSamples = sampleByLength(centeredSpline, centeredFirst, centeredLast);
        const auto translatedSamples = sampleByLength(
            translatedSpline, translatedFirst, translatedLast);
        if (!centeredSamples || !translatedSamples) return false;
        const auto matchesDirection = [&](const bool reverse) {
            for (std::size_t i = 0; i < centeredSamples->size(); ++i) {
                const auto& local = (*centeredSamples)[i];
                const std::size_t other = reverse ? centeredSamples->size() - 1 - i : i;
                const auto& world = (*translatedSamples)[other];
                const double expectedX = local.x + translation.x;
                const double expectedY = local.y + translation.y;
                const double magnitude = std::max({std::abs(expectedX), std::abs(expectedY),
                    std::abs(world.x), std::abs(world.y)});
                const double ulp = std::nextafter(magnitude,
                    std::numeric_limits<double>::infinity()) - magnitude;
                if (std::hypot(world.x - expectedX, world.y - expectedY) >
                    4.0 * tolerance + 64.0 * ulp) return false;
            }
            return true;
        };
        return matchesDirection(false) || matchesDirection(true);
    };

    std::size_t cells = 0;
    std::size_t generationRefusedCells = 0;
    std::size_t cacheRefusedCells = 0;
    std::size_t admittedCells = 0;
    std::size_t importedFiles = 0;
    std::size_t promotedFiles = 0;
    std::size_t fallbackFiles = 0;
    std::size_t partialCarrierCells = 0;
    for (std::size_t variant = 0; variant < variants.size(); ++variant) {
        for (const double scale : scales) {
            for (const double ratio : translationRatios) {
                const RS_Vector translation{ratio * scale, -ratio * scale};
                const LC_OffsetSourceGeometry centeredSource{
                    transform(variants[variant], scale, {})};
                const LC_OffsetSourceGeometry translatedSource{
                    transform(variants[variant], scale, translation)};
                REQUIRE(centeredSource.isWellFormed());
                REQUIRE(translatedSource.isWellFormed());
                for (const double normalizedDistance : normalizedDistances) {
                    ++cells;
                    CAPTURE(variant, scale, ratio, normalizedDistance);
                    const LC_OffsetSourceDefinition centeredDefinition{
                        centeredSource, normalizedDistance * scale, 1e-4, false};
                    const LC_OffsetSourceDefinition definition{
                        translatedSource, normalizedDistance * scale, 1e-4, false};
                    const auto centeredCompletion = runWorker({
                        {nextRequestId++, 1, 1, 1},
                        req::GenerateSource{centeredDefinition, {}}});
                    const auto& centeredReply = centeredCompletion->reply();
                    REQUIRE(centeredReply.failure == result::Failure::None);
                    REQUIRE(centeredReply.operation == 7);
                    REQUIRE(std::holds_alternative<result::Native>(centeredReply.result));
                    const auto& centered = std::get<result::Native>(centeredReply.result);
                    const auto centeredEntity = centeredSource.makeEntity();
                    REQUIRE(centeredEntity);
                    const auto centeredOptions = LC_OffsetWorkerTolerance::directOptions(
                        centeredDefinition, *centeredEntity);
                    REQUIRE(centeredOptions);
                    const auto sourceEntity = translatedSource.makeEntity();
                    REQUIRE(sourceEntity);
                    const auto sourceOptions = LC_OffsetWorkerTolerance::directOptions(
                        definition, *sourceEntity);
                    REQUIRE(sourceOptions);
                    const auto generatedCompletion = runWorker({
                        {nextRequestId++, 1, 1, 1}, req::GenerateSource{definition, {}}});
                    const auto& generatedReply = generatedCompletion->reply();
                    REQUIRE(generatedReply.failure == result::Failure::None);
                    REQUIRE(generatedReply.operation == 7);
                    REQUIRE(std::holds_alternative<result::Native>(generatedReply.result));
                    const auto& generated = std::get<result::Native>(generatedReply.result);
                    INFO("centered engine=" << (centered.engineStatus
                             ? static_cast<int>(*centered.engineStatus) : -1)
                         << " ordinary=" << static_cast<int>(centered.ordinaryStatus)
                         << " refusal=" << static_cast<int>(centered.refusal)
                         << " items=" << centered.ordinary.size()
                         << "; translated engine=" << (generated.engineStatus
                             ? static_cast<int>(*generated.engineStatus) : -1)
                         << " ordinary=" << static_cast<int>(generated.ordinaryStatus)
                         << " refusal=" << static_cast<int>(generated.refusal)
                         << " items=" << generated.ordinary.size());
                    CHECK(centered.engineStatus == generated.engineStatus);
                    CHECK(centered.ordinaryStatus == generated.ordinaryStatus);
                    CHECK(centered.ordinary.size() == generated.ordinary.size());
                    REQUIRE(centered.ordinary.size() == generated.ordinary.size());
                    std::vector<bool> matchedBranches(generated.ordinary.size(), false);
                    for (const auto& centeredBranch : centered.ordinary) {
                        bool matched = false;
                        for (std::size_t branch = 0; branch < generated.ordinary.size(); ++branch) {
                            if (matchedBranches[branch]) continue;
                            if (!sameBranchGeometry(centeredBranch, generated.ordinary[branch],
                                    translation, std::max(centeredOptions->tolerance.requestedGeometry,
                                        sourceOptions->tolerance.requestedGeometry))) continue;
                            matchedBranches[branch] = true;
                            matched = true;
                            break;
                        }
                        CHECK(matched);
                    }
                    if (generated.engineStatus != LC_CurveOffsetStatus::Ok ||
                        generated.ordinaryStatus != result::OrdinaryStatus::Complete ||
                        generated.ordinary.empty()) {
                        CHECK(generated.ordinary.empty());
                        CHECK_FALSE(generated.changed);
                        ++generationRefusedCells;
                        continue;
                    }

                    for (const auto& item : centered.ordinary) {
                        auto cache = asSpline(item);
                        const auto fingerprint = RS_Spline::geometryFingerprint(cache);
                        const auto validationCompletion = runWorker({
                            {nextRequestId++, 1, 1, 1},
                            req::ValidateSourceCache{cache, centeredDefinition, {}}});
                        const auto& validationReply = validationCompletion->reply();
                        REQUIRE(validationReply.failure == result::Failure::None);
                        REQUIRE(validationReply.operation == 8);
                        REQUIRE(std::holds_alternative<result::Native>(validationReply.result));
                        const auto& validation =
                            std::get<result::Native>(validationReply.result);
                        if (centered.ordinary.size() == 1) {
                            REQUIRE(validation.refusal == LC_OffsetCurveRefusal::None);
                            REQUIRE(validation.changed);
                        } else {
                            CHECK(validation.refusal ==
                                  LC_OffsetCurveRefusal::NotCacheRepresentable);
                            CHECK_FALSE(validation.changed);
                            CHECK(RS_Spline::geometryFingerprint(cache) == fingerprint);
                        }
                    }

                    bool admitted = false;
                    for (const auto& item : generated.ordinary) {
                        auto cache = asSpline(item);
                        const auto fingerprint = RS_Spline::geometryFingerprint(cache);
                        const auto validationCompletion = runWorker({
                            {nextRequestId++, 1, 1, 1},
                            req::ValidateSourceCache{cache, definition, {}}});
                        const auto& validationReply = validationCompletion->reply();
                        REQUIRE(validationReply.failure == result::Failure::None);
                        REQUIRE(validationReply.operation == 8);
                        REQUIRE(std::holds_alternative<result::Native>(validationReply.result));
                        const auto& validation = std::get<result::Native>(validationReply.result);
                        if (generated.ordinary.size() == 1) {
                            REQUIRE(validation.refusal == LC_OffsetCurveRefusal::None);
                            REQUIRE(validation.changed);
                            admitted = true;
                        } else {
                            CHECK(validation.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
                            CHECK_FALSE(validation.changed);
                            CHECK(RS_Spline::geometryFingerprint(cache) == fingerprint);
                        }
                    }
                    const bool partialCarrier = !admitted && generated.ordinary.size() > 1;
                    if (!admitted && !partialCarrier) {
                        ++cacheRefusedCells;
                        continue;
                    }
                    if (admitted) ++admittedCells;
                    else {
                        ++cacheRefusedCells;
                        ++partialCarrierCells;
                    }

                    const auto cache = asSpline(generated.ordinary.front());
                    RS_Spline expectedSpline{nullptr, cache};
                    REQUIRE(expectedSpline.validate());
                    const auto expectedSource = values::encodeSourceDefinition(definition);
                    REQUIRE(expectedSource);
                    for (const auto& format : formats) {
#ifndef DWGSUPPORT
                        if (format.dwg) continue;
#endif
                        xd::Definition metadata;
                        metadata.source = definition;
                        xd::Options encoding;
                        if (format.dwg) {
                            encoding.eed = true;
                            encoding.textEncoding = format.version == DRW::AC1015
                                ? xd::TextEncoding::Legacy : xd::TextEncoding::Utf16;
                            if (format.version == DRW::AC1015)
                                encoding.legacyEncodedBytes = [](std::string_view text)
                                    -> std::optional<std::size_t> { return text.size(); };
                        } else {
                            encoding.asciiDxf = false;
                        }
                        const auto tags = xd::forExport(
                            {std::make_shared<DRW_Variant>(1001, std::string("LibreCad"))},
                            metadata, encoding);
                        REQUIRE(tags);
                        DRW_Spline carrier = f::raw(1, *tags);
                        carrier.controllist.clear();
                        for (const auto& point : cache.controlPoints)
                            carrier.controllist.push_back(
                                std::make_shared<DRW_Coord>(point.x, point.y, 0));
                        carrier.knotslist = cache.knotslist;
                        carrier.weightlist = cache.weights;
                        carrier.degree = cache.degree;
                        carrier.flags = 8;
                        carrier.m_scenario = 1;
                        carrier.ncontrol = carrier.controllist.size();
                        carrier.nknots = carrier.knotslist.size();
                        carrier.nfit = 0;
                        const auto input = directory.file(std::to_string(variant) + "-" +
                            std::to_string(scale) + "-" + std::to_string(ratio) + "-" +
                            std::to_string(normalizedDistance) + "-" + format.suffix);
#ifdef DWGSUPPORT
                        if (format.dwg) f::dwgFile(input, carrier, format.version);
                        else f::binaryDxf(input, carrier, format.version);
#else
                        f::binaryDxf(input, carrier, format.version);
#endif
                        const auto outcome = stagedImport(input, format.lane);
                        REQUIRE(outcome.status == LC_OffsetImportSession::Status::Complete);
                        REQUIRE(outcome.graphic);
                        REQUIRE(outcome.admitted + outcome.downgraded == 1);
                        const auto* imported = f::sole(*outcome.graphic);
                        REQUIRE(imported);
                        const auto* native = dynamic_cast<const LC_OffsetCurve*>(imported);
                        CHECK((native != nullptr) == admitted);
                        CHECK((native == nullptr) == !admitted);
                        CHECK(outcome.admitted == (admitted ? 1u : 0u));
                        CHECK(outcome.downgraded == (admitted ? 0u : 1u));
                        std::optional<QCborArray> actualSource;
                        if (native) {
                            REQUIRE(native->sourceDefinition());
                            actualSource = values::encodeSourceDefinition(*native->sourceDefinition());
                            ++promotedFiles;
                        } else {
                            REQUIRE_FALSE(outcome.diagnostics.empty());
                            const auto parsed = xd::parseAndStrip(imported->getDrwExtData(), encoding);
                            REQUIRE(parsed.status == xd::Status::Schema2);
                            REQUIRE(parsed.definition);
                            REQUIRE(parsed.definition->source);
                            actualSource = values::encodeSourceDefinition(*parsed.definition->source);
                            ++fallbackFiles;
                        }
                        REQUIRE(actualSource);
                        CHECK(QCborValue(*expectedSource).toCbor() ==
                              QCborValue(*actualSource).toCbor());
                        const auto* importedSpline = dynamic_cast<const RS_Spline*>(imported);
                        REQUIRE(importedSpline);
                        CHECK(RS_Spline::geometryFingerprint(importedSpline->getData()) ==
                              RS_Spline::geometryFingerprint(cache));
                        double first = 0.0, last = 0.0;
                        double importedFirst = 0.0, importedLast = 0.0;
                        REQUIRE(expectedSpline.getParameterDomain(first, last));
                        REQUIRE(importedSpline->getParameterDomain(importedFirst, importedLast));
                        for (int sample = 0; sample <= 128; ++sample) {
                            const double fraction = static_cast<double>(sample) / 128.0;
                            const auto expected = expectedSpline.getPointAt(first + (last - first) * fraction);
                            const auto actual = importedSpline->getPointAt(
                                importedFirst + (importedLast - importedFirst) * fraction);
                            REQUIRE(expected.valid);
                            REQUIRE(actual.valid);
                            const double coordinateMagnitude = std::max(
                                std::abs(expected.x), std::abs(expected.y));
                            const double coordinateUlp = std::nextafter(
                                coordinateMagnitude, std::numeric_limits<double>::infinity()) -
                                coordinateMagnitude;
                            CHECK(std::hypot(actual.x - expected.x, actual.y - expected.y) <=
                                  4.0 * sourceOptions->tolerance.requestedGeometry +
                                  64.0 * coordinateUlp);
                        }
                        ++importedFiles;
                    }
                }
            }
        }
    }

    CHECK(cells == variants.size() * scales.size() * translationRatios.size() *
          normalizedDistances.size());
    CHECK(generationRefusedCells + cacheRefusedCells + admittedCells == cells);
#ifndef DWGSUPPORT
    CHECK(importedFiles == (admittedCells + partialCarrierCells) * 3);
    CHECK(promotedFiles == admittedCells * 3);
    CHECK(fallbackFiles == partialCarrierCells * 3);
#else
    CHECK(importedFiles == (admittedCells + partialCarrierCells) * formats.size());
    CHECK(promotedFiles == admittedCells * formats.size());
    CHECK(fallbackFiles == partialCarrierCells * formats.size());
#endif
    CHECK(promotedFiles + fallbackFiles == importedFiles);
}
