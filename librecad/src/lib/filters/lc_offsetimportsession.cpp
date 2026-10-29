/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include "lc_offsetimportsession.h"

#include "rs_filterdxfrw.h"
#include "rs_fileio.h"
#include "rs_block.h"
#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QThread>
#include <algorithm>
#include <istream>
#include <limits>
#include <set>
#include <streambuf>
#include <stdexcept>
#include <utility>
#include <vector>
#ifdef Q_OS_UNIX
#include <sys/stat.h>
#endif

bool LC_OffsetImportSourceStamp::NativeState::operator==(const NativeState& other) const noexcept {
    return valid && other.valid && device == other.device && inode == other.inode && size == other.size &&
        modifiedSeconds == other.modifiedSeconds && modifiedNanos == other.modifiedNanos &&
        changedSeconds == other.changedSeconds && changedNanos == other.changedNanos;
}

#ifdef Q_OS_UNIX
namespace {
LC_OffsetImportSourceStamp::NativeState nativeState(const struct stat& info) noexcept {
    LC_OffsetImportSourceStamp::NativeState state;
    if (!S_ISREG(info.st_mode) || info.st_size < 0) return state;
    state.valid = true; state.device = quint64(info.st_dev); state.inode = quint64(info.st_ino);
    state.size = qint64(info.st_size);
#if defined(Q_OS_DARWIN)
    state.modifiedSeconds = info.st_mtimespec.tv_sec; state.modifiedNanos = info.st_mtimespec.tv_nsec;
    state.changedSeconds = info.st_ctimespec.tv_sec; state.changedNanos = info.st_ctimespec.tv_nsec;
#elif defined(Q_OS_LINUX)
    state.modifiedSeconds = info.st_mtim.tv_sec; state.modifiedNanos = info.st_mtim.tv_nsec;
    state.changedSeconds = info.st_ctim.tv_sec; state.changedNanos = info.st_ctim.tv_nsec;
#else
    state.modifiedSeconds = info.st_mtime; state.changedSeconds = info.st_ctime;
#endif
    return state;
}
}
#endif

LC_OffsetImportSourceStamp LC_OffsetImportSourceStamp::capture(const QString& file) {
    QFileInfo info(file);
    LC_OffsetImportSourceStamp stamp;
    if (!info.exists() || !info.isFile()) return stamp;
    stamp.canonicalPath = info.canonicalFilePath();
    stamp.size = info.size();
    stamp.modified = info.lastModified();
    stamp.metadataChanged = info.metadataChangeTime();
    stamp.birth = info.birthTime();
#ifdef Q_OS_UNIX
    struct stat native{};
    const auto encoded = QFile::encodeName(file);
    if (::stat(encoded.constData(), &native) == 0) stamp.native = nativeState(native);
#endif
    stamp.valid = !stamp.canonicalPath.isEmpty() && stamp.size >= 0 && stamp.modified.isValid();
    return stamp;
}

bool LC_OffsetImportSourceStamp::matches(const QString& file) const {
    const auto current = capture(file);
    return valid && current.valid && canonicalPath == current.canonicalPath && size == current.size &&
        modified == current.modified && (!metadataChanged.isValid() || metadataChanged == current.metadataChanged) &&
        (!birth.isValid() || birth == current.birth) && (!native.valid || native == current.native);
}

bool LC_OffsetImportSourceStamp::matchesDescriptor(int descriptor) const noexcept {
#ifdef Q_OS_UNIX
    struct stat info{};
    return descriptor >= 0 && ::fstat(descriptor, &info) == 0 && native == nativeState(info);
#else
    Q_UNUSED(descriptor);
    return false;
#endif
}

void LC_OffsetImportBudget::report(LC_OffsetImportDiagnostic::Kind kind, int reason, std::size_t item, int detail) {
    if (diagnostics.size() == 32) { ++suppressedDiagnostics; return; }
    diagnostics.push_back({kind, reason, detail, item});
}

bool LC_OffsetImportBudget::admit(const RS_SplineData& cache, const LC_OffsetCurveXData::Definition& definition,
                                  std::size_t metadataItems) {
    std::size_t size = sizeof(LC_OffsetImportPlaceholder::Carrier);
    const auto add = [&](std::size_t count, std::size_t width) {
        if (count > (MaximumBytes - size) / width) return false;
        size += count * width; return true;
    };
    if (carriers >= MaximumCarriers ||
        !add(cache.controlPoints.size(), sizeof(RS_Vector)) || !add(cache.fitPoints.size(), sizeof(RS_Vector)) ||
        !add(cache.knotslist.size(), sizeof(double)) || !add(cache.weights.size(), sizeof(double)) ||
        !add(cache.savedOpenKnots.size(), sizeof(double)) ||
        !add(metadataItems, sizeof(std::shared_ptr<DRW_Variant>)) ||
        !add(definition.tail.size(), sizeof(LC_OffsetCurveExtension))) { ++refused; report(LC_OffsetImportDiagnostic::Kind::Budget); return false; }
    for (const auto& item : definition.tail) if (const auto* text = std::get_if<QString>(&item.value))
        if (!add(std::size_t(text->size()), sizeof(QChar))) { ++refused; report(LC_OffsetImportDiagnostic::Kind::Budget); return false; }
    if (size > (MaximumBytes - bytes) / 3) { ++refused; report(LC_OffsetImportDiagnostic::Kind::Budget); return false; }
    ++carriers; bytes += size * 3; return true;
}

namespace {
struct ImportCandidate { RS_EntityContainer* parent; LC_OffsetImportPlaceholder* entity; unsigned long long id; };
}

struct LC_OffsetImportSession::Work {
    QString file, error;
    RS2::FormatType type = RS2::FormatUnknown;
    RS2::DxfTextEncoding encoding = RS2::DxfTextEncoding::Declared;
    bool partial = false, expired = false, awaitingDecision = false;
    qint64 generation = 0;
    std::size_t index = 0, admitted = 0, downgraded = 0;
    std::optional<Status> terminal;
    Callback callback;
    PartialRequest partialRequest;
    std::unique_ptr<RS_Graphic> graphic;
    std::shared_ptr<LC_OffsetImportBudget> budget;
    LC_OffsetImportSourceStamp sourceStamp;
    QFile source;
    QByteArray snapshot;
    qint64 expectedSize = -1;
    std::vector<ImportCandidate> candidates;
};

namespace {
constexpr std::size_t ImportGraphLimit = std::size_t{1} << 20;
constexpr qint64 ImportSnapshotLimit = qint64{256} * 1024 * 1024;
constexpr qint64 ImportSnapshotChunk = qint64{256} * 1024;

class SnapshotStreamBuffer final : public std::streambuf {
public:
    explicit SnapshotStreamBuffer(QByteArray& bytes) : begin(bytes.data()), size(bytes.size()) {
        setg(begin, begin, begin + size);
    }
protected:
    pos_type seekoff(off_type offset, std::ios_base::seekdir direction,
                     std::ios_base::openmode mode) override {
        if (!(mode & std::ios_base::in) || (mode & std::ios_base::out)) return pos_type(off_type(-1));
        const off_type origin = direction == std::ios_base::beg ? 0 :
            direction == std::ios_base::cur ? gptr() - eback() :
            direction == std::ios_base::end ? size : -1;
        if (origin < 0 || offset < -origin || offset > size - origin) return pos_type(off_type(-1));
        const auto position = origin + offset;
        setg(begin, begin + position, begin + size);
        return pos_type(position);
    }
    pos_type seekpos(pos_type position, std::ios_base::openmode mode) override {
        return seekoff(off_type(position), std::ios_base::beg, mode);
    }
private:
    char* begin;
    off_type size;
};

bool supportedImportType(RS2::FormatType type) {
    switch (type) {
    case RS2::FormatDXFRW: case RS2::FormatDXFRW12: case RS2::FormatDXFRW14:
    case RS2::FormatDXFRW2000: case RS2::FormatDXFRW2004: case RS2::FormatDXFRW2010:
    case RS2::FormatDXFRW2013: case RS2::FormatDXFRW2018:
#ifdef DWGSUPPORT
    case RS2::FormatDWG: case RS2::FormatDWG2004: case RS2::FormatDWG2007:
    case RS2::FormatDWG2010: case RS2::FormatDWG2013: case RS2::FormatDWG2018:
#endif
        return true;
    default: return false;
    }
}

bool collectCandidates(RS_Graphic& graphic, std::vector<ImportCandidate>& result) {
    std::size_t visited = 0;
    std::set<const RS_Entity*> seen;
    const auto scan = [&](RS_EntityContainer& parent) {
        if (!parent.isOwner() || parent.size() > ImportGraphLimit - visited) return false;
        visited += parent.size();
        for (auto* entity : parent) {
            if (!entity || entity->getParent() != &parent || !seen.insert(entity).second) return false;
            if (auto* marker = dynamic_cast<LC_OffsetImportPlaceholder*>(entity)) {
                if (result.size() == LC_OffsetImportBudget::MaximumCarriers) return false;
                result.push_back({&parent, marker, marker->getId()});
            }
        }
        return true;
    };
    if (!scan(graphic) || graphic.countBlocks() > ImportGraphLimit - visited) return false;
    for (unsigned i = 0; i < graphic.countBlocks(); ++i) {
        auto* block = graphic.blockAt(i);
        if (block && !scan(*block)) return false;
    }
    return true;
}

bool candidateLive(RS_Graphic& graphic, const ImportCandidate& candidate) {
    bool parentLive = candidate.parent == &graphic;
    if (!parentLive) for (unsigned i = 0; i < graphic.countBlocks(); ++i)
        if (graphic.blockAt(i) == candidate.parent) { parentLive = true; break; }
    if (!parentLive || !candidate.parent->isOwner() || candidate.parent->findEntityIndex(candidate.entity) < 0) return false;
    return candidate.entity->getParent() == candidate.parent && candidate.entity->getId() == candidate.id;
}

bool containsPlaceholder(RS_Graphic& graphic) {
    std::vector<const RS_Entity*> pending;
    for (const auto* entity : graphic) if (entity) pending.push_back(entity);
    for (unsigned i = 0; i < graphic.countBlocks(); ++i) if (const auto* block = graphic.blockAt(i)) pending.push_back(block);
    std::set<const RS_Entity*> seen;
    for (std::size_t i = 0; i < pending.size(); ++i) {
        const auto* entity = pending[i];
        if (!entity || !seen.insert(entity).second) continue;
        if (seen.size() > ImportGraphLimit || dynamic_cast<const LC_OffsetImportPlaceholder*>(entity)) return true;
        const auto* container = dynamic_cast<const RS_EntityContainer*>(entity);
        if (container && container->isOwner()) {
            if (container->size() > ImportGraphLimit - pending.size()) return true;
            for (const auto* child : *container) if (child) pending.push_back(child);
        }
    }
    return false;
}
}

LC_OffsetImportSession::LC_OffsetImportSession(QObject* parent) : QObject(parent) {
    m_worker.setParent(this); m_deadline.setParent(this); m_deadline.setSingleShot(true);
    connect(&m_deadline, &QTimer::timeout, this, [this] {
        const auto work = m_work; if (!work) return;
        try { work->budget->report(LC_OffsetImportDiagnostic::Kind::Timeout, 0, work->index); }
        catch (...) { finish(work, Status::ResourceLimit); return; }
        work->expired = true; m_worker.cancel(); advance(work);
    });
}
LC_OffsetImportSession::~LC_OffsetImportSession() { cancel(); }
bool LC_OffsetImportSession::supportsType(RS2::FormatType type) { return supportedImportType(type); }
bool LC_OffsetImportSession::busy() const noexcept { return bool(m_work) || m_worker.busy(); }
void LC_OffsetImportSession::cancel() { m_deadline.stop(); m_work.reset(); m_worker.cancel(); }

bool LC_OffsetImportSession::start(const QString& file, RS2::FormatType type,
    RS2::DxfTextEncoding encoding, bool acceptPartial, Callback callback) {
    return start(file, type, encoding, [acceptPartial](bool, const QString&) { return acceptPartial; },
                 std::move(callback));
}

bool LC_OffsetImportSession::start(const QString& file, RS2::FormatType type,
    RS2::DxfTextEncoding encoding, PartialDecision decision, Callback callback) {
    return startWithDecision(file, type, encoding,
        [decision = std::move(decision)](bool hasEntities, const QString& error, PartialReply reply) {
            reply(decision && decision(hasEntities, error));
        }, std::move(callback));
}

bool LC_OffsetImportSession::startWithDecision(const QString& file, RS2::FormatType type,
    RS2::DxfTextEncoding encoding, PartialRequest request, Callback callback) {
    if (!QCoreApplication::instance() || QThread::currentThread() != thread() || busy() || !m_worker.healthyAndIdle() || !callback || file.isEmpty() ||
        m_generation == std::numeric_limits<qint64>::max()) return false;
    const QPointer<LC_OffsetImportSession> alive(this);
    try {
        auto work = std::make_shared<Work>();
        work->file = file; work->type = type; work->encoding = encoding; work->partialRequest = std::move(request);
        work->generation = ++m_generation; work->callback = std::move(callback);
        work->graphic = std::make_unique<RS_Graphic>(); work->graphic->initForNewDocument();
        if (!alive) return false;
        work->budget = std::make_shared<LC_OffsetImportBudget>(); m_work = work;
        QTimer::singleShot(0, this, [alive, work] { if (alive) alive->parse(work); });
        return true;
    } catch (...) { if (alive) m_work.reset(); return false; }
}

void LC_OffsetImportSession::parse(const std::shared_ptr<Work>& work) {
    if (m_work != work) return;
    const QPointer<LC_OffsetImportSession> alive(this);
    try {
        const auto type = work->type == RS2::FormatUnknown ? RS_FileIO::detectFormat(work->file, false) : work->type;
        if (!supportedImportType(type)) { work->error = tr("Unsupported offset import format"); finish(work, Status::ReadFailed); return; }
        work->type = type;
        work->sourceStamp = LC_OffsetImportSourceStamp::capture(work->file);
#ifdef Q_OS_UNIX
        if (!work->file.startsWith(QLatin1String(":/")) &&
            (!work->sourceStamp.valid || !work->sourceStamp.native.valid)) {
            work->error = tr("The source file identity could not be verified.");
            finish(work, Status::ReadFailed); return;
        }
#endif
        if (m_testBeforeSourceOpen) {
            auto hook = std::move(m_testBeforeSourceOpen); m_testBeforeSourceOpen = {}; hook();
            if (!alive || m_work != work) return;
        }
        work->source.setFileName(work->file);
        if (!work->source.open(QIODevice::ReadOnly) || work->source.isSequential()) {
            work->error = tr("The source file could not be opened for a bounded import.");
            finish(work, Status::ReadFailed); return;
        }
#ifdef Q_OS_UNIX
        if (work->sourceStamp.native.valid && !work->sourceStamp.matchesDescriptor(work->source.handle())) {
            work->error = tr("The source file changed while it was being opened; the drawing was not opened.");
            finish(work, Status::SourceChanged); return;
        }
#endif
        work->expectedSize = work->source.size();
        if (work->expectedSize < 0) {
            work->error = tr("The source file size could not be verified.");
            finish(work, Status::ReadFailed); return;
        }
        if (work->expectedSize > ImportSnapshotLimit) {
            work->error = tr("The source file exceeds the 256 MiB staged import size limit.");
            finish(work, Status::ResourceLimit); return;
        }
        QTimer::singleShot(0, this, [alive, work] { if (alive) alive->captureSource(work); });
    } catch (...) { if (alive && m_work == work) finish(work, Status::ResourceLimit); }
}

void LC_OffsetImportSession::captureSource(const std::shared_ptr<Work>& work) {
    if (m_work != work) return;
    const QPointer<LC_OffsetImportSession> alive(this);
    try {
        const QByteArray chunk = work->source.read(ImportSnapshotChunk);
        if (work->source.error() != QFileDevice::NoError || (chunk.isEmpty() && !work->source.atEnd())) {
            work->error = tr("The source file could not be read completely.");
            finish(work, Status::ReadFailed); return;
        }
        if (chunk.size() > ImportSnapshotLimit - work->snapshot.size()) {
            work->error = tr("The source file exceeds the 256 MiB staged import size limit.");
            finish(work, Status::ResourceLimit); return;
        }
        work->snapshot.append(chunk);
        if (m_testAfterSnapshotChunk) {
            auto hook = std::move(m_testAfterSnapshotChunk); m_testAfterSnapshotChunk = {}; hook();
            if (!alive || m_work != work) return;
        }
        if (!work->source.atEnd()) {
            QTimer::singleShot(0, this, [alive, work] { if (alive) alive->captureSource(work); });
            return;
        }
        if (work->snapshot.size() != work->expectedSize ||
            (work->sourceStamp.valid && !work->sourceStamp.matches(work->file))
#ifdef Q_OS_UNIX
            || (work->sourceStamp.native.valid && !work->sourceStamp.matchesDescriptor(work->source.handle()))
#endif
        ) {
            work->error = tr("The source file changed while it was being read; the drawing was not opened.");
            finish(work, Status::SourceChanged); return;
        }
        parseSnapshot(work);
    } catch (...) { if (alive && m_work == work) finish(work, Status::ResourceLimit); }
}

void LC_OffsetImportSession::parseSnapshot(const std::shared_ptr<Work>& work) {
    if (m_work != work) return;
    const QPointer<LC_OffsetImportSession> alive(this);
    try {
        RS_FilterDXFRW filter; filter.m_deferOffsetAdmission = true; filter.m_offsetImportBudget = work->budget;
        filter.setDxfTextEncoding(work->encoding);
        work->graphic->setFormatType(work->type);
        SnapshotStreamBuffer buffer(work->snapshot);
        std::istream input(&buffer);
        const bool read = filter.fileImportStream(*work->graphic, work->file, work->type, input);
        work->snapshot.clear();
        if (!alive || m_work != work) return;
        if (m_testAfterRead) {
            auto hook = std::move(m_testAfterRead); m_testAfterRead = {}; hook();
            if (!alive || m_work != work) return;
        }
        if ((work->sourceStamp.valid && !work->sourceStamp.matches(work->file))
#ifdef Q_OS_UNIX
            || (work->sourceStamp.native.valid && !work->sourceStamp.matchesDescriptor(work->source.handle()))
#endif
        ) {
            work->error = tr("The source file changed while it was being read; the drawing was not opened.");
            finish(work, Status::SourceChanged); return;
        }
        work->source.close();
        if (!read) { work->error = filter.lastError(); work->partial = true; }
        if (!read) {
            if (!work->partialRequest) { finish(work, Status::ReadFailed); return; }
            work->awaitingDecision = true;
            std::weak_ptr<Work> weak = work;
            try {
                work->partialRequest(!work->graphic->isEmpty(), work->error, [alive, weak](bool accepted) {
                    if (!alive || QThread::currentThread() != alive->thread()) return;
                    if (auto pending = weak.lock()) alive->resolvePartial(pending, accepted);
                });
            } catch (...) {
                if (alive && m_work == work && work->awaitingDecision) finish(work, Status::ResourceLimit);
            }
            return;
        }
        beginValidation(work);
    } catch (...) { if (alive && m_work == work) finish(work, Status::ResourceLimit); }
}

void LC_OffsetImportSession::resolvePartial(const std::shared_ptr<Work>& work, bool accepted) {
    if (m_work != work || !work->awaitingDecision) return;
    work->awaitingDecision = false;
    if (!accepted) { finish(work, Status::ReadFailed); return; }
    work->graphic->setFormatType(RS2::FormatUnknown);
    beginValidation(work);
}

void LC_OffsetImportSession::beginValidation(const std::shared_ptr<Work>& work) {
    if (m_work != work) return;
    const QPointer<LC_OffsetImportSession> alive(this);
    try {
        if (!collectCandidates(*work->graphic, work->candidates)) { finish(work, Status::ResourceLimit); return; }
        work->downgraded = work->budget->refused;
        m_deadline.start(int(LC_OffsetWorkerClient::Deadlines::commit));
        advance(work);
    } catch (...) { if (alive && m_work == work) finish(work, Status::ResourceLimit); }
}

void LC_OffsetImportSession::advance(const std::shared_ptr<Work>& work) {
    if (m_work != work) return;
    const QPointer<LC_OffsetImportSession> alive(this);
    try {
        if (work->terminal) { finish(work, *work->terminal); return; }
        while (work->index < work->candidates.size() && work->expired) {
            consume(work, {}); if (!alive || m_work != work) return;
        }
        if (work->index == work->candidates.size()) {
            if (m_worker.busy()) {
                QTimer::singleShot(10, this, [alive, work] { if (alive) alive->advance(work); });
            } else finish(work, m_worker.healthyAndIdle() ? Status::Complete : Status::ResourceLimit);
            return;
        }
        auto& candidate = work->candidates[work->index];
        if (!candidateLive(*work->graphic, candidate) || m_request == std::numeric_limits<qint64>::max()) {
            finish(work, Status::ResourceLimit); return;
        }
        const auto& carrier = candidate.entity->carrier();
        LC_OffsetWorkerRequest::Request request{{++m_request, work->generation, 0, 0},
            LC_OffsetWorkerRequest::ValidateCache{carrier.cache, carrier.definition.geometry, carrier.tolerance, {}}};
        if (!m_worker.submit(request, LC_OffsetWorkerClient::Mode::Commit,
            [alive, work](LC_OffsetWorkerClient::Outcome result) {
                if (!alive || alive->m_work != work || work->expired) return;
                try {
                    if (result.status != LC_OffsetWorkerClient::Status::Completed || !result.reaped || !result.exactGeometryWorkKnown)
                        work->budget->report(LC_OffsetImportDiagnostic::Kind::Worker, int(result.status), work->index);
                    auto completion = result.status == LC_OffsetWorkerClient::Status::Completed && result.reaped && result.exactGeometryWorkKnown
                        ? std::move(result.completed) : nullptr;
                    alive->consume(work, std::move(completion));
                    if (alive && alive->m_work == work) QTimer::singleShot(0, alive.data(), [alive, work] { if (alive) alive->advance(work); });
                } catch (...) { if (alive && alive->m_work == work) alive->finish(work, Status::ResourceLimit); }
            })) {
            work->budget->report(LC_OffsetImportDiagnostic::Kind::Worker, -1, work->index);
            consume(work, {});
            if (alive && m_work == work) QTimer::singleShot(0, this, [alive, work] { if (alive) alive->advance(work); });
        }
    } catch (...) { if (alive && m_work == work) finish(work, Status::ResourceLimit); }
}

void LC_OffsetImportSession::consume(const std::shared_ptr<Work>& work, std::unique_ptr<LC_OffsetWorkerCompletion> completion) {
    auto& candidate = work->candidates.at(work->index);
    if (!candidateLive(*work->graphic, candidate)) throw std::runtime_error("Import carrier changed");
    std::unique_ptr<RS_Entity> replacement;
    if (completion) {
        auto native = completion->native(*candidate.entity);
        if (native && native->setOwnedExtensions(candidate.entity->carrier().definition.reservedFlags,
                                                 candidate.entity->carrier().definition.tail)) {
            replacement = std::move(native); ++work->admitted;
        }
        if (!replacement) {
            const auto* result = std::get_if<LC_OffsetWorkerResult::Native>(&completion->reply().result);
            work->budget->report(LC_OffsetImportDiagnostic::Kind::Validation,
                result ? (result->refusal == LC_OffsetCurveRefusal::None ? -2 : int(result->refusal)) : -1,
                work->index, result && result->checkReason ? int(*result->checkReason) : -1);
        }
    }
    if (!replacement) { replacement = std::make_unique<RS_Spline>(*candidate.entity); ++work->downgraded; }
    auto* parent = candidate.parent; const int index = parent->findEntityIndex(candidate.entity);
    replacement->setParent(parent); parent->insertEntity(index, replacement.get()); replacement.release();
    if (!parent->removeEntity(candidate.entity)) throw std::runtime_error("Import carrier replacement failed");
    ++work->index;
}

void LC_OffsetImportSession::finish(const std::shared_ptr<Work>& work, Status status) {
    if (m_work != work) return;
    const QPointer<LC_OffsetImportSession> alive(this);
    m_deadline.stop();
    if (m_worker.busy()) {
        work->terminal = status; m_worker.cancel();
        QTimer::singleShot(10, this, [alive, work] { if (alive) alive->advance(work); });
        return;
    }
    if (!m_worker.healthyAndIdle() && status == Status::Complete) status = Status::ResourceLimit;
    if (status == Status::Complete) {
        try {
            RS_FilterDXFRW::refreshDeferredOffsetInserts(*work->graphic);
            if (!alive || m_work != work) return;
            if (containsPlaceholder(*work->graphic)) status = Status::ResourceLimit;
        } catch (...) { status = Status::ResourceLimit; }
    }
    if (!alive || m_work != work) return;
    if (status == Status::ResourceLimit && work->error.isEmpty())
        work->error = tr("Offset import could not be completed within its resource or worker limits");
    Outcome result; result.status = status; result.error = work->error; result.partial = work->partial;
    result.admitted = work->admitted; result.downgraded = work->downgraded;
    result.suppressedDiagnostics = work->budget->suppressedDiagnostics;
    result.diagnostics = std::move(work->budget->diagnostics);
    result.sourceStamp = work->sourceStamp;
    result.sourceUnverified = status == Status::Complete && !work->sourceStamp.matches(work->file);
    if (status == Status::Complete) result.graphic = std::move(work->graphic);
    auto callback = std::move(work->callback); m_work.reset();
    try { callback(std::move(result)); } catch (...) {}
}
