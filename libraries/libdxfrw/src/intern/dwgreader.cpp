/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include "dwgreader.h"
#include "drw_dbg.h"
#include "drw_reserve.h"
#include "drw_textcodec.h"
#include "dwgobjectframe.h"
#include "dwgsafety.h"
#include "proxygraphicdecoder.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace {
bool isSpaceBlockRecordName(const std::string &name);

bool requiresAggregateDelivery(std::int16_t type) {
  switch (type) {
  case dwgType::ATTRIB:
  case dwgType::SEQEND:
  case dwgType::INSERT:
  case dwgType::MINSERT:
  case dwgType::VERTEX_2D:
  case dwgType::VERTEX_3D:
  case dwgType::VERTEX_MESH:
  case dwgType::VERTEX_PFACE:
  case dwgType::VERTEX_PFACE_FACE:
  case dwgType::POLYLINE_2D:
  case dwgType::POLYLINE_3D:
  case dwgType::POLYLINE_PFACE:
  case dwgType::POLYLINE_MESH:
    return true;
  default:
    return false;
  }
}
} // namespace

dwgReader::DwgEntityOutput::~DwgEntityOutput() = default;

dwgReader::DwgEntityOutput::SourceScope::SourceScope(
    DwgEntityOutput &output, const DwgSourceFrameId &source) noexcept
    : m_output(output), m_previous(output.m_activeSource) {
  m_output.m_activeSource = source;
}

dwgReader::DwgEntityOutput::SourceScope::~SourceScope() {
  m_output.m_activeSource = std::move(m_previous);
}

dwgReader::DwgEntityOutput::SourceScope dwgReader::DwgEntityOutput::bindSource(
    const DwgSourceFrameId &source) noexcept {
  return SourceScope(*this, source);
}

template <typename T, typename Callback>
void dwgReader::DwgEntityOutput::appendValue(const T &value,
                                             Callback callback) {
  if constexpr (std::is_invocable_v<Callback, DRW_Interface &, const T &>) {
    class ReferenceEvent final : public Event {
    public:
      ReferenceEvent(const T &eventValue, Callback eventCallback,
                     const std::optional<DwgSourceFrameId> &source)
          : m_value(eventValue), m_callback(eventCallback), m_source(source) {}

      bool replay(dwgReader &, DRW_Interface &target) const override {
        std::invoke(m_callback, target, m_value);
        return true;
      }

      bool hasSource() const noexcept override { return m_source.has_value(); }

      DwgSourceFrameId source() const noexcept override {
        return m_source.value_or(DwgSourceFrameId{});
      }

      bool completesSource() const noexcept override { return false; }

    private:
      T m_value;
      Callback m_callback;
      std::optional<DwgSourceFrameId> m_source;
    };
    append(std::make_unique<ReferenceEvent>(value, callback, m_activeSource));
  } else {
    static_assert(std::is_invocable_v<Callback, DRW_Interface &, const T *>,
                  "DWG entity output callback must accept a copied value");
    class PointerEvent final : public Event {
    public:
      PointerEvent(const T &eventValue, Callback eventCallback,
                   const std::optional<DwgSourceFrameId> &source)
          : m_value(eventValue), m_callback(eventCallback), m_source(source) {}

      bool replay(dwgReader &, DRW_Interface &target) const override {
        std::invoke(m_callback, target, &m_value);
        return true;
      }

      bool hasSource() const noexcept override { return m_source.has_value(); }

      DwgSourceFrameId source() const noexcept override {
        return m_source.value_or(DwgSourceFrameId{});
      }

      bool completesSource() const noexcept override { return false; }

    private:
      T m_value;
      Callback m_callback;
      std::optional<DwgSourceFrameId> m_source;
    };
    append(std::make_unique<PointerEvent>(value, callback, m_activeSource));
  }
}

bool dwgReader::DwgEntityOutput::appendEndBlock() {
  class EndBlockEvent final : public Event {
  public:
    explicit EndBlockEvent(const std::optional<DwgSourceFrameId> &source)
        : m_source(source) {}

    bool replay(dwgReader &, DRW_Interface &target) const override {
      target.endBlock();
      return true;
    }

    bool hasSource() const noexcept override { return m_source.has_value(); }

    DwgSourceFrameId source() const noexcept override {
      return m_source.value_or(DwgSourceFrameId{});
    }

    bool completesSource() const noexcept override { return false; }

  private:
    std::optional<DwgSourceFrameId> m_source;
  };
  return append(std::make_unique<EndBlockEvent>(m_activeSource));
}

bool dwgReader::DwgEntityOutput::appendFramePublication(
    dwgReader &reader, const DRW_DwgFramePublication &publication,
    DwgFramePublicationArtifacts artifacts) {
  class FramePublicationEvent final : public Event {
  public:
    explicit FramePublicationEvent(
        const DRW_DwgFramePublication &eventPublication,
        DwgFramePublicationArtifacts eventArtifacts)
        : m_publication(eventPublication) {
      if (eventArtifacts.dictionaryMembership != nullptr)
        m_dictionaryMembership.emplace(*eventArtifacts.dictionaryMembership);
      if (eventArtifacts.typedReference != nullptr)
        m_typedReference.emplace(*eventArtifacts.typedReference);
      if (eventArtifacts.blockReachability != nullptr)
        m_blockReachability.emplace(*eventArtifacts.blockReachability);
      if (eventArtifacts.groupMembership != nullptr)
        m_groupMembership.emplace(*eventArtifacts.groupMembership);
      if (eventArtifacts.sortEntsMembership != nullptr)
        m_sortEntsMembership.emplace(*eventArtifacts.sortEntsMembership);
      if (eventArtifacts.fieldListMembership != nullptr)
        m_fieldListMembership.emplace(*eventArtifacts.fieldListMembership);
      if (eventArtifacts.dictionaryWithDefaultMembership != nullptr)
        m_dictionaryWithDefaultMembership.emplace(
            *eventArtifacts.dictionaryWithDefaultMembership);
    }

    bool replay(dwgReader &eventReader, DRW_Interface &target) const override {
      return eventReader.publishDwgFramePublication(
          target, m_publication,
          {m_dictionaryMembership.has_value() ? &*m_dictionaryMembership
                                              : nullptr,
           m_typedReference.has_value() ? &*m_typedReference : nullptr,
           m_blockReachability.has_value() ? &*m_blockReachability : nullptr,
           m_groupMembership.has_value() ? &*m_groupMembership : nullptr,
           m_sortEntsMembership.has_value() ? &*m_sortEntsMembership : nullptr,
           m_fieldListMembership.has_value() ? &*m_fieldListMembership
                                             : nullptr,
           m_dictionaryWithDefaultMembership.has_value()
               ? &*m_dictionaryWithDefaultMembership
               : nullptr});
    }

    bool hasSource() const noexcept override { return true; }

    DwgSourceFrameId source() const noexcept override {
      return DwgSourceFrameId{
          m_publication.m_handle, m_publication.m_sourceOffset,
          m_publication.m_sourceMapOrdinal, m_publication.m_sourceOffsetSpace};
    }

    bool completesSource() const noexcept override { return true; }

  private:
    DRW_DwgFramePublication m_publication;
    std::optional<DRW_DwgDictionaryMembership> m_dictionaryMembership;
    std::optional<DRW_DwgTypedReference> m_typedReference;
    std::optional<DRW_DwgBlockReachability> m_blockReachability;
    std::optional<DRW_DwgGroupMembership> m_groupMembership;
    std::optional<DRW_DwgSortEntsMembership> m_sortEntsMembership;
    std::optional<DRW_DwgFieldListMembership> m_fieldListMembership;
    std::optional<DRW_DwgDictionaryWithDefaultMembership>
        m_dictionaryWithDefaultMembership;
  };
  (void)reader;
  return append(
      std::make_unique<FramePublicationEvent>(publication, artifacts));
}

bool dwgReader::DwgEntityOutput::appendFrameCompletion() {
  class FrameCompletionEvent final : public Event {
  public:
    explicit FrameCompletionEvent(const std::optional<DwgSourceFrameId> &source)
        : m_source(source) {}

    bool replay(dwgReader &, DRW_Interface &) const override { return true; }

    bool hasSource() const noexcept override { return m_source.has_value(); }

    DwgSourceFrameId source() const noexcept override {
      return m_source.value_or(DwgSourceFrameId{});
    }

    bool completesSource() const noexcept override { return true; }

  private:
    std::optional<DwgSourceFrameId> m_source;
  };
  return append(std::make_unique<FrameCompletionEvent>(m_activeSource));
}

class dwgReader::DwgImmediateEntityOutput final : public DwgEntityOutput {
public:
  DwgImmediateEntityOutput(dwgReader &reader, DRW_Interface &target)
      : m_reader(reader), m_target(target) {}

private:
  bool append(std::unique_ptr<Event> event) override {
    return event->replay(m_reader, m_target);
  }

  dwgReader &m_reader;
  DRW_Interface &m_target;
};

dwgReader::DwgBlockJournalOutput::DwgBlockJournalOutput(dwgReader &reader)
    : m_reader(reader) {}

bool dwgReader::DwgBlockJournalOutput::replay(DRW_Interface &target,
                                              DwgSourceFrameId *activeSource) {
  for (std::size_t index = 0; index < m_events.size(); ++index) {
    if (!replayEvent(index, target, activeSource, nullptr))
      return false;
  }
  return true;
}

bool dwgReader::DwgBlockJournalOutput::replayEvent(
    std::size_t index, DRW_Interface &target, DwgSourceFrameId *activeSource,
    bool *completesSource) {
  if (index >= m_events.size())
    return false;
  const Event &event = *m_events[index];
  if (activeSource != nullptr && event.hasSource())
    *activeSource = event.source();
  if (completesSource != nullptr)
    *completesSource = event.completesSource();
  return event.replay(m_reader, target);
}

bool dwgReader::DwgBlockJournalOutput::reserve(std::size_t eventCount) {
  if (eventCount >
      static_cast<std::size_t>(dwgSafety::MaxBlockJournalEventCount) -
          m_events.size()) {
    return false;
  }
  try {
    m_events.reserve(m_events.size() + eventCount);
  } catch (...) {
    return false;
  }
  return true;
}

void dwgReader::DwgBlockJournalOutput::truncate(
    std::size_t eventCount) noexcept {
  if (eventCount <= m_events.size())
    m_events.resize(eventCount);
}

bool dwgReader::DwgBlockJournalOutput::empty() const noexcept {
  return m_events.empty();
}

std::size_t dwgReader::DwgBlockJournalOutput::size() const noexcept {
  return m_events.size();
}

bool dwgReader::DwgBlockJournalOutput::append(std::unique_ptr<Event> event) {
  if (event == nullptr ||
      m_events.size() >=
          static_cast<std::size_t>(dwgSafety::MaxBlockJournalEventCount)) {
    throw std::length_error("DWG block journal event limit exceeded");
  }
  m_events.push_back(std::move(event));
  return true;
}

dwgReader::DwgBlockScopeTransaction::DwgBlockScopeTransaction(dwgReader &reader)
    : m_reader(reader), m_output(reader) {}

dwgReader::DwgBlockScopeTransaction::~DwgBlockScopeTransaction() {
  if (!m_finished)
    (void)abort();
}

dwgReader::DwgBlockJournalOutput &
dwgReader::DwgBlockScopeTransaction::output() noexcept {
  return m_output;
}

bool dwgReader::DwgBlockScopeTransaction::reserveAdmission(
    std::size_t sourceCount, std::size_t eventCount,
    std::uint64_t bodyByteCount) {
  return reserveAdmissionImpl(sourceCount, eventCount, bodyByteCount, true);
}

bool dwgReader::DwgBlockScopeTransaction::reserveAdmissionImpl(
    std::size_t sourceCount, std::size_t eventCount,
    std::uint64_t bodyByteCount, bool consumeFailurePoint) {
  const std::size_t maxSources =
      static_cast<std::size_t>(dwgSafety::MaxOwnedObjectCount) + 2u;
  if (m_finished ||
      (consumeFailurePoint &&
       m_reader.consumeBlockJournalReservationFailurePointForTest()) ||
      sourceCount > maxSources - m_leases.size() ||
      bodyByteCount > dwgSafety::MaxBufferSize - m_validatedBodyBytes ||
      !m_output.reserve(eventCount)) {
    return false;
  }
  try {
    m_leases.reserve(m_leases.size() + sourceCount);
    m_leaseIndexes.reserve(m_leaseIndexes.size() + sourceCount);
  } catch (...) {
    return false;
  }
  return true;
}

bool dwgReader::DwgBlockScopeTransaction::adopt(DwgFrameMapLease &lease) {
  if (m_finished || !lease.isDetached() ||
      lease.object.handle != lease.source.handle ||
      lease.source.handle == DRW::NoHandle ||
      m_leaseIndexes.find(lease.source.handle) != m_leaseIndexes.end()) {
    return false;
  }
  const std::uint64_t bodyByteCount = lease.classification.has_value()
                                          ? lease.classification->bodyByteSize
                                          : lease.bodyByteSize;
  if (!reserveAdmissionImpl(1u, 0u, bodyByteCount, false))
    return false;
  if (lease.hasCoverage) {
    const auto sourceIt =
        m_reader.m_dwgSourceFrameIndexes.find(lease.source.handle);
    if (sourceIt == m_reader.m_dwgSourceFrameIndexes.end() ||
        sourceIt->second >= m_reader.m_dwgSourceFrameLedger.size()) {
      return false;
    }
    const DRW_DwgFrameCoverageEntry &entry =
        m_reader.m_dwgSourceFrameLedger[sourceIt->second];
    const DwgSourceFrameId expected{entry.m_handle, entry.m_sourceOffset,
                                    entry.m_sourceMapOrdinal,
                                    entry.m_sourceOffsetSpace};
    if (!(lease.source == expected) ||
        entry.m_disposition != DRW_DwgFrameDisposition::Staged ||
        entry.m_publicationCount != 0) {
      return false;
    }
  }
  if (m_reader.consumeBlockJournalAdoptFailurePointForTest())
    return false;
  const std::uint32_t sourceHandle = lease.source.handle;
  try {
    const auto inserted = m_leaseIndexes.emplace(sourceHandle, m_leases.size());
    if (!inserted.second)
      return false;
    m_leases.emplace_back(std::move(lease));
  } catch (...) {
    m_leaseIndexes.erase(sourceHandle);
    return false;
  }
  m_validatedBodyBytes += bodyByteCount;
  return true;
}

bool dwgReader::DwgBlockScopeTransaction::replay(DRW_Interface &target) {
  if (m_finished)
    return false;

  for (std::size_t index = 0; index < m_output.size(); ++index) {
    DwgSourceFrameId activeSource;
    bool completesSource = false;
    bool replayed = false;
    try {
      replayed =
          m_output.replayEvent(index, target, &activeSource, &completesSource);
    } catch (...) {
      (void)failLease(activeSource);
      (void)abort();
      return false;
    }
    if (!replayed || activeSource.handle == DRW::NoHandle ||
        findLease(activeSource) == nullptr) {
      (void)abort();
      return false;
    }
    if (completesSource && !retireLease(activeSource)) {
      (void)abort();
      return false;
    }
  }

  for (const DwgFrameMapLease &lease : m_leases) {
    if (lease.isDetached()) {
      (void)abort();
      return false;
    }
  }
  m_finished = true;
  return true;
}

bool dwgReader::DwgBlockScopeTransaction::abort() noexcept {
  if (m_finished)
    return false;

  bool success = true;
  try {
    for (DwgFrameMapLease &lease : m_leases) {
      if (!lease.isDetached())
        continue;
      if (lease.hasCoverage) {
        const auto sourceIt =
            m_reader.m_dwgSourceFrameIndexes.find(lease.source.handle);
        if (sourceIt == m_reader.m_dwgSourceFrameIndexes.end() ||
            sourceIt->second >= m_reader.m_dwgSourceFrameLedger.size()) {
          success = false;
        } else {
          const DRW_DwgFrameDisposition disposition =
              m_reader.m_dwgSourceFrameLedger[sourceIt->second].m_disposition;
          if (disposition == DRW_DwgFrameDisposition::Pending ||
              disposition == DRW_DwgFrameDisposition::Deferred ||
              disposition == DRW_DwgFrameDisposition::Staged) {
            success = m_reader.quarantineDwgFrame(lease.source) && success;
          }
        }
      } else {
        success = m_reader.suppressDwgFrame(lease.source, false) && success;
      }
      success = m_reader.discardDetachedDwgSourceFrame(lease) && success;
    }
  } catch (...) {
    success = false;
  }
  m_finished = true;
  return success;
}

std::size_t dwgReader::DwgBlockScopeTransaction::sourceCount() const noexcept {
  return m_leases.size();
}

dwgReader::DwgFrameMapLease *dwgReader::DwgBlockScopeTransaction::findLease(
    const DwgSourceFrameId &source) noexcept {
  const auto index = m_leaseIndexes.find(source.handle);
  if (index == m_leaseIndexes.end() || index->second >= m_leases.size())
    return nullptr;
  DwgFrameMapLease &lease = m_leases[index->second];
  return lease.source == source ? &lease : nullptr;
}

bool dwgReader::DwgBlockScopeTransaction::retireLease(
    const DwgSourceFrameId &source) noexcept {
  DwgFrameMapLease *const lease = findLease(source);
  return lease != nullptr && lease->isDetached() &&
         m_reader.discardDetachedDwgSourceFrame(*lease);
}

bool dwgReader::DwgBlockScopeTransaction::releaseLast(
    const DwgSourceFrameId &expected, DwgFrameMapLease &lease) noexcept {
  if (m_finished || m_leases.empty())
    return false;
  DwgFrameMapLease &last = m_leases.back();
  if (!last.isDetached() || !(last.source == expected) ||
      last.object.handle != last.source.handle) {
    return false;
  }
  const std::uint64_t bodyByteCount = last.classification.has_value()
                                          ? last.classification->bodyByteSize
                                          : last.bodyByteSize;
  if (bodyByteCount > m_validatedBodyBytes)
    return false;
  const auto index = m_leaseIndexes.find(last.source.handle);
  if (index == m_leaseIndexes.end() || index->second + 1u != m_leases.size())
    return false;
  lease = std::move(last);
  m_leases.pop_back();
  m_leaseIndexes.erase(index);
  m_validatedBodyBytes -= bodyByteCount;
  return true;
}

bool dwgReader::DwgBlockScopeTransaction::failLease(
    const DwgSourceFrameId &source) noexcept {
  DwgFrameMapLease *const lease = findLease(source);
  if (lease == nullptr || !lease->isDetached())
    return false;
  if (!lease->hasCoverage)
    return m_reader.suppressDwgFrame(lease->source, false);
  return m_reader.markDwgFrameOutcome(
      lease->source, DRW_DwgFrameDisposition::Failed,
      DRW_DwgFrameCoverageReason::CallbackException);
}

void dwgReader::addIntegrityDiagnostic(
    DwgIntegrityDiagnostic diagnostic) noexcept {
  if (diagnostic.version == DRW::UNKNOWNV)
    diagnostic.version = version;
  if (m_integrityDiagnostics.size() >= dwgSafety::MaxIntegrityDiagnostics) {
    if (m_integrityDiagnosticsDropped !=
        std::numeric_limits<std::size_t>::max())
      ++m_integrityDiagnosticsDropped;
    return;
  }
  try {
    m_integrityDiagnostics.push_back(std::move(diagnostic));
  } catch (...) {
    // Diagnostics must never change the established read result when the
    // process cannot allocate the optional reporting record.
    if (m_integrityDiagnosticsDropped !=
        std::numeric_limits<std::size_t>::max())
      ++m_integrityDiagnosticsDropped;
  }
}

void dwgReader::recordIntegrityDiagnostic(
    DwgIntegritySeverity severity, DwgIntegrityAddressSpace offsetSpace,
    DwgIntegrityPhase phase, DwgIntegrityCheckKind kind,
    std::int32_t logicalSectionId, std::int32_t sectionDescriptorId,
    const char *sectionName, std::uint64_t pageId, bool hasPageId,
    std::uint64_t offset, bool hasOffset, std::uint32_t logicalHandle,
    bool hasHandle, std::uint64_t expected, std::uint64_t observed,
    bool hasValues) noexcept {
  try {
    DwgIntegrityDiagnostic diagnostic;
    diagnostic.severity = severity;
    diagnostic.offsetSpace = offsetSpace;
    diagnostic.phase = phase;
    diagnostic.kind = kind;
    diagnostic.logicalSectionId = logicalSectionId;
    diagnostic.sectionDescriptorId = sectionDescriptorId;
    if (sectionName != nullptr)
      diagnostic.sectionName = sectionName;
    diagnostic.pageId = pageId;
    diagnostic.hasPageId = hasPageId;
    diagnostic.fileOffset = offset;
    diagnostic.hasFileOffset = hasOffset;
    diagnostic.logicalHandle = logicalHandle;
    diagnostic.hasLogicalHandle = hasHandle;
    diagnostic.expected = expected;
    diagnostic.observed = observed;
    diagnostic.hasExpected = hasValues;
    diagnostic.hasObserved = hasValues;
    addIntegrityDiagnostic(std::move(diagnostic));
  } catch (...) {
    // Integrity reporting is optional and must never alter parsing.
  }
}

void dwgReader::recordObjectFrameFailure(
    const objHandle &object, DwgIntegrityAddressSpace offsetSpace) noexcept {
  recordIntegrityDiagnostic(
      DwgIntegritySeverity::Error, offsetSpace, DwgIntegrityPhase::ObjectFrame,
      DwgIntegrityCheckKind::ObjectFrameBounds, secEnum::OBJECTS, -1, nullptr,
      0, false, object.loc, true, object.handle,
      object.handle != DRW::NoHandle);
}

void dwgReader::recordEntityFailure(const objHandle &object, std::int16_t type,
                                    DwgEntityFailurePhase phase,
                                    std::uint32_t blockRecordHandle) noexcept {
  if (phase == DwgEntityFailurePhase::None ||
      m_entityFailureDiagnostics.size() >=
          dwgSafety::MaxEntityFailureDiagnostics) {
    return;
  }
  try {
    m_entityFailureDiagnostics.push_back({object.handle, type,
                                          blockRecordHandle != DRW::NoHandle
                                              ? blockRecordHandle
                                              : rawBlockEntityOwner,
                                          phase});
  } catch (...) {
    // Fixture attribution must never turn an existing parse failure into
    // a different observable result when reporting cannot allocate.
  }
}

bool dwgReader::classifyDwgSourceFrame(dwgBuffer *dbuf, const objHandle &object,
                                       DwgFrameClassification &classification) {
  classification = {};
  if (dbuf == nullptr)
    return false;

  DwgObjectFrame frame;
  if (!frame.readAt(*dbuf, version, object.loc))
    return false;
  if (frame.body().size() > std::numeric_limits<std::uint32_t>::max())
    return false;
  classification.bodyByteSize = static_cast<std::uint32_t>(frame.body().size());

  std::vector<std::uint8_t> &body = frame.body();
  dwgBuffer buffer(body.data(), body.size(), &decoder);
  const std::int16_t encodedType = buffer.getObjType(version);
  if (!buffer.isGood() || encodedType < 0)
    return false;

  classification.encodedType = encodedType;
  classification.resolvedType = encodedType;
  if (encodedType == dwgType::BLOCK || encodedType == dwgType::ENDBLK) {
    classification.route = DwgFrameClassification::Route::BlockDelimiter;
    return true;
  }

  classification.fixedObjectShell =
      version >= DRW::AC1021 &&
      DRW_UnsupportedObject::isFixedObjectShellType(encodedType);
  const bool fixedEntityShell =
      version >= DRW::AC1021 &&
      DRW_UnsupportedObject::isFixedEntityShellType(encodedType);
  const bool fixedObject = dwgObjType::isFixedObject(encodedType);
  if (encodedType > dwgObjType::PROXY_OBJECT && !fixedObject &&
      !classification.fixedObjectShell && !fixedEntityShell &&
      encodedType != dwgType::WIPEOUT) {
    const auto classIt = classesmap.find(encodedType);
    if (classIt != classesmap.end() && classIt->second != nullptr) {
      classification.resolvedClass = classIt->second;
      if (classIt->second->dwgType != 0) {
        classification.resolvedType =
            static_cast<std::int16_t>(classIt->second->dwgType);
      }
    }
  }

  if (classification.fixedObjectShell || fixedObject) {
    classification.route = DwgFrameClassification::Route::Object;
  } else if (classification.resolvedClass != nullptr) {
    classification.route = classification.resolvedClass->entityFlag == 0
                               ? DwgFrameClassification::Route::Object
                               : DwgFrameClassification::Route::Entity;
  } else if (encodedType > dwgObjType::PROXY_OBJECT && !fixedEntityShell &&
             encodedType != dwgType::WIPEOUT) {
    // Unregistered class ids have no on-disk entity/object discriminator.
    // The entity pass only defers ownerless instances, which are preserved
    // as opaque OBJECTS records here.
    classification.route = DwgFrameClassification::Route::Object;
  } else {
    classification.route = DwgFrameClassification::Route::Entity;
  }
  return true;
}

bool dwgReader::classificationsMatch(
    const DwgFrameClassification &expected,
    const DwgFrameClassification &observed) noexcept {
  return expected.encodedType == observed.encodedType &&
         expected.resolvedType == observed.resolvedType &&
         expected.resolvedClass == observed.resolvedClass &&
         expected.bodyByteSize == observed.bodyByteSize &&
         expected.fixedObjectShell == observed.fixedObjectShell &&
         expected.route == observed.route;
}

void dwgReader::recordDwgFramePhaseSnapshot(
    const DwgFrameMapLease &lease,
    DwgFramePhaseSnapshot::Destination destination,
    DRW_DwgFrameDisposition disposition) noexcept {
  if (!lease.hasCoverage || !lease.classification.has_value())
    return;
  try {
    m_dwgFramePhaseSnapshots.push_back({lease.source, *lease.classification,
                                        lease.origin, destination,
                                        disposition});
  } catch (...) {
    // Routing evidence is optional and must never influence parsing.
  }
}

bool dwgReader::reportDwgFrameTransitionFailure(const DwgSourceFrameId &source,
                                                std::uint64_t offset,
                                                bool hasOffset) noexcept {
  m_dwgFrameCoverageIntegrityViolation = true;
  recordIntegrityDiagnostic(
      DwgIntegritySeverity::Error, DwgIntegrityAddressSpace::None,
      DwgIntegrityPhase::ObjectMap,
      DwgIntegrityCheckKind::FrameLedgerTransition, secEnum::HANDLES, -1,
      nullptr, 0, false, offset, hasOffset, source.handle,
      source.handle != DRW::NoHandle);
  return false;
}

DwgSourceFrameId
dwgReader::sourceFrameIdForHandle(std::uint32_t handle) const noexcept {
  DwgSourceFrameId source;
  source.handle = handle;
  const auto it = m_dwgSourceFrameIndexes.find(handle);
  if (it == m_dwgSourceFrameIndexes.end() ||
      it->second >= m_dwgSourceFrameLedger.size()) {
    return source;
  }
  const DRW_DwgFrameCoverageEntry &entry = m_dwgSourceFrameLedger[it->second];
  source.offset = entry.m_sourceOffset;
  source.ordinal = entry.m_sourceMapOrdinal;
  source.offsetSpace = entry.m_sourceOffsetSpace;
  return source;
}

bool dwgReader::borrowDwgSourceFrame(DwgObjectMap &map,
                                     DwgObjectMap::iterator it,
                                     DwgSourceFrameLease &lease) {
  if (it == map.end())
    return false;

  const objHandle object = it->second;
  const DwgSourceFrameId source = sourceFrameId(object);
  if (object.handle != it->first)
    return reportDwgFrameTransitionFailure(source, object.loc, true);
  if ((&map == &ObjectMap &&
       objObjectMap.find(object.handle) != objObjectMap.end()) ||
      (&map == &objObjectMap &&
       ObjectMap.find(object.handle) != ObjectMap.end())) {
    return reportDwgFrameTransitionFailure(source, object.loc, true);
  }

  const bool hasCoverage =
      m_dwgFrameCoverageStatus != DRW_DwgFrameCoverageStatus::NotAvailable;
  if (hasCoverage) {
    const auto sourceIt = m_dwgSourceFrameIndexes.find(object.handle);
    if (sourceIt == m_dwgSourceFrameIndexes.end() ||
        sourceIt->second >= m_dwgSourceFrameLedger.size()) {
      return reportDwgFrameTransitionFailure(source, object.loc, true);
    }
    DRW_DwgFrameCoverageEntry &entry = m_dwgSourceFrameLedger[sourceIt->second];
    const DwgSourceFrameId expected{entry.m_handle, entry.m_sourceOffset,
                                    entry.m_sourceMapOrdinal,
                                    entry.m_sourceOffsetSpace};
    if (!(source == expected) ||
        (entry.m_disposition != DRW_DwgFrameDisposition::Pending &&
         entry.m_disposition != DRW_DwgFrameDisposition::Deferred) ||
        entry.m_publicationCount != 0) {
      return reportDwgFrameTransitionFailure(source, entry.m_sourceOffset,
                                             true);
    }
  }

  lease.object = object;
  lease.source = source;
  lease.hasCoverage = hasCoverage;
  return true;
}

bool dwgReader::takeDwgSourceFrame(DwgObjectMap &map, DwgObjectMap::iterator it,
                                   DwgSourceFrameLease &lease) {
  if (!borrowDwgSourceFrame(map, it, lease))
    return false;
  map.erase(it);
  return true;
}

bool dwgReader::detachDwgSourceFrame(DwgObjectMap &map,
                                     DwgObjectMap::iterator it,
                                     DwgFrameMapLease &lease) {
  if (lease.isDetached())
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);

  DwgSourceFrameLease borrowed;
  if (!borrowDwgSourceFrame(map, it, borrowed))
    return false;

  DwgFrameMapLease::Origin origin = DwgFrameMapLease::Origin::None;
  if (&map == &ObjectMap)
    origin = DwgFrameMapLease::Origin::ObjectMap;
  else if (&map == &objObjectMap)
    origin = DwgFrameMapLease::Origin::DeferredObjectMap;
  else
    return reportDwgFrameTransitionFailure(borrowed.source, borrowed.object.loc,
                                           true);

  DwgObjectMap::node_type node = map.extract(it);
  if (node.empty())
    return reportDwgFrameTransitionFailure(borrowed.source, borrowed.object.loc,
                                           true);

  lease.object = borrowed.object;
  lease.source = borrowed.source;
  lease.origin = origin;
  lease.hasCoverage = borrowed.hasCoverage;
  lease.classification.reset();
  if (origin == DwgFrameMapLease::Origin::DeferredObjectMap) {
    const auto classificationIt =
        m_deferredFrameClassifications.find(lease.object.handle);
    if (classificationIt != m_deferredFrameClassifications.end())
      lease.classification.emplace(classificationIt->second);
  }
  lease.node.emplace(std::move(node));
  return true;
}

bool dwgReader::stageDetachedDwgSourceFrame(DwgFrameMapLease &lease) {
  if (!lease.isDetached() || lease.object.handle != lease.source.handle ||
      lease.origin == DwgFrameMapLease::Origin::None) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  if (ObjectMap.find(lease.object.handle) != ObjectMap.end() ||
      objObjectMap.find(lease.object.handle) != objObjectMap.end()) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  if (!lease.hasCoverage)
    return true;

  const auto sourceIt = m_dwgSourceFrameIndexes.find(lease.source.handle);
  if (sourceIt == m_dwgSourceFrameIndexes.end() ||
      sourceIt->second >= m_dwgSourceFrameLedger.size()) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  const DRW_DwgFrameCoverageEntry &entry =
      m_dwgSourceFrameLedger[sourceIt->second];
  const DwgSourceFrameId expected{entry.m_handle, entry.m_sourceOffset,
                                  entry.m_sourceMapOrdinal,
                                  entry.m_sourceOffsetSpace};
  if (!(lease.source == expected) ||
      entry.m_disposition != DRW_DwgFrameDisposition::Pending ||
      entry.m_publicationCount != 0) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  return markDwgFrameOutcome(lease.source, DRW_DwgFrameDisposition::Staged,
                             DRW_DwgFrameCoverageReason::CompoundStaged);
}

bool dwgReader::restoreDwgSourceFrame(DwgFrameMapLease &lease) {
  if (!lease.isDetached() || lease.object.handle != lease.source.handle) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }

  DwgObjectMap *sourceMap = nullptr;
  DwgObjectMap *otherMap = nullptr;
  if (lease.origin == DwgFrameMapLease::Origin::ObjectMap) {
    sourceMap = &ObjectMap;
    otherMap = &objObjectMap;
  } else if (lease.origin == DwgFrameMapLease::Origin::DeferredObjectMap) {
    sourceMap = &objObjectMap;
    otherMap = &ObjectMap;
  } else {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }

  if (sourceMap->find(lease.object.handle) != sourceMap->end() ||
      otherMap->find(lease.object.handle) != otherMap->end()) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  DRW_DwgFrameCoverageEntry *stagedEntry = nullptr;
  if (lease.hasCoverage) {
    const auto sourceIt = m_dwgSourceFrameIndexes.find(lease.source.handle);
    if (sourceIt == m_dwgSourceFrameIndexes.end() ||
        sourceIt->second >= m_dwgSourceFrameLedger.size()) {
      return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                             true);
    }
    DRW_DwgFrameCoverageEntry &entry = m_dwgSourceFrameLedger[sourceIt->second];
    const DwgSourceFrameId expected{entry.m_handle, entry.m_sourceOffset,
                                    entry.m_sourceMapOrdinal,
                                    entry.m_sourceOffsetSpace};
    if (!(lease.source == expected) ||
        (entry.m_disposition != DRW_DwgFrameDisposition::Pending &&
         entry.m_disposition != DRW_DwgFrameDisposition::Deferred &&
         entry.m_disposition != DRW_DwgFrameDisposition::Staged) ||
        entry.m_publicationCount != 0) {
      return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                             true);
    }
    if (entry.m_disposition == DRW_DwgFrameDisposition::Staged)
      stagedEntry = &entry;
  }

  DwgObjectMap::insert_return_type inserted;
  try {
    inserted = sourceMap->insert(std::move(*lease.node));
  } catch (...) {
    return false;
  }
  if (!inserted.inserted) {
    lease.node.reset();
    lease.node.emplace(std::move(inserted.node));
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  lease.node.reset();
  if (lease.origin == DwgFrameMapLease::Origin::DeferredObjectMap)
    m_deferredFrameClassifications.erase(lease.object.handle);
  lease.origin = DwgFrameMapLease::Origin::None;
  if (stagedEntry != nullptr) {
    stagedEntry->m_disposition = DRW_DwgFrameDisposition::Pending;
    stagedEntry->m_reason = DRW_DwgFrameCoverageReason::None;
  }
  return true;
}

bool dwgReader::deferDetachedDwgSourceFrame(DwgFrameMapLease &lease,
                                            DwgObjectMap &target) {
  if (!lease.isDetached() ||
      lease.origin != DwgFrameMapLease::Origin::ObjectMap ||
      &target != &objObjectMap || lease.object.handle != lease.source.handle) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  if (ObjectMap.find(lease.object.handle) != ObjectMap.end() ||
      objObjectMap.find(lease.object.handle) != objObjectMap.end()) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  if (lease.hasCoverage) {
    const auto sourceIt = m_dwgSourceFrameIndexes.find(lease.source.handle);
    if (sourceIt == m_dwgSourceFrameIndexes.end() ||
        sourceIt->second >= m_dwgSourceFrameLedger.size()) {
      return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                             true);
    }
    const DRW_DwgFrameCoverageEntry &entry =
        m_dwgSourceFrameLedger[sourceIt->second];
    const DwgSourceFrameId expected{entry.m_handle, entry.m_sourceOffset,
                                    entry.m_sourceMapOrdinal,
                                    entry.m_sourceOffsetSpace};
    if (!(lease.source == expected) ||
        (entry.m_disposition != DRW_DwgFrameDisposition::Pending &&
         entry.m_disposition != DRW_DwgFrameDisposition::Deferred) ||
        entry.m_publicationCount != 0) {
      return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                             true);
    }
  }
  bool storedClassification = false;
  if (lease.classification.has_value()) {
    if (m_deferredFrameClassifications.find(lease.object.handle) !=
        m_deferredFrameClassifications.end()) {
      return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                             true);
    }
    try {
      const auto inserted = m_deferredFrameClassifications.emplace(
          lease.object.handle, *lease.classification);
      if (!inserted.second) {
        return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                               true);
      }
      storedClassification = true;
    } catch (...) {
      return false;
    }
  }
  try {
    target.reserve(target.size() + 1u);
  } catch (...) {
    if (storedClassification)
      m_deferredFrameClassifications.erase(lease.object.handle);
    return false;
  }

  DwgObjectMap::node_type node = std::move(*lease.node);
  lease.node.reset();
  DwgObjectMap::insert_return_type inserted;
  try {
    inserted = target.insert(std::move(node));
  } catch (...) {
    if (storedClassification)
      m_deferredFrameClassifications.erase(lease.object.handle);
    lease.node.emplace(std::move(node));
    return false;
  }
  if (!inserted.inserted) {
    if (storedClassification)
      m_deferredFrameClassifications.erase(lease.object.handle);
    lease.node.emplace(std::move(inserted.node));
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  if (!lease.hasCoverage ||
      markDwgFrameOutcome(lease.source, DRW_DwgFrameDisposition::Deferred)) {
    recordDwgFramePhaseSnapshot(
        lease, DwgFramePhaseSnapshot::Destination::DeferredObjectMap,
        DRW_DwgFrameDisposition::Deferred);
    lease.origin = DwgFrameMapLease::Origin::None;
    return true;
  }

  lease.node.emplace(target.extract(lease.object.handle));
  if (storedClassification)
    m_deferredFrameClassifications.erase(lease.object.handle);
  return false;
}

bool dwgReader::discardDetachedDwgSourceFrame(DwgFrameMapLease &lease) {
  if (!lease.isDetached() || lease.object.handle != lease.source.handle ||
      lease.origin == DwgFrameMapLease::Origin::None) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  if (ObjectMap.find(lease.object.handle) != ObjectMap.end() ||
      objObjectMap.find(lease.object.handle) != objObjectMap.end()) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  if (lease.hasCoverage) {
    const auto sourceIt = m_dwgSourceFrameIndexes.find(lease.source.handle);
    if (sourceIt == m_dwgSourceFrameIndexes.end() ||
        sourceIt->second >= m_dwgSourceFrameLedger.size()) {
      return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                             true);
    }
    const DRW_DwgFrameCoverageEntry &entry =
        m_dwgSourceFrameLedger[sourceIt->second];
    const DwgSourceFrameId expected{entry.m_handle, entry.m_sourceOffset,
                                    entry.m_sourceMapOrdinal,
                                    entry.m_sourceOffsetSpace};
    if (!(lease.source == expected) || entry.m_publicationCount > 1 ||
        (entry.m_disposition != DRW_DwgFrameDisposition::Published &&
         entry.m_disposition != DRW_DwgFrameDisposition::Failed &&
         entry.m_disposition != DRW_DwgFrameDisposition::Quarantined &&
         entry.m_disposition != DRW_DwgFrameDisposition::Unresolved)) {
      return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                             true);
    }
  }
  lease.node.reset();
  if (lease.origin == DwgFrameMapLease::Origin::DeferredObjectMap)
    m_deferredFrameClassifications.erase(lease.object.handle);
  lease.origin = DwgFrameMapLease::Origin::None;
  return true;
}

bool dwgReader::deferDwgSourceFrame(const DwgSourceFrameLease &lease,
                                    DwgObjectMap &target) {
  if (lease.object.handle != lease.source.handle)
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);

  const auto validateLease = [&]() {
    if (!lease.hasCoverage)
      return true;
    const auto sourceIt = m_dwgSourceFrameIndexes.find(lease.source.handle);
    if (sourceIt == m_dwgSourceFrameIndexes.end() ||
        sourceIt->second >= m_dwgSourceFrameLedger.size()) {
      return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                             true);
    }
    const DRW_DwgFrameCoverageEntry &entry =
        m_dwgSourceFrameLedger[sourceIt->second];
    const DwgSourceFrameId expected{entry.m_handle, entry.m_sourceOffset,
                                    entry.m_sourceMapOrdinal,
                                    entry.m_sourceOffsetSpace};
    if (!(lease.source == expected) ||
        (entry.m_disposition != DRW_DwgFrameDisposition::Pending &&
         entry.m_disposition != DRW_DwgFrameDisposition::Deferred) ||
        entry.m_publicationCount != 0) {
      return reportDwgFrameTransitionFailure(lease.source, entry.m_sourceOffset,
                                             true);
    }
    return true;
  };
  if (!validateLease())
    return false;

  const auto sourceObjectIt = ObjectMap.find(lease.object.handle);
  const auto sourceDeferredIt = objObjectMap.find(lease.object.handle);
  const bool inObjectMap = sourceObjectIt != ObjectMap.end();
  const bool inDeferredMap = sourceDeferredIt != objObjectMap.end();
  if (inObjectMap && inDeferredMap) {
    return reportDwgFrameTransitionFailure(
        sourceFrameId(sourceObjectIt->second), sourceObjectIt->second.loc,
        true);
  }

  const auto matchesLease = [&](const objHandle &object) {
    return object.handle == lease.object.handle &&
           sourceFrameId(object) == lease.source;
  };
  if ((inObjectMap && !matchesLease(sourceObjectIt->second)) ||
      (inDeferredMap && !matchesLease(sourceDeferredIt->second))) {
    const objHandle &mapped =
        inObjectMap ? sourceObjectIt->second : sourceDeferredIt->second;
    return reportDwgFrameTransitionFailure(sourceFrameId(mapped), mapped.loc,
                                           true);
  }

  if (target.find(lease.object.handle) != target.end()) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }

  // Reserve before extracting a source node so allocation failure cannot
  // leave a covered frame detached from both parser maps.
  try {
    target.reserve(target.size() + 1u);
  } catch (...) {
    return false;
  }

  DwgObjectMap *sourceMap = nullptr;
  DwgObjectMap::iterator sourceIt;
  if (inObjectMap) {
    sourceMap = &ObjectMap;
    sourceIt = sourceObjectIt;
  } else if (inDeferredMap) {
    sourceMap = &objObjectMap;
    sourceIt = sourceDeferredIt;
  }

  if (sourceMap != nullptr) {
    if (sourceMap == &target) {
      return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                             true);
    }

    DwgObjectMap::node_type node = sourceMap->extract(sourceIt);
    DwgObjectMap::insert_return_type inserted;
    try {
      inserted = target.insert(std::move(node));
    } catch (...) {
      sourceMap->insert(std::move(node));
      return false;
    }
    if (!inserted.inserted) {
      sourceMap->insert(std::move(inserted.node));
      return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                             true);
    }
    if (!lease.hasCoverage ||
        markDwgFrameOutcome(lease.source, DRW_DwgFrameDisposition::Deferred)) {
      return true;
    }

    DwgObjectMap::node_type rollback = target.extract(lease.object.handle);
    sourceMap->insert(std::move(rollback));
    return false;
  }

  try {
    const auto inserted = target.emplace(lease.object.handle, lease.object);
    if (!inserted.second) {
      return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                             true);
    }
  } catch (...) {
    return false;
  }

  if (!lease.hasCoverage ||
      markDwgFrameOutcome(lease.source, DRW_DwgFrameDisposition::Deferred)) {
    return true;
  }

  target.erase(lease.object.handle);
  return false;
}

bool dwgReader::discardDwgSourceFrame(DwgObjectMap &map,
                                      DwgObjectMap::iterator it) {
  if (it == map.end())
    return false;

  const objHandle object = it->second;
  const DwgSourceFrameId source = sourceFrameId(object);
  if (object.handle != it->first)
    return reportDwgFrameTransitionFailure(source, object.loc, true);
  if ((&map == &ObjectMap &&
       objObjectMap.find(object.handle) != objObjectMap.end()) ||
      (&map == &objObjectMap &&
       ObjectMap.find(object.handle) != ObjectMap.end())) {
    return reportDwgFrameTransitionFailure(source, object.loc, true);
  }

  if (&map == &ObjectMap &&
      objObjectMap.find(object.handle) != objObjectMap.end()) {
    return reportDwgFrameTransitionFailure(source, object.loc, true);
  }
  if (&map == &objObjectMap &&
      ObjectMap.find(object.handle) != ObjectMap.end()) {
    return reportDwgFrameTransitionFailure(source, object.loc, true);
  }

  if (m_dwgFrameCoverageStatus == DRW_DwgFrameCoverageStatus::NotAvailable) {
    try {
      m_quarantinedEntityHandles.insert(object.handle);
    } catch (...) {
      // The source-less marker only suppresses later recovery sweeps.
    }
    map.erase(it);
    return true;
  }

  if (!quarantineDwgFrame(source))
    return false;
  map.erase(it);
  return true;
}

bool dwgReader::quarantineMappedDwgSourceFrame(std::uint32_t handle) {
  const auto objectIt = ObjectMap.find(handle);
  const auto deferredIt = objObjectMap.find(handle);
  if (objectIt != ObjectMap.end() && deferredIt != objObjectMap.end()) {
    return reportDwgFrameTransitionFailure(sourceFrameId(objectIt->second),
                                           objectIt->second.loc, true);
  }

  if (m_dwgFrameCoverageStatus == DRW_DwgFrameCoverageStatus::NotAvailable) {
    if (objectIt == ObjectMap.end() && deferredIt == objObjectMap.end()) {
      return false;
    }
    try {
      m_quarantinedEntityHandles.insert(handle);
    } catch (...) {
      // Source-less recovery has no ledger. The marker is a best-effort
      // guard against later standalone child dispatch.
    }
    return true;
  }

  if (objectIt != ObjectMap.end())
    return quarantineDwgFrame(sourceFrameId(objectIt->second));

  if (deferredIt != objObjectMap.end())
    return quarantineDwgFrame(sourceFrameId(deferredIt->second));

  return false;
}

bool dwgReader::stageCurrentEntityFrame(
    DwgStagedFrame &frame, const DRW_DwgFramePublication &publication) {
  if (frame.publication.has_value())
    return reportDwgFrameTransitionFailure(
        DwgSourceFrameId{publication.m_handle});
  if (!frame.lease.has_value())
    return reportDwgFrameTransitionFailure(
        DwgSourceFrameId{publication.m_handle});
  if (!frame.hasDetachedLease())
    return reportDwgFrameTransitionFailure(frame.lease->source,
                                           frame.lease->object.loc, true);

  const DwgFrameMapLease &lease = *frame.lease;
  const DwgSourceFrameId source{
      publication.m_handle, publication.m_sourceOffset,
      publication.m_sourceMapOrdinal, publication.m_sourceOffsetSpace};
  if (!lease.hasCoverage || !publication.m_hasSourceLocation ||
      !(lease.source == source)) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }

  try {
    frame.publication.emplace(publication);
  } catch (...) {
    return false;
  }
  return true;
}

bool dwgReader::validateStagedFrame(const DwgStagedFrame &frame) {
  if (!frame.lease.has_value()) {
    if (!frame.publication.has_value())
      return true;
    return reportDwgFrameTransitionFailure(
        DwgSourceFrameId{frame.publication->m_handle});
  }

  const DwgFrameMapLease &lease = *frame.lease;
  if (!lease.isDetached() || lease.origin == DwgFrameMapLease::Origin::None ||
      lease.object.handle != lease.source.handle) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  if (ObjectMap.find(lease.object.handle) != ObjectMap.end() ||
      objObjectMap.find(lease.object.handle) != objObjectMap.end()) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  if (!lease.hasCoverage) {
    if (!frame.publication.has_value())
      return true;
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  if (!frame.publication.has_value() ||
      m_dwgFrameCoverageStatus == DRW_DwgFrameCoverageStatus::NotAvailable) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }

  const DRW_DwgFramePublication &publication = *frame.publication;
  const DwgSourceFrameId publicationSource{
      publication.m_handle, publication.m_sourceOffset,
      publication.m_sourceMapOrdinal, publication.m_sourceOffsetSpace};
  if (!publication.m_hasSourceLocation ||
      !(lease.source == publicationSource)) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  const auto sourceIt = m_dwgSourceFrameIndexes.find(lease.source.handle);
  if (sourceIt == m_dwgSourceFrameIndexes.end() ||
      sourceIt->second >= m_dwgSourceFrameLedger.size()) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  const DRW_DwgFrameCoverageEntry &entry =
      m_dwgSourceFrameLedger[sourceIt->second];
  const DwgSourceFrameId expected{entry.m_handle, entry.m_sourceOffset,
                                  entry.m_sourceMapOrdinal,
                                  entry.m_sourceOffsetSpace};
  if (!(lease.source == expected) ||
      entry.m_disposition != DRW_DwgFrameDisposition::Staged ||
      entry.m_publicationCount != 0) {
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  return true;
}

bool dwgReader::validateStagedCompoundState() {
  if (m_pendingInsertStates.empty() && m_orphanAttribStates.empty() &&
      m_pendingPolylineStates.empty() && m_orphanPolylineVertexStates.empty() &&
      m_stagedSeqEnds.empty()) {
    return true;
  }

  const auto fail = [this](std::uint32_t handle) {
    const DwgSourceFrameId source = sourceFrameIdForHandle(handle);
    return reportDwgFrameTransitionFailure(source, source.offset, true);
  };
  std::vector<std::uint32_t> attributeHandles;
  for (const auto &item : m_pendingInsertStates) {
    const std::uint32_t handle = item.first;
    const PendingInsertState &pending = item.second;
    if (handle == DRW::NoHandle || pending.entity.handle != handle ||
        pending.entity.attlist.size() != pending.attributes.size() ||
        !validateStagedFrame(pending.frame)) {
      return fail(handle);
    }
    for (std::size_t index = 0; index < pending.attributes.size(); ++index) {
      const StagedAttribState &attribute = pending.attributes[index];
      if (attribute.entity == nullptr ||
          attribute.entity->parentHandle != handle ||
          pending.entity.attlist[index] != attribute.entity ||
          std::find(attributeHandles.cbegin(), attributeHandles.cend(),
                    attribute.entity->handle) != attributeHandles.cend() ||
          !validateStagedFrame(attribute.frame)) {
        return fail(handle);
      }
      attributeHandles.push_back(attribute.entity->handle);
    }
  }
  for (const auto &item : m_orphanAttribStates) {
    const std::uint32_t owner = item.first;
    if (owner == DRW::NoHandle ||
        m_pendingInsertStates.find(owner) != m_pendingInsertStates.end()) {
      return fail(owner);
    }
    for (const StagedAttribState &attribute : item.second.attributes) {
      if (attribute.entity == nullptr ||
          attribute.entity->parentHandle != owner ||
          std::find(attributeHandles.cbegin(), attributeHandles.cend(),
                    attribute.entity->handle) != attributeHandles.cend() ||
          !validateStagedFrame(attribute.frame)) {
        return fail(owner);
      }
      attributeHandles.push_back(attribute.entity->handle);
    }
  }
  std::vector<std::uint32_t> vertexHandles;
  for (const auto &item : m_pendingPolylineStates) {
    const std::uint32_t handle = item.first;
    const PendingPolylineState &pending = item.second;
    if (handle == DRW::NoHandle || pending.entity.handle != handle ||
        m_invalidPolylineOwners.find(handle) != m_invalidPolylineOwners.end() ||
        !validateStagedFrame(pending.frame)) {
      return fail(handle);
    }
    for (const StagedVertexState &vertex : pending.vertices) {
      if (vertex.entity.handle == DRW::NoHandle ||
          vertex.entity.parentHandle != handle ||
          std::find(vertexHandles.cbegin(), vertexHandles.cend(),
                    vertex.entity.handle) != vertexHandles.cend() ||
          !validateStagedFrame(vertex.frame)) {
        return fail(handle);
      }
      vertexHandles.push_back(vertex.entity.handle);
    }
  }
  for (const auto &item : m_orphanPolylineVertexStates) {
    const std::uint32_t owner = item.first;
    if (owner == DRW::NoHandle ||
        m_pendingPolylineStates.find(owner) != m_pendingPolylineStates.end() ||
        m_invalidPolylineOwners.find(owner) != m_invalidPolylineOwners.end()) {
      return fail(owner);
    }
    for (const StagedVertexState &vertex : item.second.vertices) {
      if (vertex.entity.handle == DRW::NoHandle ||
          vertex.entity.parentHandle != owner ||
          std::find(vertexHandles.cbegin(), vertexHandles.cend(),
                    vertex.entity.handle) != vertexHandles.cend() ||
          !validateStagedFrame(vertex.frame)) {
        return fail(owner);
      }
      vertexHandles.push_back(vertex.entity.handle);
    }
  }
  for (const auto &item : m_stagedSeqEnds) {
    const std::uint32_t handle = item.first;
    const StagedSeqEndState &sequenceEnd = item.second;
    if (handle == DRW::NoHandle || sequenceEnd.owner == DRW::NoHandle ||
        m_invalidSeqEndHandles.find(handle) != m_invalidSeqEndHandles.end() ||
        m_consumedSeqEndHandles.find(handle) != m_consumedSeqEndHandles.end() ||
        !validateStagedFrame(sequenceEnd.frame)) {
      return fail(handle);
    }
  }
  return true;
}

bool dwgReader::hasPendingCompoundState() const noexcept {
  return !m_pendingInsertStates.empty() || !m_orphanAttribStates.empty() ||
         !m_pendingPolylineStates.empty() ||
         !m_orphanPolylineVertexStates.empty() || !m_stagedSeqEnds.empty();
}

bool dwgReader::abandonStagedCompoundState() {
  std::vector<std::uint32_t> pendingHandles;
  std::vector<std::uint32_t> orphanOwners;
  std::vector<std::uint32_t> polylineHandles;
  std::vector<std::uint32_t> polylineOrphanOwners;
  std::vector<std::uint32_t> sequenceHandles;
  try {
    pendingHandles.reserve(m_pendingInsertStates.size());
    orphanOwners.reserve(m_orphanAttribStates.size());
    polylineHandles.reserve(m_pendingPolylineStates.size());
    polylineOrphanOwners.reserve(m_orphanPolylineVertexStates.size());
    sequenceHandles.reserve(m_stagedSeqEnds.size());
    for (const auto &item : m_pendingInsertStates)
      pendingHandles.push_back(item.first);
    for (const auto &item : m_orphanAttribStates)
      orphanOwners.push_back(item.first);
    for (const auto &item : m_pendingPolylineStates)
      polylineHandles.push_back(item.first);
    for (const auto &item : m_orphanPolylineVertexStates)
      polylineOrphanOwners.push_back(item.first);
    for (const auto &item : m_stagedSeqEnds)
      sequenceHandles.push_back(item.first);
  } catch (...) {
    return false;
  }
  std::sort(pendingHandles.begin(), pendingHandles.end());
  std::sort(orphanOwners.begin(), orphanOwners.end());
  std::sort(polylineHandles.begin(), polylineHandles.end());
  std::sort(polylineOrphanOwners.begin(), polylineOrphanOwners.end());
  std::sort(sequenceHandles.begin(), sequenceHandles.end());

  for (const std::uint32_t handle : pendingHandles) {
    abandonPendingInsertState(handle);
    if (m_pendingInsertStates.find(handle) != m_pendingInsertStates.end())
      return false;
  }
  for (const std::uint32_t owner : orphanOwners) {
    terminalizeOrphanAttribOwner(owner);
    if (m_orphanAttribStates.find(owner) != m_orphanAttribStates.end())
      return false;
  }
  for (const std::uint32_t handle : polylineHandles) {
    terminalizePendingPolylineState(handle,
                                    DwgInsertTerminalReason::MalformedGroup);
    if (m_pendingPolylineStates.find(handle) != m_pendingPolylineStates.end()) {
      return false;
    }
  }
  for (const std::uint32_t owner : polylineOrphanOwners) {
    terminalizeOrphanPolylineVertexOwner(owner);
    if (m_orphanPolylineVertexStates.find(owner) !=
        m_orphanPolylineVertexStates.end()) {
      return false;
    }
  }
  for (const std::uint32_t handle : sequenceHandles) {
    const auto sequenceIt = m_stagedSeqEnds.find(handle);
    if (sequenceIt == m_stagedSeqEnds.end())
      continue;
    bool insertedMarker = false;
    if (!claimInvalidSeqEndTerminalizerMarker(handle, insertedMarker))
      return false;
    if (!abandonStagedFrame(sequenceIt->second.frame))
      return false;
    m_stagedSeqEnds.erase(sequenceIt);
  }
  return m_pendingInsertStates.empty() && m_orphanAttribStates.empty() &&
         m_pendingPolylineStates.empty() &&
         m_orphanPolylineVertexStates.empty() && m_stagedSeqEnds.empty();
}

bool dwgReader::restoreStagedFrame(DwgStagedFrame &frame) {
  if (!frame.lease.has_value()) {
    if (frame.publication.has_value())
      return reportDwgFrameTransitionFailure(
          DwgSourceFrameId{frame.publication->m_handle});
    return true;
  }
  if (!frame.hasDetachedLease())
    return reportDwgFrameTransitionFailure(frame.lease->source,
                                           frame.lease->object.loc, true);
  if (frame.hasDetachedLease() && !restoreDwgSourceFrame(*frame.lease)) {
    return false;
  }
  frame.lease.reset();
  frame.publication.reset();
  return true;
}

bool dwgReader::abandonStagedFrame(DwgStagedFrame &frame) {
  if (!frame.lease.has_value()) {
    if (frame.publication.has_value())
      return reportDwgFrameTransitionFailure(
          DwgSourceFrameId{frame.publication->m_handle});
    return true;
  }
  if (!frame.hasDetachedLease())
    return reportDwgFrameTransitionFailure(frame.lease->source,
                                           frame.lease->object.loc, true);
  if (frame.hasDetachedLease()) {
    DwgFrameMapLease &lease = *frame.lease;
    if (lease.hasCoverage) {
      const auto sourceIt = m_dwgSourceFrameIndexes.find(lease.source.handle);
      if (sourceIt == m_dwgSourceFrameIndexes.end() ||
          sourceIt->second >= m_dwgSourceFrameLedger.size()) {
        return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                               true);
      }
      const DRW_DwgFrameDisposition disposition =
          m_dwgSourceFrameLedger[sourceIt->second].m_disposition;
      if (disposition == DRW_DwgFrameDisposition::Staged &&
          !markDwgFrameOutcome(lease.source,
                               DRW_DwgFrameDisposition::Quarantined)) {
        return false;
      }
      if (disposition != DRW_DwgFrameDisposition::Staged &&
          disposition != DRW_DwgFrameDisposition::Failed &&
          disposition != DRW_DwgFrameDisposition::Quarantined) {
        return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                               true);
      }
      if (!discardDetachedDwgSourceFrame(lease)) {
        return false;
      }
    } else {
      try {
        m_quarantinedEntityHandles.insert(lease.object.handle);
      } catch (...) {
        // Source-less duplicate suppression is best effort.
      }
      lease.node.reset();
      lease.origin = DwgFrameMapLease::Origin::None;
    }
  }
  frame.lease.reset();
  frame.publication.reset();
  return true;
}

bool dwgReader::markDwgFrameOutcome(
    std::uint32_t handle, DRW_DwgFrameDisposition disposition,
    DRW_DwgFrameCoverageReason reason) noexcept {
  return markDwgFrameOutcome(sourceFrameIdForHandle(handle), disposition,
                             reason);
}

bool dwgReader::markDwgFrameOutcome(
    const DwgSourceFrameId &source, DRW_DwgFrameDisposition disposition,
    DRW_DwgFrameCoverageReason reason) noexcept {
  const auto it = m_dwgSourceFrameIndexes.find(source.handle);
  if (it == m_dwgSourceFrameIndexes.end() ||
      it->second >= m_dwgSourceFrameLedger.size()) {
    return reportDwgFrameTransitionFailure(source);
  }
  DRW_DwgFrameCoverageEntry &entry = m_dwgSourceFrameLedger[it->second];
  const DwgSourceFrameId expected{entry.m_handle, entry.m_sourceOffset,
                                  entry.m_sourceMapOrdinal,
                                  entry.m_sourceOffsetSpace};
  if (!(source == expected)) {
    return reportDwgFrameTransitionFailure(source, entry.m_sourceOffset, true);
  }
  if (reason == DRW_DwgFrameCoverageReason::None) {
    switch (disposition) {
    case DRW_DwgFrameDisposition::Published:
      reason = DRW_DwgFrameCoverageReason::ReceiptPublished;
      break;
    case DRW_DwgFrameDisposition::Deferred:
      reason = DRW_DwgFrameCoverageReason::CompoundDeferred;
      break;
    case DRW_DwgFrameDisposition::Staged:
      reason = DRW_DwgFrameCoverageReason::CompoundStaged;
      break;
    case DRW_DwgFrameDisposition::Quarantined:
      reason = DRW_DwgFrameCoverageReason::Quarantined;
      break;
    case DRW_DwgFrameDisposition::Failed:
      reason = DRW_DwgFrameCoverageReason::ParseFailure;
      break;
    case DRW_DwgFrameDisposition::Unresolved:
      reason = DRW_DwgFrameCoverageReason::FinalizationUnresolved;
      break;
    case DRW_DwgFrameDisposition::Pending:
    default:
      break;
    }
  }

  const DRW_DwgFrameDisposition current = entry.m_disposition;
  if (current == disposition) {
    if (disposition == DRW_DwgFrameDisposition::Published &&
        entry.m_publicationCount != 1) {
      return reportDwgFrameTransitionFailure(source, entry.m_sourceOffset,
                                             true);
    }
    return true;
  }

  const bool allowed =
      current == DRW_DwgFrameDisposition::Pending
          ? disposition == DRW_DwgFrameDisposition::Deferred ||
                disposition == DRW_DwgFrameDisposition::Staged ||
                disposition == DRW_DwgFrameDisposition::Published ||
                disposition == DRW_DwgFrameDisposition::Quarantined ||
                disposition == DRW_DwgFrameDisposition::Failed
          : (current == DRW_DwgFrameDisposition::Deferred ||
             current == DRW_DwgFrameDisposition::Staged) &&
                (disposition == DRW_DwgFrameDisposition::Published ||
                 disposition == DRW_DwgFrameDisposition::Quarantined ||
                 disposition == DRW_DwgFrameDisposition::Failed);
  if (!allowed)
    return reportDwgFrameTransitionFailure(source, entry.m_sourceOffset, true);

  if (disposition == DRW_DwgFrameDisposition::Published) {
    if (entry.m_publicationCount != 0)
      return reportDwgFrameTransitionFailure(source, entry.m_sourceOffset,
                                             true);
    entry.m_publicationCount = 1;
  }
  entry.m_disposition = disposition;
  entry.m_reason = reason;
  return true;
}

bool dwgReader::quarantineDwgFrame(std::uint32_t handle) {
  if (handle == DRW::NoHandle)
    return false;
  return quarantineDwgFrame(sourceFrameIdForHandle(handle));
}

bool dwgReader::quarantineDwgFrame(const DwgSourceFrameId &source) {
  if (source.handle == DRW::NoHandle)
    return false;
  const auto sourceIt = m_dwgSourceFrameIndexes.find(source.handle);
  if (sourceIt == m_dwgSourceFrameIndexes.end() ||
      sourceIt->second >= m_dwgSourceFrameLedger.size()) {
    (void)reportDwgFrameTransitionFailure(source);
    return false;
  }

  const DRW_DwgFrameDisposition disposition =
      m_dwgSourceFrameLedger[sourceIt->second].m_disposition;
  if (disposition == DRW_DwgFrameDisposition::Published ||
      disposition == DRW_DwgFrameDisposition::Unresolved) {
    (void)reportDwgFrameTransitionFailure(
        source, m_dwgSourceFrameLedger[sourceIt->second].m_sourceOffset, true);
    return false;
  }
  if (disposition != DRW_DwgFrameDisposition::Failed &&
      disposition != DRW_DwgFrameDisposition::Quarantined &&
      !markDwgFrameOutcome(source, DRW_DwgFrameDisposition::Quarantined)) {
    return false;
  }
  try {
    m_quarantinedEntityHandles.insert(source.handle);
  } catch (...) {
    // The ledger state remains authoritative if optional parser cleanup
    // cannot allocate its duplicate-suppression marker.
  }
  return true;
}

bool dwgReader::suppressDwgFrame(const DwgSourceFrameId &source,
                                 bool hasCoverage) {
  if (source.handle == DRW::NoHandle)
    return false;
  if (hasCoverage)
    return quarantineDwgFrame(source);
  try {
    m_quarantinedEntityHandles.insert(source.handle);
  } catch (...) {
    // Source-less recovery has no coverage state to corrupt. The marker
    // is best effort and only prevents a later sweep from republishing it.
  }
  return true;
}

bool dwgReader::suppressDwgFramePublication(
    const DRW_DwgFramePublication &publication) {
  const DwgSourceFrameId source{
      publication.m_handle, publication.m_sourceOffset,
      publication.m_sourceMapOrdinal, publication.m_sourceOffsetSpace};
  const bool hasCoverage =
      m_dwgFrameCoverageStatus != DRW_DwgFrameCoverageStatus::NotAvailable;
  if (hasCoverage && !publication.m_hasSourceLocation)
    return reportDwgFrameTransitionFailure(source);
  return suppressDwgFrame(source, hasCoverage);
}

void dwgReader::normalizeDwgFramePublication(
    DRW_DwgFramePublication &publication) const {
  if (!publication.m_isCustomClass ||
      publication.m_classStreamOrdinal.has_value()) {
    return;
  }
  const auto classOrdinal = m_dwgClassNumberOrdinals.find(
      static_cast<std::uint32_t>(publication.m_encodedType));
  if (classOrdinal != m_dwgClassNumberOrdinals.end())
    publication.m_classStreamOrdinal = classOrdinal->second;
}

bool dwgReader::validateDwgFramePublicationStaticArtifacts(
    const DRW_DwgFramePublication &publication,
    DwgFramePublicationArtifacts artifacts) const noexcept {
  const DRW_DwgDictionaryMembership *const dictionaryMembership =
      artifacts.dictionaryMembership;
  if (dictionaryMembership != nullptr &&
      (!dictionaryMembership->m_complete ||
       !dictionaryMembership->m_hasSourceLocation ||
       dictionaryMembership->m_version != publication.m_version ||
       dictionaryMembership->m_dictionaryHandle != publication.m_handle ||
       dictionaryMembership->m_sourceOffset != publication.m_sourceOffset ||
       dictionaryMembership->m_sourceMapOrdinal !=
           publication.m_sourceMapOrdinal ||
       dictionaryMembership->m_sourceOffsetSpace !=
           publication.m_sourceOffsetSpace ||
       publication.m_resolvedType != dwgObjType::DICTIONARY)) {
    return false;
  }

  const DRW_DwgGroupMembership *const groupMembership =
      artifacts.groupMembership;
  if (groupMembership != nullptr) {
    if (!groupMembership->m_complete || !groupMembership->m_hasSourceLocation ||
        groupMembership->m_version != publication.m_version ||
        groupMembership->m_groupHandle != publication.m_handle ||
        groupMembership->m_sourceOffset != publication.m_sourceOffset ||
        groupMembership->m_sourceMapOrdinal != publication.m_sourceMapOrdinal ||
        groupMembership->m_sourceOffsetSpace !=
            publication.m_sourceOffsetSpace ||
        publication.m_resolvedType != DRW_Group::kDwgFixedType ||
        publication.m_isEntity ||
        groupMembership->m_entries.size() > DRW_Group::kMaxEntityHandles) {
      return false;
    }
    for (std::size_t index = 0; index < groupMembership->m_entries.size();
         ++index) {
      const DRW_DwgGroupMembership::Entry &entry =
          groupMembership->m_entries[index];
      if (entry.m_handle == DRW::NoHandle ||
          entry.m_ordinal != static_cast<std::uint32_t>(index)) {
        return false;
      }
    }
  }

  const DRW_DwgSortEntsMembership *const sortEntsMembership =
      artifacts.sortEntsMembership;
  const bool sortEntsTable = publication.m_isCustomClass &&
                             (publication.m_recordName == "SORTENTSTABLE" ||
                              publication.m_className == "AcDbSortentsTable");
  if (sortEntsMembership != nullptr) {
    if (!sortEntsMembership->m_complete ||
        !sortEntsMembership->m_hasSourceLocation ||
        sortEntsMembership->m_version != publication.m_version ||
        sortEntsMembership->m_tableHandle != publication.m_handle ||
        sortEntsMembership->m_sourceOffset != publication.m_sourceOffset ||
        sortEntsMembership->m_sourceMapOrdinal !=
            publication.m_sourceMapOrdinal ||
        sortEntsMembership->m_sourceOffsetSpace !=
            publication.m_sourceOffsetSpace ||
        sortEntsMembership->m_encodedType != publication.m_encodedType ||
        sortEntsMembership->m_resolvedType != publication.m_resolvedType ||
        sortEntsMembership->m_recordName != publication.m_recordName ||
        sortEntsMembership->m_className != publication.m_className ||
        !sortEntsMembership->m_classStreamOrdinal.has_value() ||
        !publication.m_classStreamOrdinal.has_value() ||
        sortEntsMembership->m_classStreamOrdinal !=
            publication.m_classStreamOrdinal ||
        sortEntsMembership->m_blockOwnerHandle == DRW::NoHandle ||
        sortEntsMembership->m_entries.size() > DRW_SortEntsTable::kMaxEntries ||
        publication.m_isEntity || !sortEntsTable) {
      return false;
    }
    for (std::size_t index = 0; index < sortEntsMembership->m_entries.size();
         ++index) {
      const DRW_DwgSortEntsMembership::Entry &entry =
          sortEntsMembership->m_entries[index];
      if (entry.m_entityHandle == DRW::NoHandle ||
          entry.m_ordinal != static_cast<std::uint32_t>(index) ||
          entry.m_sortFallsBackToEntity !=
              (entry.m_sortHandle == DRW::NoHandle)) {
        return false;
      }
    }
  }

  const DRW_DwgFieldListMembership *const fieldListMembership =
      artifacts.fieldListMembership;
  const bool fieldList = publication.m_isCustomClass &&
                         !publication.m_isEntity &&
                         (publication.m_recordName == "FIELDLIST" ||
                          publication.m_className == "AcDbFieldList");
  if (fieldListMembership != nullptr) {
    if (!fieldListMembership->m_complete ||
        !fieldListMembership->m_hasSourceLocation ||
        fieldListMembership->m_version != publication.m_version ||
        fieldListMembership->m_listHandle != publication.m_handle ||
        fieldListMembership->m_sourceOffset != publication.m_sourceOffset ||
        fieldListMembership->m_sourceMapOrdinal !=
            publication.m_sourceMapOrdinal ||
        fieldListMembership->m_sourceOffsetSpace !=
            publication.m_sourceOffsetSpace ||
        fieldListMembership->m_encodedType != publication.m_encodedType ||
        fieldListMembership->m_resolvedType != publication.m_resolvedType ||
        fieldListMembership->m_recordName != publication.m_recordName ||
        fieldListMembership->m_className != publication.m_className ||
        !fieldListMembership->m_classStreamOrdinal.has_value() ||
        !publication.m_classStreamOrdinal.has_value() ||
        fieldListMembership->m_classStreamOrdinal !=
            publication.m_classStreamOrdinal ||
        fieldListMembership->m_entries.size() > DRW_Field::kMaxItems ||
        publication.m_version < DRW::AC1015 || !fieldList) {
      return false;
    }
    for (std::size_t index = 0; index < fieldListMembership->m_entries.size();
         ++index) {
      if (fieldListMembership->m_entries[index].m_ordinal !=
          static_cast<std::uint32_t>(index)) {
        return false;
      }
    }
  }

  const bool dictionaryWithDefault =
      publication.m_isCustomClass && !publication.m_isEntity &&
      publication.m_className == "AcDbDictionaryWithDefault";
  const DRW_DwgDictionaryWithDefaultMembership *const
      dictionaryWithDefaultMembership =
          artifacts.dictionaryWithDefaultMembership;
  if (dictionaryWithDefaultMembership != nullptr) {
    if (!dictionaryWithDefaultMembership->m_complete ||
        !dictionaryWithDefaultMembership->m_hasSourceLocation ||
        dictionaryWithDefaultMembership->m_version != publication.m_version ||
        dictionaryWithDefaultMembership->m_dictionaryHandle !=
            publication.m_handle ||
        dictionaryWithDefaultMembership->m_sourceOffset !=
            publication.m_sourceOffset ||
        dictionaryWithDefaultMembership->m_sourceMapOrdinal !=
            publication.m_sourceMapOrdinal ||
        dictionaryWithDefaultMembership->m_sourceOffsetSpace !=
            publication.m_sourceOffsetSpace ||
        dictionaryWithDefaultMembership->m_encodedType !=
            publication.m_encodedType ||
        dictionaryWithDefaultMembership->m_resolvedType !=
            publication.m_resolvedType ||
        dictionaryWithDefaultMembership->m_recordName !=
            publication.m_recordName ||
        dictionaryWithDefaultMembership->m_className !=
            publication.m_className ||
        !dictionaryWithDefaultMembership->m_classStreamOrdinal.has_value() ||
        !publication.m_classStreamOrdinal.has_value() ||
        dictionaryWithDefaultMembership->m_classStreamOrdinal !=
            publication.m_classStreamOrdinal ||
        dictionaryWithDefaultMembership->m_hardOwner < 0 ||
        dictionaryWithDefaultMembership->m_hardOwner > 1 ||
        dictionaryWithDefaultMembership->m_defaultEntryHandle ==
            DRW::NoHandle ||
        dictionaryWithDefaultMembership->m_entries.size() >
            DRW_Dictionary::kMaxEntries ||
        publication.m_version < DRW::AC1015 || !dictionaryWithDefault) {
      return false;
    }
    for (const DRW_DwgDictionaryWithDefaultMembership::Entry &entry :
         dictionaryWithDefaultMembership->m_entries) {
      if (entry.m_name.empty() || entry.m_handle == DRW::NoHandle)
        return false;
    }
  }

  const DRW_DwgFieldPayloadReceipt *const fieldPayloadReceipt =
      artifacts.fieldPayloadReceipt;
  if (fieldPayloadReceipt != nullptr) {
    const DRW_Field &field = fieldPayloadReceipt->m_field;
    const bool fieldObject = publication.m_isCustomClass &&
                             !publication.m_isEntity &&
                             publication.m_recordName == "FIELD" &&
                             publication.m_className == "AcDbField";
    if (!fieldPayloadReceipt->m_complete ||
        !fieldPayloadReceipt->m_hasSourceLocation ||
        fieldPayloadReceipt->m_version != publication.m_version ||
        fieldPayloadReceipt->m_fieldHandle != publication.m_handle ||
        fieldPayloadReceipt->m_sourceOffset != publication.m_sourceOffset ||
        fieldPayloadReceipt->m_sourceMapOrdinal !=
            publication.m_sourceMapOrdinal ||
        fieldPayloadReceipt->m_sourceOffsetSpace !=
            publication.m_sourceOffsetSpace ||
        fieldPayloadReceipt->m_encodedType != publication.m_encodedType ||
        fieldPayloadReceipt->m_resolvedType != publication.m_resolvedType ||
        fieldPayloadReceipt->m_recordName != publication.m_recordName ||
        fieldPayloadReceipt->m_className != publication.m_className ||
        !fieldPayloadReceipt->m_classStreamOrdinal.has_value() ||
        !publication.m_classStreamOrdinal.has_value() ||
        fieldPayloadReceipt->m_classStreamOrdinal !=
            publication.m_classStreamOrdinal ||
        field.handle != publication.m_handle ||
        !field.hasCompleteDwgPayload() ||
        !field.isDwgPayloadValid(publication.m_version) ||
        publication.m_version < DRW::AC1015 || !fieldObject) {
      return false;
    }
  }

  const DRW_DwgTypedReference *const typedReference = artifacts.typedReference;
  return typedReference == nullptr ||
         (typedReference->m_complete && typedReference->m_hasSourceLocation &&
          typedReference->m_version == publication.m_version &&
          typedReference->m_sourceHandle == publication.m_handle &&
          typedReference->m_sourceOffset == publication.m_sourceOffset &&
          typedReference->m_sourceMapOrdinal ==
              publication.m_sourceMapOrdinal &&
          typedReference->m_sourceOffsetSpace ==
              publication.m_sourceOffsetSpace &&
          typedReference->m_encodedType == publication.m_encodedType &&
          typedReference->m_resolvedType == publication.m_resolvedType &&
          typedReference->m_classStreamOrdinal.has_value() &&
          publication.m_classStreamOrdinal.has_value() &&
          typedReference->m_classStreamOrdinal ==
              publication.m_classStreamOrdinal &&
          typedReference->m_field ==
              DRW_DwgTypedReferenceField::DictionaryDefault &&
          typedReference->m_referenceCode == DRW::DwgHardPointer &&
          typedReference->m_targetHandle != DRW::NoHandle &&
          (dictionaryWithDefaultMembership == nullptr ||
           typedReference->m_targetHandle ==
               dictionaryWithDefaultMembership->m_defaultEntryHandle) &&
          dictionaryWithDefault);
}

bool dwgReader::publishDwgFramePublication(
    DRW_Interface &intfa, DRW_DwgFramePublication publication,
    DwgFramePublicationArtifacts artifacts,
    DwgFieldFamilyPublicationOutputs fieldOutputs) {
  if (m_dwgFrameCoverageStatus == DRW_DwgFrameCoverageStatus::NotAvailable) {
    try {
      if (fieldOutputs.field != nullptr)
        intfa.addField(*fieldOutputs.field);
      if (fieldOutputs.fieldList != nullptr)
        intfa.addFieldList(*fieldOutputs.fieldList);
      if (fieldOutputs.rawObject != nullptr)
        intfa.addUnsupportedObject(*fieldOutputs.rawObject);
    } catch (...) {
      return false;
    }
    return true;
  }
  if (!publication.m_hasSourceLocation) {
    return reportDwgFrameTransitionFailure(
        DwgSourceFrameId{publication.m_handle});
  }
  const DwgSourceFrameId frameSource{
      publication.m_handle, publication.m_sourceOffset,
      publication.m_sourceMapOrdinal, publication.m_sourceOffsetSpace};
  const auto sourceIt = m_dwgSourceFrameIndexes.find(publication.m_handle);
  if (sourceIt == m_dwgSourceFrameIndexes.end() ||
      sourceIt->second >= m_dwgSourceFrameLedger.size()) {
    return reportDwgFrameTransitionFailure(frameSource);
  }
  const DRW_DwgFrameCoverageEntry &entry =
      m_dwgSourceFrameLedger[sourceIt->second];
  const DwgSourceFrameId expected{entry.m_handle, entry.m_sourceOffset,
                                  entry.m_sourceMapOrdinal,
                                  entry.m_sourceOffsetSpace};
  if (!(frameSource == expected) ||
      (entry.m_disposition != DRW_DwgFrameDisposition::Pending &&
       entry.m_disposition != DRW_DwgFrameDisposition::Deferred &&
       entry.m_disposition != DRW_DwgFrameDisposition::Staged) ||
      entry.m_publicationCount != 0) {
    return reportDwgFrameTransitionFailure(frameSource, entry.m_sourceOffset,
                                           true);
  }
  normalizeDwgFramePublication(publication);
  const DRW_DwgDictionaryMembership *const dictionaryMembership =
      artifacts.dictionaryMembership;
  const DRW_DwgTypedReference *const typedReference = artifacts.typedReference;
  const DRW_DwgGroupMembership *const groupMembership =
      artifacts.groupMembership;
  const DRW_DwgSortEntsMembership *const sortEntsMembership =
      artifacts.sortEntsMembership;
  const DRW_DwgFieldListMembership *const fieldListMembership =
      artifacts.fieldListMembership;
  const DRW_DwgDictionaryWithDefaultMembership *const
      dictionaryWithDefaultMembership =
          artifacts.dictionaryWithDefaultMembership;
  const DRW_DwgFieldPayloadReceipt *const fieldPayloadReceipt =
      artifacts.fieldPayloadReceipt;
  if (!validateDwgFramePublicationStaticArtifacts(publication, artifacts)) {
    (void)markDwgFrameOutcome(frameSource, DRW_DwgFrameDisposition::Failed,
                              DRW_DwgFrameCoverageReason::ReceiptFailure);
    return false;
  }
  const auto matchesFieldListOutput = [&]() {
    if ((fieldOutputs.fieldList == nullptr) !=
        (fieldListMembership == nullptr)) {
      return false;
    }
    if (fieldOutputs.fieldList == nullptr)
      return true;
    const DRW_FieldList &fieldList = *fieldOutputs.fieldList;
    if (fieldList.handle != publication.m_handle ||
        !fieldList.hasCompleteDwgEntries() ||
        !fieldList.isDwgPayloadValid(publication.m_version) ||
        fieldList.m_fieldHandles.size() != fieldListMembership->m_entries.size()) {
      return false;
    }
    for (std::size_t index = 0; index < fieldList.m_fieldHandles.size();
         ++index) {
      const DRW_DwgFieldListMembership::Entry &entry =
          fieldListMembership->m_entries[index];
      if (entry.m_ordinal != static_cast<std::uint32_t>(index) ||
          entry.m_fieldHandle != fieldList.m_fieldHandles[index]) {
        return false;
      }
    }
    return true;
  };
  if ((fieldOutputs.field != nullptr &&
       (fieldPayloadReceipt == nullptr ||
        fieldOutputs.field->handle != publication.m_handle ||
        fieldOutputs.field->handle != fieldPayloadReceipt->m_field.handle)) ||
      !matchesFieldListOutput() ||
      (fieldOutputs.rawObject != nullptr &&
       (fieldOutputs.rawObject->m_version != publication.m_version ||
        fieldOutputs.rawObject->m_handle != publication.m_handle ||
        fieldOutputs.rawObject->m_objectType != publication.m_resolvedType ||
        !fieldOutputs.rawObject->m_isCustomClass ||
        fieldOutputs.rawObject->m_recordName != publication.m_recordName ||
        fieldOutputs.rawObject->m_className != publication.m_className))) {
    (void)markDwgFrameOutcome(frameSource, DRW_DwgFrameDisposition::Failed,
                              DRW_DwgFrameCoverageReason::ReceiptFailure);
    return false;
  }
  const DRW_DwgBlockReachability *const blockReachability =
      artifacts.blockReachability;
  const auto matchesLedgerSource =
      [this](const DRW_DwgSourceFrame &receiptSource) {
        if (receiptSource.m_handle == DRW::NoHandle ||
            !receiptSource.m_hasSourceLocation) {
          return false;
        }
        const auto source =
            m_dwgSourceFrameIndexes.find(receiptSource.m_handle);
        if (source == m_dwgSourceFrameIndexes.cend() ||
            source->second >= m_dwgSourceFrameLedger.size()) {
          return false;
        }
        const DRW_DwgFrameCoverageEntry &ledger =
            m_dwgSourceFrameLedger[source->second];
        return ledger.m_handle == receiptSource.m_handle &&
               ledger.m_sourceOffset == receiptSource.m_sourceOffset &&
               ledger.m_sourceMapOrdinal == receiptSource.m_sourceMapOrdinal &&
               ledger.m_sourceOffsetSpace == receiptSource.m_sourceOffsetSpace;
      };
  const auto sameReceiptSource = [](const DRW_DwgSourceFrame &source,
                                    const DRW_DwgFramePublication &receipt) {
    return source.m_handle == receipt.m_handle &&
           source.m_sourceOffset == receipt.m_sourceOffset &&
           source.m_sourceMapOrdinal == receipt.m_sourceMapOrdinal &&
           source.m_sourceOffsetSpace == receipt.m_sourceOffsetSpace &&
           source.m_hasSourceLocation == receipt.m_hasSourceLocation;
  };
  const auto receiptDisposition =
      [this](const DRW_DwgSourceFrame &receiptSource)
      -> std::optional<DRW_DwgFrameDisposition> {
    const auto source = m_dwgSourceFrameIndexes.find(receiptSource.m_handle);
    if (source == m_dwgSourceFrameIndexes.cend() ||
        source->second >= m_dwgSourceFrameLedger.size()) {
      return std::nullopt;
    }
    return m_dwgSourceFrameLedger[source->second].m_disposition;
  };
  const auto receiptPublished = [this](
                                    const DRW_DwgSourceFrame &receiptSource) {
    const auto source = m_dwgSourceFrameIndexes.find(receiptSource.m_handle);
    return source != m_dwgSourceFrameIndexes.cend() &&
           source->second < m_dwgSourceFrameLedger.size() &&
           m_dwgSourceFrameLedger[source->second].m_disposition ==
               DRW_DwgFrameDisposition::Published &&
           m_dwgSourceFrameLedger[source->second].m_publicationCount == 1u;
  };
  if (blockReachability != nullptr) {
    bool validReachability =
        blockReachability->m_complete &&
        blockReachability->m_version == publication.m_version &&
        publication.m_version >= DRW::AC1018 &&
        publication.m_resolvedType == dwgType::BLOCK &&
        publication.m_isEntity &&
        sameReceiptSource(blockReachability->m_block, publication) &&
        matchesLedgerSource(blockReachability->m_blockRecord) &&
        matchesLedgerSource(blockReachability->m_block) &&
        matchesLedgerSource(blockReachability->m_endBlock) &&
        receiptPublished(blockReachability->m_blockRecord) &&
        receiptDisposition(blockReachability->m_block) ==
            DRW_DwgFrameDisposition::Staged &&
        receiptDisposition(blockReachability->m_endBlock) ==
            DRW_DwgFrameDisposition::Staged &&
        blockReachability->m_blockRecord.m_handle != publication.m_handle &&
        blockReachability->m_endBlock.m_handle != publication.m_handle &&
        blockReachability->m_endBlock.m_handle !=
            blockReachability->m_blockRecord.m_handle;
    std::unordered_set<std::uint32_t> entityHandles;
    if (validReachability) {
      try {
        entityHandles.reserve(blockReachability->m_entities.size());
        for (const DRW_DwgSourceFrame &entity : blockReachability->m_entities) {
          if (!matchesLedgerSource(entity) || !receiptPublished(entity) ||
              entity.m_handle == publication.m_handle ||
              entity.m_handle == blockReachability->m_blockRecord.m_handle ||
              entity.m_handle == blockReachability->m_endBlock.m_handle ||
              !entityHandles.insert(entity.m_handle).second) {
            validReachability = false;
            break;
          }
        }
      } catch (...) {
        validReachability = false;
      }
    }
    if (!validReachability) {
      (void)markDwgFrameOutcome(frameSource, DRW_DwgFrameDisposition::Failed,
                                DRW_DwgFrameCoverageReason::ReceiptFailure);
      return false;
    }
  }
  try {
    intfa.addDwgFramePublication(publication);
    if (dictionaryMembership != nullptr)
      intfa.addDwgDictionaryMembership(*dictionaryMembership);
    if (dictionaryWithDefaultMembership != nullptr)
      intfa.addDwgDictionaryWithDefaultMembership(
          *dictionaryWithDefaultMembership);
    if (typedReference != nullptr)
      intfa.addDwgTypedReference(*typedReference);
    if (blockReachability != nullptr)
      intfa.addDwgBlockReachability(*blockReachability);
    if (groupMembership != nullptr)
      intfa.addDwgGroupMembership(*groupMembership);
    if (sortEntsMembership != nullptr)
      intfa.addDwgSortEntsMembership(*sortEntsMembership);
    if (fieldListMembership != nullptr)
      intfa.addDwgFieldListMembership(*fieldListMembership);
    if (fieldPayloadReceipt != nullptr)
      intfa.addDwgFieldPayloadReceipt(*fieldPayloadReceipt);
    if (fieldOutputs.field != nullptr)
      intfa.addField(*fieldOutputs.field);
    if (fieldOutputs.fieldList != nullptr)
      intfa.addFieldList(*fieldOutputs.fieldList);
    if (fieldOutputs.rawObject != nullptr)
      intfa.addUnsupportedObject(*fieldOutputs.rawObject);
  } catch (...) {
    (void)markDwgFrameOutcome(frameSource, DRW_DwgFrameDisposition::Failed,
                              DRW_DwgFrameCoverageReason::CallbackException);
    return false;
  }
  return markDwgFrameOutcome(frameSource, DRW_DwgFrameDisposition::Published,
                             DRW_DwgFrameCoverageReason::ReceiptPublished);
}

void dwgReader::finalizeDwgFrameCoverage(DRW_Interface &intfa,
                                         bool readCompleted) {
  if (m_dwgFrameCoverageStatus == DRW_DwgFrameCoverageStatus::NotAvailable ||
      m_dwgFrameCoveragePublished) {
    return;
  }

  for (DRW_DwgFrameCoverageEntry &entry : m_dwgSourceFrameLedger) {
    if (entry.m_disposition == DRW_DwgFrameDisposition::Pending ||
        entry.m_disposition == DRW_DwgFrameDisposition::Deferred ||
        entry.m_disposition == DRW_DwgFrameDisposition::Staged) {
      entry.m_disposition = DRW_DwgFrameDisposition::Unresolved;
      entry.m_reason = readCompleted
                           ? DRW_DwgFrameCoverageReason::FinalizationUnresolved
                           : DRW_DwgFrameCoverageReason::PhaseAborted;
    }
  }

  DRW_DwgFrameCoverageReport report;
  report.m_entries = m_dwgSourceFrameLedger;
  report.m_complete =
      readCompleted && !m_dwgFrameCoverageIntegrityViolation &&
      std::all_of(report.m_entries.cbegin(), report.m_entries.cend(),
                  [](const DRW_DwgFrameCoverageEntry &entry) {
                    return entry.m_disposition ==
                               DRW_DwgFrameDisposition::Published &&
                           entry.m_publicationCount == 1;
                  });
  report.m_status = report.m_complete
                        ? DRW_DwgFrameCoverageStatus::FinalizedComplete
                        : DRW_DwgFrameCoverageStatus::FinalizedPartial;
  m_dwgFrameCoverageStatus = report.m_status;
  m_dwgFrameCoveragePublished = true;
  intfa.addDwgFrameCoverageReport(report);
}

void dwgReader::finalizeDwgFrameCoverageNoThrow(DRW_Interface &intfa,
                                                bool readCompleted) noexcept {
  try {
    finalizeDwgFrameCoverage(intfa, readCompleted);
  } catch (...) {
    if (m_dwgFrameCoverageStatus != DRW_DwgFrameCoverageStatus::NotAvailable) {
      m_dwgFrameCoverageIntegrityViolation = true;
      m_dwgFrameCoverageStatus = DRW_DwgFrameCoverageStatus::FinalizedPartial;
      m_dwgFrameCoveragePublished = true;
    }
  }
}

namespace {

DRW_DwgFrameOffsetSpace
frameOffsetSpace(DwgIntegrityAddressSpace offsetSpace) noexcept {
  switch (offsetSpace) {
  case DwgIntegrityAddressSpace::PhysicalFile:
    return DRW_DwgFrameOffsetSpace::PhysicalFile;
  case DwgIntegrityAddressSpace::DecodedBuffer:
    return DRW_DwgFrameOffsetSpace::DecodedBuffer;
  case DwgIntegrityAddressSpace::None:
  default:
    return DRW_DwgFrameOffsetSpace::Unknown;
  }
}

// DWG control-object layouts do not use one common count field.  The
// listed controls encode numEntries as FIELD_BS; the remaining controls
// use FIELD_BL (dwg.spec control-object definitions).
bool controlEntryCountUsesBitShort(std::int16_t objectType) {
  switch (objectType) {
  case DRW::DwgLTypeControlObjectType:
  case DRW::DwgUcsControlObjectType:
  case DRW::DwgVPortControlObjectType:
  case DRW::DwgAppIdControlObjectType:
  case DRW::DwgDimStyleControlObjectType:
    return true;
  default:
    return false;
  }
}

bool controlHasPhantomEntries(std::int16_t objectType) {
  return objectType == DRW::DwgBlockControlObjectType ||
         objectType == DRW::DwgLTypeControlObjectType;
}

struct DwgTableDescriptor {
  std::int16_t controlType;
  std::int16_t recordType;
  const char *controlReceiptName;
};

constexpr DwgTableDescriptor kLTypeTable{
    DRW::DwgLTypeControlObjectType, DRW::DwgLTypeObjectType, "LTYPE_CONTROL"};
constexpr DwgTableDescriptor kLayerTable{
    DRW::DwgLayerControlObjectType, DRW::DwgLayerObjectType, "LAYER_CONTROL"};
constexpr DwgTableDescriptor kStyleTable{
    DRW::DwgStyleControlObjectType, DRW::DwgStyleObjectType, "STYLE_CONTROL"};
constexpr DwgTableDescriptor kDimStyleTable{DRW::DwgDimStyleControlObjectType,
                                            DRW::DwgDimStyleObjectType,
                                            "DIMSTYLE_CONTROL"};
constexpr DwgTableDescriptor kVPortTable{
    DRW::DwgVPortControlObjectType, DRW::DwgVPortObjectType, "VPORT_CONTROL"};
constexpr DwgTableDescriptor kBlockTable{DRW::DwgBlockControlObjectType,
                                         DRW::DwgBlockRecordObjectType,
                                         "BLOCK_CONTROL"};
constexpr DwgTableDescriptor kAppIdTable{
    DRW::DwgAppIdControlObjectType, DRW::DwgAppIdObjectType, "APPID_CONTROL"};
constexpr DwgTableDescriptor kViewTable{DRW::DwgViewControlObjectType,
                                        DRW::DwgViewObjectType, "VIEW_CONTROL"};
constexpr DwgTableDescriptor kUcsTable{DRW::DwgUcsControlObjectType,
                                       DRW::DwgUcsObjectType, "UCS_CONTROL"};

bool controlCurrentBit(const dwgBuffer &buffer, std::uint64_t &value) {
  std::uint64_t byteBits = 0;
  return dwgSafety::multiply(buffer.getPosition(), 8, byteBits) &&
         dwgSafety::add(byteBits, buffer.getBitPos(), value);
}

template <typename Value, typename Reader>
bool readControlValue(dwgBuffer &buffer, std::uint64_t endBit, Reader reader,
                      Value &value) {
  std::uint64_t currentBit = 0;
  if (!buffer.isGood() || !controlCurrentBit(buffer, currentBit) ||
      currentBit > endBit)
    return false;

  dwgBuffer probe = buffer.forkIndependent();
  const Value parsed = reader(probe);
  std::uint64_t parsedBit = 0;
  if (!probe.isGood() || !controlCurrentBit(probe, parsedBit) ||
      parsedBit > endBit)
    return false;
  buffer = probe;
  value = parsed;
  return true;
}

bool readControlBitShort(dwgBuffer &buffer, std::uint64_t endBit,
                         std::int32_t &value) {
  return readControlValue<std::int32_t>(
      buffer, endBit,
      [](dwgBuffer &probe) {
        return static_cast<std::int32_t>(probe.getBitShort());
      },
      value);
}

bool readControlBitLong(dwgBuffer &buffer, std::uint64_t endBit,
                        std::int32_t &value) {
  return readControlValue<std::int32_t>(
      buffer, endBit,
      [](dwgBuffer &probe) {
        return static_cast<std::int32_t>(probe.getBitLong());
      },
      value);
}

bool readControlRawChar(dwgBuffer &buffer, std::uint64_t endBit,
                        std::uint8_t &value) {
  return readControlValue<std::uint8_t>(
      buffer, endBit, [](dwgBuffer &probe) { return probe.getRawChar8(); },
      value);
}

bool readControlBit(dwgBuffer &buffer, std::uint64_t endBit, bool &value) {
  return readControlValue<bool>(
      buffer, endBit, [](dwgBuffer &probe) { return probe.getBit() != 0; },
      value);
}

bool isSpaceBlockRecordName(const std::string &name) {
  std::string normalized = name;
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 [](unsigned char value) {
                   return static_cast<char>(std::toupper(value));
                 });
  return normalized == "*MODEL_SPACE" || normalized == "*PAPER_SPACE";
}

// helper function to cleanup pointers in Look Up Tables
template <typename T>
void mapCleanUp(std::unordered_map<std::uint32_t, T *> &table) {
  for (auto &item : table)
    delete item.second;
}

DRW_DwgFramePublication
makeTypedEntityFramePublication(DRW::Version version, const objHandle &object,
                                int type, const DRW_Entity &entity) {
  DRW_DwgFramePublication publication;
  publication.m_version = version;
  publication.m_handle = object.handle;
  publication.m_sourceOffset = object.loc;
  publication.m_sourceMapOrdinal = object.sourceOrdinal;
  publication.m_sourceOffsetSpace = object.sourceOffsetSpace;
  publication.m_hasSourceLocation = true;
  publication.m_encodedType = type;
  publication.m_resolvedType = type;
  publication.m_isEntity = true;
  publication.setCommonLinkEvidence(drwDwgCommonLinkEvidenceForLinks(
      entity.hasDwgCommonLinkTail(), entity.parentHandle, entity.reactorHandles,
      entity.dwgReactorCount(), entity.xDictHandle));
  publication.m_parentHandle = entity.parentHandle;
  publication.m_reactorHandles = entity.reactorHandles;
  publication.m_xDictHandle = entity.xDictHandle;
  publication.m_numReactors = entity.dwgReactorCount();
  publication.m_xDictFlag = entity.dwgXDictionaryFlag();
  publication.m_carrier = DRW_DwgFramePublication::Carrier::Typed;
  return publication;
}

bool isExpectedAttribute(const DRW_Insert &insert, std::uint32_t handle,
                         DRW::Version version) {
  if (version < DRW::AC1018 && insert.attribHandles.size() == 2) {
    // R13-R2000 carries only the first and last ATTRIB handles. The
    // owner handle remains the authoritative membership check for any
    // attributes between those two boundaries.
    const std::uint32_t first = insert.attribHandles.front().ref;
    const std::uint32_t last = insert.attribHandles.back().ref;
    if (first == DRW::NoHandle || last == DRW::NoHandle)
      return false;
    if (first == last)
      return handle == first;
    return handle != DRW::NoHandle;
  }
  for (const dwgHandle &expected : insert.attribHandles) {
    if (expected.ref == handle)
      return true;
  }
  return false;
}

bool isExpectedAttribute(const DRW_Insert &insert, const DRW_Attrib &attribute,
                         DRW::Version version) {
  return attribute.parentHandle == insert.handle &&
         isExpectedAttribute(insert, attribute.handle, version);
}

bool hasCompleteAttributeList(const DRW_Insert &insert, DRW::Version version) {
  if (version < DRW::AC1018 && insert.attribHandles.size() == 2) {
    const std::uint32_t first = insert.attribHandles.front().ref;
    const std::uint32_t last = insert.attribHandles.back().ref;
    if (first == DRW::NoHandle || last == DRW::NoHandle)
      return first == last && insert.attlist.empty();
    if (first == last) {
      return insert.attlist.size() == 1 && insert.attlist.front() &&
             insert.attlist.front()->handle == first;
    }

    bool hasFirst = false;
    bool hasLast = false;
    for (std::size_t index = 0; index < insert.attlist.size(); ++index) {
      const auto &attrib = insert.attlist[index];
      if (!attrib)
        return false;
      for (std::size_t previous = 0; previous < index; ++previous) {
        if (insert.attlist[previous] &&
            insert.attlist[previous]->handle == attrib->handle) {
          return false;
        }
      }
      hasFirst = hasFirst || attrib->handle == first;
      hasLast = hasLast || attrib->handle == last;
    }
    return hasFirst && hasLast;
  }
  if (insert.attlist.size() != insert.attribHandles.size())
    return false;

  for (const dwgHandle &handle : insert.attribHandles) {
    if (handle.ref == DRW::NoHandle)
      return false;
    std::size_t matches = 0;
    for (const auto &attrib : insert.attlist) {
      if (!attrib)
        return false;
      if (attrib->handle == handle.ref)
        ++matches;
    }
    if (matches != 1u)
      return false;
  }
  return true;
}

// Minimal concrete entity whose only job is to run DRW_Entity's
// class-agnostic common-prologue parser (handle/EED/graphData/layer/…).
// Raw-net custom entities (STDPART2D, AEC_*) are emitted byte-for-byte by
// makeRawEntity and never parsed, so their proxyGraphics is empty; this host
// lets us lift the cached graphData bytes without modelling the unknown
// class body.  parseDwg runs only the common DATA prologue (it does NOT read
// the handle stream), which is exactly the section that carries graphData.
struct ProxyHostEntity : public DRW_Entity {
  void applyExtrusion() override {}
  bool parseDwg(DRW::Version v, dwgBuffer *b, std::uint32_t bsz = 0) override {
    return DRW_Entity::parseDwg(v, b, nullptr, bsz) && b != nullptr &&
           b->isGood();
  }
  bool hasOwnerHandle() const noexcept { return ownerHandle; }
};

// The R2007 dynamic-block parameter/grip range has no typed body parser,
// but each record still has the standard entity prologue and handle tail.
// Validate both before publishing its raw bytes for same-version replay.
struct RawEntityShell final : public DRW_Entity {
  void applyExtrusion() override {}
  bool parseDwg(DRW::Version v, dwgBuffer *b, std::uint32_t bsz = 0) override {
    return DRW_Entity::parseDwg(v, b, nullptr, bsz) &&
           DRW_Entity::parseDwgEntHandle(v, b);
  }
};

// Run only the common OBJECTS header parser for an opaque carrier.  The
// AC1027 DataStorage bit belongs to that header, so keeping this probe on
// DRW_TableEntry avoids a second, drift-prone bit layout implementation.
struct RawObjectHeaderProbe final : public DRW_TableEntry {
  bool parseCommon(DRW::Version version, dwgBuffer *buffer,
                   std::uint32_t bodyBitSize) {
    return DRW_TableEntry::parseDwg(version, buffer, nullptr, bodyBitSize);
  }

protected:
  bool parseDwg(DRW::Version version, dwgBuffer *buffer,
                std::uint32_t bodyBitSize = 0) override {
    return parseCommon(version, buffer, bodyBitSize);
  }
};

// Fixed opaque OBJECTS records still require the standard header and
// handle tail. Preserve only records that satisfy both framing contracts.
struct RawObjectShell final : public DRW_TableEntry {
  bool parseDwg(DRW::Version v, dwgBuffer *b, std::uint32_t bsz = 0) override {
    if (b == nullptr)
      return false;
    dwgBuffer strings = b->forkIndependent();
    return DRW_TableEntry::parseDwg(v, b, v > DRW::AC1018 ? &strings : nullptr,
                                    bsz) &&
           DRW_TableEntry::parseDwgCommonHandleData(v, b);
  }
};

bool rawObjectHasDataStorage(DRW::Version version,
                             std::vector<std::uint8_t> &bytes,
                             std::uint32_t bodyBitSize,
                             DRW_TextCodec *decoder) {
  if (version <= DRW::AC1024 || bytes.empty() || decoder == nullptr)
    return false;

  dwgBuffer probeBuffer(bytes.data(), bytes.size(), decoder);
  RawObjectHeaderProbe probe;
  if (!probe.parseCommon(version, &probeBuffer, bodyBitSize) ||
      !probeBuffer.isGood())
    return false;
  return probe.hasDataStorageBinaryData();
}

std::string normalizeDwgClassToken(const std::string &value) {
  std::string token;
  token.reserve(value.size());
  for (unsigned char ch : value) {
    if (std::isalnum(ch))
      token.push_back(static_cast<char>(std::toupper(ch)));
  }
  if (token.rfind("ACDB", 0) == 0)
    token.erase(0, 4);
  const std::string suffix = "CLASS";
  if (token.size() >= suffix.size() &&
      token.compare(token.size() - suffix.size(), suffix.size(), suffix) == 0) {
    token.erase(token.size() - suffix.size());
  }
  return token;
}

bool isValidatedRawCustomObjectShell(const DRW_Class *objectClass) {
  if (objectClass == nullptr)
    return false;
  const std::string recordName = normalizeDwgClassToken(objectClass->recName);
  const std::string className = normalizeDwgClassToken(objectClass->className);
  return recordName == "LINERES" || recordName == "CIRCARCRES" ||
         recordName == "TABLETEMPLATE" || className == "LINERES" ||
         className == "CIRCARCRES" || className == "TABLETEMPLATE";
}

bool isCenterLineActionBodyClass(const DRW_Class *objectClass) {
  if (objectClass == nullptr)
    return false;
  return normalizeDwgClassToken(objectClass->recName) ==
             "CENTERLINEACTIONBODY" ||
         normalizeDwgClassToken(objectClass->className) ==
             "CENTERLINEACTIONBODY";
}

bool objectContextKindFromClassNames(const std::string &recName,
                                     const std::string &className,
                                     DRW_ObjectContextData::Kind &kind) {
  const std::string rn = normalizeDwgClassToken(recName);
  const std::string cn = normalizeDwgClassToken(className);
  const auto matches = [&](const char *compact, const char *verbose = nullptr) {
    return rn == compact || cn == compact ||
           (verbose != nullptr && (rn == verbose || cn == verbose));
  };

  if (matches("ANNOTSCALEOBJECTCONTEXTDATA",
              "ANNOTATIONSCALEOBJECTCONTEXTDATA")) {
    kind = DRW_ObjectContextData::Kind::AnnotScale;
    return true;
  }
  if (matches("TEXTOBJECTCONTEXTDATA")) {
    kind = DRW_ObjectContextData::Kind::Text;
    return true;
  }
  if (matches("MTEXTOBJECTCONTEXTDATA")) {
    kind = DRW_ObjectContextData::Kind::MText;
    return true;
  }
  if (matches("MTEXTATTRIBUTEOBJECTCONTEXTDATA")) {
    kind = DRW_ObjectContextData::Kind::MTextAttribute;
    return true;
  }
  if (matches("ORDDIMOBJECTCONTEXTDATA",
              "ORDINATEDIMENSIONOBJECTCONTEXTDATA")) {
    kind = DRW_ObjectContextData::Kind::OrdinateDimension;
    return true;
  }
  if (matches("ALDIMOBJECTCONTEXTDATA", "ALIGNEDDIMENSIONOBJECTCONTEXTDATA")) {
    kind = DRW_ObjectContextData::Kind::AlignedDimension;
    return true;
  }
  if (matches("ANGDIMOBJECTCONTEXTDATA", "ANGULARDIMENSIONOBJECTCONTEXTDATA")) {
    kind = DRW_ObjectContextData::Kind::AngularDimension;
    return true;
  }
  if (matches("RADIMOBJECTCONTEXTDATA", "RADIALDIMENSIONOBJECTCONTEXTDATA")) {
    kind = DRW_ObjectContextData::Kind::RadialDimension;
    return true;
  }
  if (matches("RADIMLGOBJECTCONTEXTDATA",
              "LARGERADIALDIMENSIONOBJECTCONTEXTDATA")) {
    kind = DRW_ObjectContextData::Kind::LargeRadialDimension;
    return true;
  }
  if (matches("DMDIMOBJECTCONTEXTDATA",
              "DIAMETRICDIMENSIONOBJECTCONTEXTDATA")) {
    kind = DRW_ObjectContextData::Kind::DiameterDimension;
    return true;
  }
  if (matches("LEADEROBJECTCONTEXTDATA")) {
    kind = DRW_ObjectContextData::Kind::Leader;
    return true;
  }
  if (matches("BLKREFOBJECTCONTEXTDATA", "BLOCKREFERENCEOBJECTCONTEXTDATA")) {
    kind = DRW_ObjectContextData::Kind::BlockReference;
    return true;
  }
  if (matches("FCFOBJECTCONTEXTDATA")) {
    kind = DRW_ObjectContextData::Kind::Fcf;
    return true;
  }
  if (matches("MLEADEROBJECTCONTEXTDATA")) {
    kind = DRW_ObjectContextData::Kind::MLeader;
    return true;
  }

  return false;
}
} // namespace

bool dwgReader::hasPendingCompoundStateForBlock(
    const DRW_Block_Record &block) const {
  const auto containsEntity = [&block](std::uint32_t handle) {
    return std::find(block.entMap.cbegin(), block.entMap.cend(), handle) !=
           block.entMap.cend();
  };
  const auto belongsToBlock = [&block](const DRW_Entity &entity) {
    return std::find(block.entMap.cbegin(), block.entMap.cend(),
                     entity.handle) != block.entMap.cend() ||
           entity.parentHandle == block.handle ||
           (entity.parentHandle == DRW::NoHandle &&
            isSpaceBlockRecordName(block.name));
  };
  return std::any_of(m_pendingInsertStates.cbegin(),
                     m_pendingInsertStates.cend(),
                     [&belongsToBlock](const auto &item) {
                       return belongsToBlock(item.second.entity);
                     }) ||
         std::any_of(m_pendingPolylineStates.cbegin(),
                     m_pendingPolylineStates.cend(),
                     [&belongsToBlock](const auto &item) {
                       return belongsToBlock(item.second.entity);
                     }) ||
         std::any_of(
             m_orphanAttribStates.cbegin(), m_orphanAttribStates.cend(),
             [&containsEntity](const auto &item) {
               return std::any_of(
                   item.second.attributes.cbegin(),
                   item.second.attributes.cend(),
                   [&containsEntity](const StagedAttribState &attribute) {
                     return attribute.entity != nullptr &&
                            containsEntity(attribute.entity->handle);
                   });
             }) ||
         std::any_of(m_orphanPolylineVertexStates.cbegin(),
                     m_orphanPolylineVertexStates.cend(),
                     [&containsEntity](const auto &item) {
                       return std::any_of(
                           item.second.vertices.cbegin(),
                           item.second.vertices.cend(),
                           [&containsEntity](const StagedVertexState &vertex) {
                             return containsEntity(vertex.entity.handle);
                           });
                     }) ||
         std::any_of(m_stagedSeqEnds.cbegin(), m_stagedSeqEnds.cend(),
                     [&containsEntity](const auto &item) {
                       return containsEntity(item.first);
                     });
}

bool readDwgClassStringFooter(dwgBuffer &buffer, std::uint64_t footerEndBit,
                              std::uint64_t &stringStartBit,
                              std::uint64_t &stringSize) {
  const std::uint64_t savedPosition = buffer.getPosition();
  const std::uint8_t savedBitPos = buffer.getBitPos();
  std::uint64_t totalBits = 0;
  if (!dwgSafety::multiply(buffer.size(), 8, totalBits) ||
      footerEndBit >= totalBits)
    return false;

  auto restore = [&]() {
    buffer.setPosition(savedPosition);
    buffer.setBitPos(savedBitPos);
  };
  auto seekBits = [&](std::uint64_t bitPosition) {
    if (bitPosition >= totalBits || !buffer.setPosition(bitPosition >> 3))
      return false;
    const auto bitPos = static_cast<std::uint8_t>(bitPosition & 7);
    buffer.setBitPos(bitPos);
    return buffer.isGood() && buffer.getPosition() == (bitPosition >> 3) &&
           buffer.getBitPos() == bitPos;
  };

  std::uint64_t cursor = footerEndBit;
  if (!seekBits(cursor)) {
    restore();
    return false;
  }
  buffer.getBit(); // end-bit field
  if (!buffer.isGood() || cursor < 16) {
    restore();
    return false;
  }
  cursor -= 16;
  if (!seekBits(cursor)) {
    restore();
    return false;
  }
  std::uint64_t encodedSize = buffer.getRawShort16();
  if (!buffer.isGood()) {
    restore();
    return false;
  }
  if ((encodedSize & 0x8000U) != 0) {
    encodedSize &= 0x7FFFU;
    if (cursor < 16) {
      restore();
      return false;
    }
    cursor -= 16;
    if (!seekBits(cursor)) {
      restore();
      return false;
    }
    const std::uint64_t highSize = buffer.getRawShort16();
    if (!buffer.isGood()) {
      restore();
      return false;
    }
    encodedSize |= highSize << 15;
  }
  if (encodedSize > cursor) {
    restore();
    return false;
  }
  cursor -= encodedSize;
  if (!seekBits(cursor)) {
    restore();
    return false;
  }
  stringStartBit = cursor;
  stringSize = encodedSize;
  return true;
}

// DWG file-header codepage id -> DRW_TextCodec ANSI name (libreDWG
// codepages.h:35-82). Only codec-recognized names are mapped; unknown/rare ids
// (UTF-16, Johab, CP866, US-ASCII, ...) return nullptr so the caller keeps the
// ANSI_1252 default. 31 (GB2312) maps to its CP936 superset.
const char *dwgCodePageName(std::uint16_t cp) {
  switch (cp) {
  case dwgCP::ANSI_1250:
    return "ANSI_1250";
  case dwgCP::ANSI_1251:
    return "ANSI_1251";
  case dwgCP::ANSI_1252:
    return "ANSI_1252";
  case dwgCP::GBK_CP936:
    return "ANSI_936";
  case dwgCP::ANSI_1253:
    return "ANSI_1253";
  case dwgCP::ANSI_1254:
    return "ANSI_1254";
  case dwgCP::ANSI_1255:
    return "ANSI_1255";
  case dwgCP::ANSI_1256:
    return "ANSI_1256";
  case dwgCP::ANSI_1257:
    return "ANSI_1257";
  case dwgCP::ANSI_874:
    return "ANSI_874";
  case dwgCP::SHIFT_JIS:
    return "ANSI_932";
  case dwgCP::GBK:
    return "ANSI_936";
  case dwgCP::KOREAN_WANSUNG:
    return "ANSI_949";
  case dwgCP::BIG5:
    return "ANSI_950";
  case dwgCP::ANSI_1258:
    return "ANSI_1258";
  default:
    return nullptr;
  }
}

std::uint16_t dwgCodePageId(const char *name) {
  if (name == nullptr)
    return 30;
  // Round-trip set: map back exactly the names dwgCodePageName() emits.
  // 31 (GB2312) and 39 both resolve to "ANSI_936"; pick 39 (Simplified
  // Chinese / GBK superset) for the inverse direction.
  const std::string n(name);
  if (n == "ANSI_1250")
    return 28;
  if (n == "ANSI_1251")
    return 29;
  if (n == "ANSI_1252")
    return 30;
  if (n == "ANSI_1253")
    return 32;
  if (n == "ANSI_1254")
    return 33;
  if (n == "ANSI_1255")
    return 34;
  if (n == "ANSI_1256")
    return 35;
  if (n == "ANSI_1257")
    return 36;
  if (n == "ANSI_874")
    return 37;
  if (n == "ANSI_932")
    return 38;
  if (n == "ANSI_936")
    return 39;
  if (n == "ANSI_949")
    return 40;
  if (n == "ANSI_950")
    return 41;
  if (n == "ANSI_1258")
    return 44;
  return 30; // fallback
}

std::string decodeEedString(std::uint16_t cp, const std::string &raw,
                            DRW_TextCodec *fallback) {
  if (raw.empty())
    return std::string{};
  if (const char *name = dwgCodePageName(cp)) {
    // Build an AC1015-bound codec so setCodePage() selects the table
    // converter for `name` (the AC1021+ branch would pick UTF-16 instead).
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1015, /*dxfFormat=*/false);
    codec.setCodePage(name, /*dxfFormat=*/false);
    return codec.toUtf8(raw);
  }
  return fallback ? fallback->toUtf8(raw) : raw;
}

namespace {

void appendUtf8(std::uint32_t codePoint, std::string &output) {
  if (codePoint <= 0x7FU) {
    output.push_back(static_cast<char>(codePoint));
  } else if (codePoint <= 0x7FFU) {
    output.push_back(static_cast<char>(0xC0U | (codePoint >> 6)));
    output.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
  } else if (codePoint <= 0xFFFFU) {
    output.push_back(static_cast<char>(0xE0U | (codePoint >> 12)));
    output.push_back(static_cast<char>(0x80U | ((codePoint >> 6) & 0x3FU)));
    output.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
  } else {
    output.push_back(static_cast<char>(0xF0U | (codePoint >> 18)));
    output.push_back(static_cast<char>(0x80U | ((codePoint >> 12) & 0x3FU)));
    output.push_back(static_cast<char>(0x80U | ((codePoint >> 6) & 0x3FU)));
    output.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
  }
}

bool decodeEedUtf16(const std::vector<std::uint8_t> &bytes,
                    std::string &output) {
  if ((bytes.size() & 1U) != 0)
    return false;

  std::string decoded;
  if (bytes.size() >
          static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
      !DRW::reserve(decoded, static_cast<int>(bytes.size())))
    return false;
  for (std::size_t i = 0; i < bytes.size(); i += 2) {
    const std::uint16_t unit = static_cast<std::uint16_t>(bytes[i]) |
                               (static_cast<std::uint16_t>(bytes[i + 1]) << 8);
    std::uint32_t codePoint = unit;
    if (unit >= 0xD800U && unit <= 0xDBFFU) {
      if (i + 3 >= bytes.size())
        return false;
      const std::uint16_t low = static_cast<std::uint16_t>(bytes[i + 2]) |
                                (static_cast<std::uint16_t>(bytes[i + 3]) << 8);
      if (low < 0xDC00U || low > 0xDFFFU)
        return false;
      codePoint = 0x10000U +
                  ((static_cast<std::uint32_t>(unit) - 0xD800U) << 10) +
                  (static_cast<std::uint32_t>(low) - 0xDC00U);
      i += 2;
    } else if (unit >= 0xDC00U && unit <= 0xDFFFU) {
      return false;
    }
    appendUtf8(codePoint, decoded);
  }
  output = std::move(decoded);
  return true;
}

bool readEedRawHandle(dwgBuffer &buffer, std::uint64_t &ref) {
  std::uint8_t bytes[8]{};
  if (!buffer.getBytes(bytes, sizeof(bytes)))
    return false;
  ref = 0;
  for (const std::uint8_t byte : bytes)
    ref = (ref << 8) | byte;
  return true;
}

bool readEedRawHandleLE(dwgBuffer &buffer, std::uint64_t &ref) {
  std::uint8_t bytes[8]{};
  if (!buffer.getBytes(bytes, sizeof(bytes)))
    return false;
  ref = 0;
  for (std::size_t index = 0; index < sizeof(bytes); ++index)
    ref |= static_cast<std::uint64_t>(bytes[index]) << (index * 8U);
  return true;
}

std::uint64_t eedCurrentBit(const dwgBuffer &buffer) {
  return buffer.getPosition() * 8u + buffer.getBitPos();
}

bool eedHasBits(const dwgBuffer &buffer, std::uint64_t endBit,
                std::uint64_t count) {
  const std::uint64_t current = eedCurrentBit(buffer);
  return current <= endBit && count <= endBit - current;
}

bool readEedBitShort(dwgBuffer &buffer, std::uint64_t endBit,
                     std::uint16_t &value) {
  if (!eedHasBits(buffer, endBit, 2))
    return false;
  dwgBuffer probe = buffer.forkIndependent();
  const std::uint8_t selector = probe.get2Bits();
  if (!probe.isGood())
    return false;
  const std::uint64_t bits = selector == 0 ? 18u : selector == 1 ? 10u : 2u;
  if (!eedHasBits(buffer, endBit, bits))
    return false;
  value = buffer.getBitShort();
  return buffer.isGood();
}

bool readEedHandle(dwgBuffer &buffer, std::uint64_t endBit, dwgHandle &value) {
  if (!eedHasBits(buffer, endBit, 8))
    return false;
  dwgBuffer probe = buffer.forkIndependent();
  const dwgHandle parsed = probe.getHandle();
  if (!probe.isGood() || eedCurrentBit(probe) > endBit)
    return false;
  buffer = probe;
  value = parsed;
  return true;
}

bool readEedBytes(dwgBuffer &buffer, std::uint64_t endBit, std::uint8_t *data,
                  std::size_t size) {
  std::uint64_t bits = 0;
  if (!dwgSafety::multiply(static_cast<std::uint64_t>(size), 8, bits) ||
      !eedHasBits(buffer, endBit, bits))
    return false;
  return size == 0 || (buffer.getBytes(data, size) && buffer.isGood());
}

} // namespace

namespace {

template <typename... Args>
bool appendEedItem(DwgEedChunk &chunk, std::uint32_t &totalItems,
                   Args &&...args) {
  if (chunk.items.size() >= dwgSafety::MaxEedItems ||
      totalItems >= dwgSafety::MaxEedTotalItems)
    return false;
  try {
    chunk.items.emplace_back(std::forward<Args>(args)...);
  } catch (...) {
    return false;
  }
  ++totalItems;
  return true;
}

} // namespace

bool readDwgEed(DRW::Version version, dwgBuffer &buffer,
                std::vector<DwgEedChunk> &chunks, std::uint64_t endBit) {
  try {
    std::vector<DwgEedChunk> staged;
    std::uint32_t totalItems = 0;
    while (true) {
      std::uint16_t dataSize = 0;
      if (!readEedBitShort(buffer, endBit, dataSize))
        return false;
      if (dataSize == 0)
        break;
      if (staged.size() >= dwgSafety::MaxEedChunks)
        return false;

      dwgHandle appHandle;
      if (!readEedHandle(buffer, endBit, appHandle))
        return false;
      std::vector<std::uint8_t> data;
      if (!DRW::resize(data, static_cast<int>(dataSize)))
        return false;
      if (!readEedBytes(buffer, endBit, data.data(), data.size()))
        return false;

      DwgEedChunk chunk;
      chunk.appHandle = appHandle.ref;
      dwgBuffer itemBuffer(data.data(), data.size(), buffer.decoder);
      while (itemBuffer.numRemainingBytes() > 0) {
        const std::uint8_t code = itemBuffer.getRawChar8();
        if (!itemBuffer.isGood())
          return false;

        switch (code) {
        case 0: {
          std::string value;
          if (version > DRW::AC1018) {
            if (itemBuffer.numRemainingBytes() < 2)
              return false;
            const std::uint16_t charCount = itemBuffer.getRawShort16();
            const std::uint64_t byteCount =
                static_cast<std::uint64_t>(charCount) * 2U;
            if (!itemBuffer.isGood() ||
                byteCount >
                    static_cast<std::uint64_t>(itemBuffer.numRemainingBytes()))
              return false;
            std::vector<std::uint8_t> bytes;
            if (!DRW::resize(bytes, static_cast<int>(byteCount)))
              return false;
            if (!itemBuffer.getBytes(bytes.data(), bytes.size()) ||
                !decodeEedUtf16(bytes, value))
              return false;
          } else {
            if (itemBuffer.numRemainingBytes() < 3)
              return false;
            const std::uint8_t length = itemBuffer.getRawChar8();
            const std::uint16_t codePage = itemBuffer.getBERawShort16();
            if (!itemBuffer.isGood() || length > itemBuffer.numRemainingBytes())
              return false;
            std::string raw;
            if (!DRW::resize(raw, static_cast<int>(length)))
              return false;
            if (length > 0 &&
                !itemBuffer.getBytes(
                    reinterpret_cast<std::uint8_t *>(raw.data()), length))
              return false;
            value = decodeEedString(codePage, raw, itemBuffer.decoder);
          }
          if (!appendEedItem(chunk, totalItems, 1000, std::move(value)))
            return false;
          break;
        }
        case 2: {
          if (itemBuffer.numRemainingBytes() < 1)
            return false;
          const std::uint8_t control = itemBuffer.getRawChar8();
          if (!itemBuffer.isGood() || control > 1)
            return false;
          if (!appendEedItem(chunk, totalItems, 1002,
                             std::string(control == 0 ? "{" : "}")))
            return false;
          break;
        }
        case 3: {
          if (itemBuffer.numRemainingBytes() < 8)
            return false;
          std::uint64_t ref = 0;
          if (!readEedRawHandleLE(itemBuffer, ref))
            return false;
          const std::size_t index = chunk.items.size();
          char text[24]{};
          std::snprintf(text, sizeof(text), "%llX",
                        static_cast<unsigned long long>(ref));
          if (!appendEedItem(chunk, totalItems, 1003, std::string{text}, true))
            return false;
          chunk.items.back().setDwgRawLayerReference(ref, version);
          // EED layer references are always eight bytes on disk. Keep
          // a valid wide reference even though the in-memory layer map
          // is currently keyed by 32-bit DWG handles.
          if (ref <= std::numeric_limits<std::uint32_t>::max())
            chunk.layerRefs.push_back({index, static_cast<std::uint32_t>(ref)});
          break;
        }
        case 4: {
          if (itemBuffer.numRemainingBytes() < 1)
            return false;
          const std::uint8_t length = itemBuffer.getRawChar8();
          if (!itemBuffer.isGood() || length > itemBuffer.numRemainingBytes())
            return false;
          std::vector<std::uint8_t> value;
          if (!DRW::resize(value, static_cast<int>(length)))
            return false;
          if (length > 0 && !itemBuffer.getBytes(value.data(), value.size()))
            return false;
          if (!appendEedItem(chunk, totalItems, 1004, std::move(value)))
            return false;
          break;
        }
        case 5: {
          if (itemBuffer.numRemainingBytes() < 8)
            return false;
          std::uint64_t ref = 0;
          if (!readEedRawHandle(itemBuffer, ref))
            return false;
          char text[24]{};
          std::snprintf(text, sizeof(text), "%llX",
                        static_cast<unsigned long long>(ref));
          if (!appendEedItem(chunk, totalItems, 1005, std::string{text}))
            return false;
          break;
        }
        case 10:
        case 11:
        case 12:
        case 13: {
          if (itemBuffer.numRemainingBytes() < 24)
            return false;
          DRW_Coord value;
          value.x = itemBuffer.getRawDouble();
          value.y = itemBuffer.getRawDouble();
          value.z = itemBuffer.getRawDouble();
          if (!itemBuffer.isGood())
            return false;
          if (!appendEedItem(chunk, totalItems, 1000 + code, value))
            return false;
          break;
        }
        case 40:
        case 41:
        case 42: {
          if (itemBuffer.numRemainingBytes() < 8)
            return false;
          const double value = itemBuffer.getRawDouble();
          if (!itemBuffer.isGood())
            return false;
          if (!appendEedItem(chunk, totalItems, 1000 + code, value))
            return false;
          break;
        }
        case 70: {
          if (itemBuffer.numRemainingBytes() < 2)
            return false;
          const auto value =
              static_cast<std::int16_t>(itemBuffer.getRawShort16());
          if (!itemBuffer.isGood())
            return false;
          if (!appendEedItem(chunk, totalItems, 1070,
                             static_cast<std::int32_t>(value)))
            return false;
          break;
        }
        case 71: {
          if (itemBuffer.numRemainingBytes() < 4)
            return false;
          const auto value =
              static_cast<std::int32_t>(itemBuffer.getRawLong32());
          if (!itemBuffer.isGood())
            return false;
          if (!appendEedItem(chunk, totalItems, 1071, value))
            return false;
          break;
        }
        default:
          return false;
        }
      }
      if (!itemBuffer.isGood() || itemBuffer.numRemainingBytes() != 0)
        return false;
      staged.push_back(std::move(chunk));
    }
    chunks = std::move(staged);
    return true;
  } catch (...) {
    return false;
  }
}

bool readDwgHandleChecked(dwgBuffer &buffer, std::uint32_t baseHandle,
                          bool offset, dwgHandle &handle) {
  if (!buffer.isGood())
    return false;
  const dwgHandle value =
      offset ? buffer.getOffsetHandle(baseHandle) : buffer.getHandle();
  if (!buffer.isGood())
    return false;
  handle = value;
  return true;
}

bool readDwgHandleList(dwgBuffer &buffer, std::uint32_t baseHandle,
                       std::int32_t count, bool offset,
                       std::vector<std::uint32_t> *refs) {
  if (!dwgSafety::validReactorCount(count)) {
    buffer.invalidate();
    return false;
  }
  const int remaining = buffer.numRemainingBytes();
  if (remaining < 0 || static_cast<std::uint32_t>(count) >
                           static_cast<std::uint32_t>(remaining)) {
    buffer.invalidate();
    return false;
  }

  std::vector<std::uint32_t> staged;
  if (refs != nullptr && !DRW::reserve(staged, count))
    return false;
  for (std::int32_t i = 0; i < count; ++i) {
    dwgHandle value;
    if (!readDwgHandleChecked(buffer, baseHandle, offset, value))
      return false;
    if (refs != nullptr)
      staged.push_back(value.ref);
  }
  if (refs != nullptr)
    *refs = std::move(staged);
  return true;
}

bool dwgReader::stageActiveEntityFrame(
    DwgStagedFrame &frame, const DRW_DwgFramePublication &publication) {
  if (frame.lease.has_value() || frame.publication.has_value() ||
      m_activeEntityFrameLease == nullptr) {
    return reportDwgFrameTransitionFailure(
        DwgSourceFrameId{publication.m_handle});
  }

  DwgFrameMapLease &activeLease = *m_activeEntityFrameLease;
  if (!activeLease.isDetached() ||
      activeLease.origin != DwgFrameMapLease::Origin::ObjectMap ||
      activeLease.object.handle != activeLease.source.handle ||
      publication.m_handle != activeLease.object.handle) {
    return reportDwgFrameTransitionFailure(activeLease.source,
                                           activeLease.object.loc, true);
  }
  if (activeLease.hasCoverage) {
    const DwgSourceFrameId publicationSource{
        publication.m_handle, publication.m_sourceOffset,
        publication.m_sourceMapOrdinal, publication.m_sourceOffsetSpace};
    if (!publication.m_hasSourceLocation ||
        !(activeLease.source == publicationSource)) {
      return reportDwgFrameTransitionFailure(activeLease.source,
                                             activeLease.object.loc, true);
    }
  }
  if (!stageDetachedDwgSourceFrame(activeLease))
    return false;

  frame.lease.emplace(std::move(activeLease));
  if (!frame.lease->hasCoverage)
    return true;
  if (stageCurrentEntityFrame(frame, publication))
    return true;

  // Attaching the receipt is the only allocation after the source node has
  // been staged. Preserve recoverability when it fails.
  if (!restoreStagedFrame(frame))
    (void)abandonStagedFrame(frame);
  return false;
}

bool dwgReader::claimInvalidSeqEndTerminalizerMarker(std::uint32_t handle,
                                                     bool &inserted) noexcept {
  inserted = false;
  if (handle == DRW::NoHandle ||
      consumeTerminalizerFailurePointForTest(
          DwgTerminalizerFailurePoint::BeforeSeqEndMarker)) {
    return false;
  }
  try {
    inserted = m_invalidSeqEndHandles.insert(handle).second;
    return true;
  } catch (...) {
    return false;
  }
}

bool dwgReader::claimInvalidInsertOwnerTerminalizerMarker(
    std::uint32_t owner, DwgTerminalizerFailurePoint failurePoint,
    bool &inserted) noexcept {
  inserted = false;
  if (owner == DRW::NoHandle ||
      consumeTerminalizerFailurePointForTest(failurePoint)) {
    return false;
  }
  try {
    inserted = m_invalidInsertOwners.insert(owner).second;
    return true;
  } catch (...) {
    return false;
  }
}

void dwgReader::terminalizeInsertGroup(std::uint32_t handle,
                                       DwgInsertTerminalReason reason) {
  const auto pendingIt = m_pendingInsertStates.find(handle);
  if (pendingIt == m_pendingInsertStates.end())
    return;

  const auto coverageReason = [reason]() {
    switch (reason) {
    case DwgInsertTerminalReason::CallbackException:
      return DRW_DwgFrameCoverageReason::CallbackException;
    case DwgInsertTerminalReason::ReceiptFailure:
      return DRW_DwgFrameCoverageReason::ReceiptFailure;
    case DwgInsertTerminalReason::MalformedGroup:
    default:
      return DRW_DwgFrameCoverageReason::None;
    }
  }();
  const auto terminalizeFrame = [this, coverageReason](DwgStagedFrame &frame) {
    if (coverageReason != DRW_DwgFrameCoverageReason::None &&
        frame.lease.has_value() && frame.hasDetachedLease() &&
        frame.lease->hasCoverage) {
      const auto sourceIt =
          m_dwgSourceFrameIndexes.find(frame.lease->source.handle);
      if (sourceIt != m_dwgSourceFrameIndexes.end() &&
          sourceIt->second < m_dwgSourceFrameLedger.size() &&
          m_dwgSourceFrameLedger[sourceIt->second].m_disposition ==
              DRW_DwgFrameDisposition::Staged) {
        (void)markDwgFrameOutcome(frame.lease->source,
                                  DRW_DwgFrameDisposition::Failed,
                                  coverageReason);
      }
    }
    return abandonStagedFrame(frame);
  };

  PendingInsertState &pending = pendingIt->second;
  const std::uint32_t sequenceHandle = pending.entity.seqendH.ref;
  auto sequenceIt = m_stagedSeqEnds.find(sequenceHandle);
  const bool hasOwnedSequence =
      sequenceIt != m_stagedSeqEnds.end() && sequenceIt->second.owner == handle;
  bool insertedSequenceMarker = false;
  if (hasOwnedSequence && !claimInvalidSeqEndTerminalizerMarker(
                              sequenceHandle, insertedSequenceMarker)) {
    return;
  }
  bool insertedOwnerMarker = false;
  if (!claimInvalidInsertOwnerTerminalizerMarker(
          handle, DwgTerminalizerFailurePoint::BeforeInsertOwnerMarker,
          insertedOwnerMarker)) {
    if (insertedSequenceMarker)
      m_invalidSeqEndHandles.erase(sequenceHandle);
    return;
  }

  bool complete = true;
  for (StagedAttribState &attribute : pending.attributes)
    complete = terminalizeFrame(attribute.frame) && complete;

  if (hasOwnedSequence) {
    const bool sequenceComplete = terminalizeFrame(sequenceIt->second.frame);
    complete = sequenceComplete && complete;
    if (sequenceComplete)
      m_stagedSeqEnds.erase(sequenceIt);
  }

  complete = terminalizeFrame(pending.frame) && complete;
  if (!complete)
    return;
  ++m_entityParseFailures;
  m_pendingInsertStates.erase(pendingIt);
}

void dwgReader::abandonPendingInsertState(std::uint32_t handle) {
  terminalizeInsertGroup(handle, DwgInsertTerminalReason::MalformedGroup);
}

void dwgReader::terminalizeOrphanAttribOwner(std::uint32_t owner) {
  if (owner == DRW::NoHandle)
    return;

  const auto orphanIt = m_orphanAttribStates.find(owner);
  bool insertedOwnerMarker = false;
  if (!claimInvalidInsertOwnerTerminalizerMarker(
          owner, DwgTerminalizerFailurePoint::BeforeOrphanOwnerMarker,
          insertedOwnerMarker)) {
    return;
  }
  bool complete = true;
  if (orphanIt != m_orphanAttribStates.end()) {
    for (StagedAttribState &attribute : orphanIt->second.attributes)
      complete = abandonStagedFrame(attribute.frame) && complete;
  }
  if (!complete)
    return;
  if (orphanIt != m_orphanAttribStates.end()) {
    ++m_entityParseFailures;
    m_orphanAttribStates.erase(orphanIt);
  }
}

void dwgReader::terminalizePendingPolylineState(
    std::uint32_t handle, DwgInsertTerminalReason reason) {
  const auto pendingIt = m_pendingPolylineStates.find(handle);
  if (pendingIt == m_pendingPolylineStates.end())
    return;

  bool markedInvalid = false;
  try {
    markedInvalid = m_invalidPolylineOwners.insert(handle).second;
  } catch (...) {
    return;
  }
  if (!markedInvalid)
    return;

  const auto coverageReason = [reason]() {
    switch (reason) {
    case DwgInsertTerminalReason::CallbackException:
      return DRW_DwgFrameCoverageReason::CallbackException;
    case DwgInsertTerminalReason::ReceiptFailure:
      return DRW_DwgFrameCoverageReason::ReceiptFailure;
    case DwgInsertTerminalReason::MalformedGroup:
    default:
      return DRW_DwgFrameCoverageReason::None;
    }
  }();
  const auto terminalizeFrame = [this, coverageReason](DwgStagedFrame &frame) {
    if (frame.lease.has_value() && frame.lease->hasCoverage &&
        coverageReason != DRW_DwgFrameCoverageReason::None) {
      (void)markDwgFrameOutcome(
          frame.lease->source, DRW_DwgFrameDisposition::Failed, coverageReason);
    }
    return abandonStagedFrame(frame);
  };

  const auto orphanIt = m_orphanPolylineVertexStates.find(handle);
  bool complete = true;
  for (StagedVertexState &vertex : pendingIt->second.vertices)
    complete = terminalizeFrame(vertex.frame) && complete;
  if (orphanIt != m_orphanPolylineVertexStates.end()) {
    for (StagedVertexState &vertex : orphanIt->second.vertices)
      complete = terminalizeFrame(vertex.frame) && complete;
  }
  const auto sequenceIt = std::find_if(
      m_stagedSeqEnds.begin(), m_stagedSeqEnds.end(),
      [handle](const auto &item) { return item.second.owner == handle; });
  if (sequenceIt != m_stagedSeqEnds.end()) {
    const bool sequenceComplete = terminalizeFrame(sequenceIt->second.frame);
    complete = sequenceComplete && complete;
    if (sequenceComplete)
      m_stagedSeqEnds.erase(sequenceIt);
  }
  complete = terminalizeFrame(pendingIt->second.frame) && complete;
  if (!complete)
    return;
  ++m_entityParseFailures;
  m_pendingPolylineStates.erase(pendingIt);
  if (orphanIt != m_orphanPolylineVertexStates.end())
    m_orphanPolylineVertexStates.erase(orphanIt);
}

void dwgReader::terminalizeOrphanPolylineVertexOwner(std::uint32_t owner) {
  const auto orphanIt = m_orphanPolylineVertexStates.find(owner);
  if (orphanIt == m_orphanPolylineVertexStates.end())
    return;
  try {
    if (!m_invalidPolylineOwners.insert(owner).second)
      return;
  } catch (...) {
    return;
  }
  bool complete = true;
  for (StagedVertexState &vertex : orphanIt->second.vertices)
    complete = abandonStagedFrame(vertex.frame) && complete;
  if (!complete)
    return;
  ++m_entityParseFailures;
  m_orphanPolylineVertexStates.erase(orphanIt);
}

dwgReader::DwgMappedEntityOutcome
dwgReader::stagePendingInsert(DRW_Insert &&insert,
                              const DRW_DwgFramePublication &publication,
                              DRW_Interface &intfa) {
  if (insert.handle == DRW::NoHandle || !insert.attlist.empty() ||
      m_invalidInsertOwners.find(insert.handle) !=
          m_invalidInsertOwners.end() ||
      m_pendingInsertStates.find(insert.handle) !=
          m_pendingInsertStates.end()) {
    (void)reportDwgFrameTransitionFailure(DwgSourceFrameId{insert.handle});
    return DwgMappedEntityOutcome::Rejected;
  }

  const auto orphanIt = m_orphanAttribStates.find(insert.handle);
  if (orphanIt != m_orphanAttribStates.end()) {
    std::vector<std::uint32_t> handles;
    try {
      handles.reserve(orphanIt->second.attributes.size());
    } catch (...) {
      terminalizeOrphanAttribOwner(insert.handle);
      return DwgMappedEntityOutcome::Rejected;
    }
    for (const StagedAttribState &attribute : orphanIt->second.attributes) {
      if (attribute.entity == nullptr ||
          !isExpectedAttribute(insert, *attribute.entity, version) ||
          std::find(handles.cbegin(), handles.cend(),
                    attribute.entity->handle) != handles.cend()) {
        terminalizeOrphanAttribOwner(insert.handle);
        (void)reportDwgFrameTransitionFailure(DwgSourceFrameId{insert.handle});
        return DwgMappedEntityOutcome::Rejected;
      }
      handles.push_back(attribute.entity->handle);
    }
  }

  PendingInsertState pending;
  pending.entity = std::move(insert);
  const std::uint32_t insertHandle = pending.entity.handle;
  const std::size_t adoptedAttributeCount =
      orphanIt == m_orphanAttribStates.end()
          ? 0u
          : orphanIt->second.attributes.size();
  try {
    m_pendingInsertStates.reserve(m_pendingInsertStates.size() + 1u);
    pending.entity.attlist.reserve(adoptedAttributeCount);
    pending.attributes.reserve(adoptedAttributeCount);
  } catch (...) {
    terminalizeOrphanAttribOwner(insertHandle);
    return DwgMappedEntityOutcome::Rejected;
  }
  if (!stageActiveEntityFrame(pending.frame, publication)) {
    terminalizeOrphanAttribOwner(insertHandle);
    return DwgMappedEntityOutcome::Rejected;
  }

  auto inserted = m_pendingInsertStates.end();
  try {
    const auto result = m_pendingInsertStates.emplace(pending.entity.handle,
                                                      std::move(pending));
    if (!result.second) {
      (void)restoreStagedFrame(pending.frame);
      terminalizeOrphanAttribOwner(insertHandle);
      (void)reportDwgFrameTransitionFailure(
          DwgSourceFrameId{publication.m_handle});
      return DwgMappedEntityOutcome::Rejected;
    }
    inserted = result.first;
  } catch (...) {
    (void)restoreStagedFrame(pending.frame);
    terminalizeOrphanAttribOwner(insertHandle);
    return DwgMappedEntityOutcome::Rejected;
  }

  if (orphanIt != m_orphanAttribStates.end()) {
    auto adoptedIt = m_orphanAttribStates.find(inserted->first);
    if (adoptedIt == m_orphanAttribStates.end()) {
      abandonPendingInsertState(inserted->first);
      return DwgMappedEntityOutcome::Rejected;
    }
    PendingInsertState &staged = inserted->second;
    for (StagedAttribState &attribute : adoptedIt->second.attributes) {
      staged.entity.attlist.push_back(attribute.entity);
      staged.attributes.push_back(std::move(attribute));
    }
    m_orphanAttribStates.erase(adoptedIt);
  }
  return tryCommitPendingInsert(inserted->first, intfa);
}

dwgReader::DwgMappedEntityOutcome dwgReader::stageMappedInsertAggregate(
    DRW_Insert &&insert, const DRW_DwgFramePublication &publication,
    dwgBuffer *dbuf, DRW_Interface &intfa,
    DwgIntegrityAddressSpace offsetSpace) {
  if (version < DRW::AC1018 || dbuf == nullptr ||
      m_activeEntityFrameLease == nullptr ||
      !m_activeEntityFrameLease->isDetached() ||
      m_activeEntityFrameLease->object.handle != insert.handle) {
    return DwgMappedEntityOutcome::Rejected;
  }

  const std::uint32_t insertHandle = insert.handle;
  std::vector<std::uint32_t> discoveredHandles;
  const auto reject = [this, insertHandle, &discoveredHandles]() {
    terminalizeInsertGroup(insertHandle,
                           DwgInsertTerminalReason::MalformedGroup);
    for (const std::uint32_t handle : discoveredHandles)
      (void)quarantineMappedDwgSourceFrame(handle);
    return DwgMappedEntityOutcome::Rejected;
  };

  try {
    discoveredHandles.reserve(insert.attribHandles.size() + 1u);
    std::vector<std::uint32_t> declaredAttributeHandles;
    declaredAttributeHandles.reserve(insert.attribHandles.size());
    std::unordered_set<std::uint32_t> declaredAttributes;
    declaredAttributes.reserve(insert.attribHandles.size());
    for (const dwgHandle &declared : insert.attribHandles) {
      const std::uint32_t handle = declared.ref;
      if (handle == DRW::NoHandle ||
          !declaredAttributes.insert(handle).second) {
        return reject();
      }
      declaredAttributeHandles.push_back(handle);

      const auto sourceIt = ObjectMap.find(handle);
      if (sourceIt == ObjectMap.end())
        continue;
      discoveredHandles.push_back(handle);
      DwgFrameClassification classification;
      if (!classifyDwgSourceFrame(dbuf, sourceIt->second, classification) ||
          classification.route != DwgFrameClassification::Route::Entity ||
          classification.resolvedType != dwgType::ATTRIB) {
        return reject();
      }
    }

    const std::uint32_t sequenceHandle = insert.seqendH.ref;
    if (sequenceHandle != DRW::NoHandle) {
      if (declaredAttributes.find(sequenceHandle) != declaredAttributes.end()) {
        return reject();
      }
      const auto sourceIt = ObjectMap.find(sequenceHandle);
      if (sourceIt != ObjectMap.end()) {
        discoveredHandles.push_back(sequenceHandle);
        DwgFrameClassification classification;
        if (!classifyDwgSourceFrame(dbuf, sourceIt->second, classification) ||
            classification.route != DwgFrameClassification::Route::Entity ||
            classification.resolvedType != dwgType::SEQEND) {
          return reject();
        }
      }
    }

    const DwgMappedEntityOutcome parentOutcome =
        stagePendingInsert(std::move(insert), publication, intfa);
    if (parentOutcome == DwgMappedEntityOutcome::Rejected)
      return reject();
    if (parentOutcome == DwgMappedEntityOutcome::CommittedCompound) {
      // A declared child frame still in ObjectMap would duplicate an
      // already committed child-first sequence.
      return discoveredHandles.empty() ? parentOutcome : reject();
    }

    const auto stageMappedChild = [this, dbuf, &intfa,
                                   offsetSpace](DwgFrameMapLease &lease) {
      DwgFrameMapLease *const previousLease = m_activeEntityFrameLease;
      m_activeEntityFrameLease = nullptr;
      bool frameFailure = false;
      bool read = false;
      try {
        read =
            readMappedDwgEntity(dbuf, lease, intfa, &frameFailure, offsetSpace);
      } catch (...) {
        read = false;
        frameFailure = true;
      }
      m_activeEntityFrameLease = previousLease;
      return read && !frameFailure;
    };
    const auto restoreUnstagedLease = [this](DwgFrameMapLease &lease) {
      if (lease.isDetached())
        (void)restoreDwgSourceFrame(lease);
    };
    const auto stageDeclaredChild =
        [this, dbuf, &stageMappedChild,
         &restoreUnstagedLease](std::uint32_t handle) {
          const auto sourceIt = ObjectMap.find(handle);
          if (sourceIt == ObjectMap.end())
            return true;
          DwgFrameClassification classification;
          if (!classifyDwgSourceFrame(dbuf, sourceIt->second, classification)) {
            return false;
          }
          DwgFrameMapLease lease;
          if (!detachDwgSourceFrame(ObjectMap, sourceIt, lease))
            return false;
          lease.classification.emplace(std::move(classification));
          if (stageMappedChild(lease))
            return true;
          restoreUnstagedLease(lease);
          return false;
        };

    // The INSERT handle list defines ATTRIB order. The set above is only
    // for duplicate detection; iterating it would reorder the callback.
    for (const std::uint32_t declared : declaredAttributeHandles) {
      if (!stageDeclaredChild(declared))
        return reject();
    }
    if (sequenceHandle != DRW::NoHandle &&
        !stageDeclaredChild(sequenceHandle)) {
      return reject();
    }

    // Staging the declared SEQEND may commit and remove the complete
    // group. A second commit probe would report it as merely unstaged.
    if (m_pendingInsertStates.find(insertHandle) ==
        m_pendingInsertStates.end()) {
      return DwgMappedEntityOutcome::CommittedCompound;
    }
    const DwgMappedEntityOutcome outcome =
        tryCommitPendingInsert(insertHandle, intfa);
    return outcome == DwgMappedEntityOutcome::StagedCompound ? reject()
                                                             : outcome;
  } catch (...) {
    return reject();
  }
}

dwgReader::DwgMappedEntityOutcome dwgReader::stageLegacyInsertAggregate(
    DRW_Insert &&insert, const DRW_DwgFramePublication &publication,
    dwgBuffer *dbuf, DRW_Interface &intfa,
    DwgIntegrityAddressSpace offsetSpace) {
  if (version >= DRW::AC1018 || dbuf == nullptr ||
      m_activeEntityFrameLease == nullptr ||
      !m_activeEntityFrameLease->isDetached() ||
      m_activeEntityFrameLease->object.handle != insert.handle) {
    return DwgMappedEntityOutcome::Rejected;
  }

  // R13-R2000 stores only the first and last ATTRIB handles. Those children
  // are outside the BLOCK_RECORD's normal next-entity chain, so follow their
  // own explicit (or implicit +1) chain before staging the parent.
  if (insert.attribHandles.empty())
    return stagePendingInsert(std::move(insert), publication, intfa);
  if (insert.attribHandles.size() != 2u ||
      insert.seqendH.ref == DRW::NoHandle) {
    return DwgMappedEntityOutcome::Rejected;
  }

  struct ParsedAttrib {
    objHandle object;
    std::shared_ptr<DRW_Attrib> entity;
    DRW_DwgFramePublication publication;
  };

  const std::uint32_t insertHandle = insert.handle;
  const std::uint32_t firstHandle = insert.attribHandles.front().ref;
  const std::uint32_t lastHandle = insert.attribHandles.back().ref;
  const std::uint32_t sequenceHandle = insert.seqendH.ref;
  const bool emptyAttributeRange = firstHandle == DRW::NoHandle &&
                                   lastHandle == DRW::NoHandle;
  if ((firstHandle == DRW::NoHandle) != (lastHandle == DRW::NoHandle))
    return DwgMappedEntityOutcome::Rejected;
  std::vector<ParsedAttrib> attributes;
  std::vector<std::uint32_t> discoveredHandles;
  std::unordered_set<std::uint32_t> visitedHandles;
  const auto quarantineDiscovered = [this, &discoveredHandles]() {
    for (const std::uint32_t handle : discoveredHandles)
      (void)quarantineMappedDwgSourceFrame(handle);
  };
  const auto reject = [this, insertHandle, &quarantineDiscovered]() {
    terminalizeInsertGroup(insertHandle, DwgInsertTerminalReason::MalformedGroup);
    quarantineDiscovered();
    return DwgMappedEntityOutcome::Rejected;
  };

  try {
    std::uint32_t nextHandle = firstHandle;
    bool reachedLast = emptyAttributeRange;
    while (nextHandle != DRW::NoHandle) {
      if (!visitedHandles.insert(nextHandle).second)
        return reject();
      discoveredHandles.push_back(nextHandle);

      const auto sourceIt = ObjectMap.find(nextHandle);
      if (sourceIt == ObjectMap.end())
        return reject();
      DwgSourceFrameLease borrowed;
      if (!borrowDwgSourceFrame(ObjectMap, sourceIt, borrowed) ||
          borrowed.object.handle != nextHandle) {
        return reject();
      }

      DwgObjectFrame frame;
      if (!frame.readAt(*dbuf, version, borrowed.object.loc)) {
        recordObjectFrameFailure(borrowed.object, offsetSpace);
        return reject();
      }
      std::vector<std::uint8_t> &body = frame.body();
      dwgBuffer buffer(body.data(), body.size(), &decoder);
      if (buffer.getObjType(version) != dwgType::ATTRIB || !buffer.isGood())
        return reject();
      buffer.resetPosition();

      auto attribute = std::make_shared<DRW_Attrib>();
      if (!attribute->parseDwg(version, &buffer, frame.bodyBitSize()) ||
          !buffer.isGood() || attribute->handle != nextHandle) {
        if (attribute->handle != nextHandle)
          parsedEntityHandleMismatch = true;
        return reject();
      }
      if (attribute->parentHandle != insertHandle) {
        parsedEntityOwnerMismatch = true;
        return reject();
      }
      parseAttribs(attribute.get());

      ParsedAttrib parsed;
      parsed.object = borrowed.object;
      parsed.entity = std::move(attribute);
      parsed.publication = makeTypedEntityFramePublication(
          version, borrowed.object, dwgType::ATTRIB, *parsed.entity);
      attributes.push_back(std::move(parsed));

      if (nextHandle == lastHandle) {
        reachedLast = true;
        break;
      }
      nextHandle = attributes.back().entity->nextEntLink;
    }
    if (!reachedLast)
      return reject();

    // Handle maps are visited by handle, so a valid legacy SEQEND can be
    // staged before its INSERT.  The empty-range form has no ATTRIB frames to
    // move, and the pending INSERT commit can consume that staged SEQEND
    // directly.
    const auto preStagedSequenceIt = m_stagedSeqEnds.find(sequenceHandle);
    if (preStagedSequenceIt != m_stagedSeqEnds.end()) {
      if (preStagedSequenceIt->second.owner != insertHandle ||
          !attributes.empty())
        return reject();
      const DwgMappedEntityOutcome outcome =
          stagePendingInsert(std::move(insert), publication, intfa);
      return outcome == DwgMappedEntityOutcome::CommittedCompound
                 ? outcome
                 : reject();
    }

    discoveredHandles.push_back(sequenceHandle);
    const auto sequenceIt = ObjectMap.find(sequenceHandle);
    if (sequenceIt == ObjectMap.end())
      return reject();
    DwgSourceFrameLease borrowedSequence;
    if (!borrowDwgSourceFrame(ObjectMap, sequenceIt, borrowedSequence) ||
        borrowedSequence.object.handle != sequenceHandle) {
      return reject();
    }
    DwgObjectFrame sequenceFrame;
    if (!sequenceFrame.readAt(*dbuf, version, borrowedSequence.object.loc)) {
      recordObjectFrameFailure(borrowedSequence.object, offsetSpace);
      return reject();
    }
    std::vector<std::uint8_t> &sequenceBody = sequenceFrame.body();
    dwgBuffer sequenceBuffer(sequenceBody.data(), sequenceBody.size(),
                             &decoder);
    if (sequenceBuffer.getObjType(version) != dwgType::SEQEND ||
        !sequenceBuffer.isGood()) {
      return reject();
    }
    sequenceBuffer.resetPosition();
    DRW_SeqEnd sequenceEnd;
    if (!sequenceEnd.parseDwg(version, &sequenceBuffer,
                              sequenceFrame.bodyBitSize()) ||
        !sequenceBuffer.isGood() || sequenceEnd.handle != sequenceHandle) {
      if (sequenceEnd.handle != sequenceHandle)
        parsedEntityHandleMismatch = true;
      return reject();
    }
    if (sequenceEnd.parentHandle != insertHandle) {
      parsedEntityOwnerMismatch = true;
      return reject();
    }
    const DRW_DwgFramePublication sequencePublication =
        makeTypedEntityFramePublication(version, borrowedSequence.object,
                                        dwgType::SEQEND, sequenceEnd);

    const DwgMappedEntityOutcome parentOutcome =
        stagePendingInsert(std::move(insert), publication, intfa);
    if (parentOutcome != DwgMappedEntityOutcome::StagedCompound)
      return reject();

    const auto stageWithLease = [this](DwgFrameMapLease &lease,
                                       const auto &operation) {
      DwgFrameMapLease *const previousLease = m_activeEntityFrameLease;
      m_activeEntityFrameLease = &lease;
      DwgMappedEntityOutcome outcome = DwgMappedEntityOutcome::Rejected;
      try {
        outcome = operation();
      } catch (...) {
        outcome = DwgMappedEntityOutcome::Rejected;
      }
      m_activeEntityFrameLease = previousLease;
      return outcome;
    };
    const auto restoreUnstagedLease = [this](DwgFrameMapLease &lease) {
      if (lease.isDetached())
        (void)restoreDwgSourceFrame(lease);
    };

    for (ParsedAttrib &attribute : attributes) {
      const auto sourceIt = ObjectMap.find(attribute.object.handle);
      if (sourceIt == ObjectMap.end() ||
          !(sourceFrameId(sourceIt->second) == sourceFrameId(attribute.object))) {
        return reject();
      }
      DwgFrameMapLease lease;
      if (!detachDwgSourceFrame(ObjectMap, sourceIt, lease))
        return reject();
      const DwgMappedEntityOutcome outcome =
          stageWithLease(lease, [this, &attribute, &intfa]() {
            return stagePendingAttribute(std::move(attribute.entity),
                                         attribute.publication, intfa);
          });
      if (outcome == DwgMappedEntityOutcome::Rejected) {
        restoreUnstagedLease(lease);
        return reject();
      }
    }

    const auto stagedSequenceIt = ObjectMap.find(sequenceHandle);
    if (stagedSequenceIt == ObjectMap.end() ||
        !(sourceFrameId(stagedSequenceIt->second) ==
          sourceFrameId(borrowedSequence.object))) {
      return reject();
    }
    DwgFrameMapLease sequenceLease;
    if (!detachDwgSourceFrame(ObjectMap, stagedSequenceIt, sequenceLease))
      return reject();
    const DwgMappedEntityOutcome outcome =
        stageWithLease(sequenceLease,
                       [this, sequenceHandle, insertHandle,
                        &sequencePublication, &intfa]() {
                         return stagePendingSeqEnd(
                             sequenceHandle, insertHandle,
                             sequencePublication, intfa);
                       });
    if (outcome != DwgMappedEntityOutcome::CommittedCompound) {
      restoreUnstagedLease(sequenceLease);
      return reject();
    }
    return outcome;
  } catch (...) {
    return reject();
  }
}

dwgReader::DwgMappedEntityOutcome dwgReader::stageMappedPolylineAggregate(
    DRW_Polyline &&polyline, const DRW_DwgFramePublication &publication,
    dwgBuffer *dbuf, DRW_Interface &intfa,
    DwgIntegrityAddressSpace offsetSpace) {
  if (version < DRW::AC1018 || dbuf == nullptr ||
      m_activeEntityFrameLease == nullptr ||
      !m_activeEntityFrameLease->isDetached() ||
      m_activeEntityFrameLease->object.handle != polyline.handle) {
    return DwgMappedEntityOutcome::Rejected;
  }

  const std::uint32_t polylineHandle = polyline.handle;
  std::vector<std::uint32_t> discoveredHandles;
  std::vector<std::uint32_t> declaredChildHandles;
  const auto reject = [this, polylineHandle, &discoveredHandles,
                       &declaredChildHandles]() {
    const auto discardChild = [this](std::uint32_t handle) {
      const auto objectIt = ObjectMap.find(handle);
      if (objectIt != ObjectMap.end())
        return discardDwgSourceFrame(ObjectMap, objectIt);
      const auto deferredIt = objObjectMap.find(handle);
      return deferredIt == objObjectMap.end() ||
             discardDwgSourceFrame(objObjectMap, deferredIt);
    };
    terminalizePendingPolylineState(polylineHandle,
                                    DwgInsertTerminalReason::MalformedGroup);
    try {
      m_invalidPolylineOwners.insert(polylineHandle);
    } catch (...) {
      // The source frames below still cannot be republished.
    }
    for (const std::uint32_t handle : discoveredHandles)
      (void)discardChild(handle);
    for (const std::uint32_t handle : declaredChildHandles)
      (void)discardChild(handle);
    return DwgMappedEntityOutcome::Rejected;
  };
  const auto isVertexType = [](std::int16_t type) {
    return type == dwgType::VERTEX_2D || type == dwgType::VERTEX_3D ||
           type == dwgType::VERTEX_MESH || type == dwgType::VERTEX_PFACE ||
           type == dwgType::VERTEX_PFACE_FACE;
  };

  try {
    const std::uint32_t sequenceHandle = polyline.seqEndH.ref;
    declaredChildHandles.reserve(polyline.hadlesList.size() + 1u);
    for (const std::uint32_t handle : polyline.hadlesList) {
      if (handle != DRW::NoHandle)
        declaredChildHandles.push_back(handle);
    }
    if (sequenceHandle != DRW::NoHandle)
      declaredChildHandles.push_back(sequenceHandle);

    discoveredHandles.reserve(polyline.hadlesList.size() + 1u);
    std::vector<std::uint32_t> declaredVertexHandles;
    declaredVertexHandles.reserve(polyline.hadlesList.size());
    std::unordered_set<std::uint32_t> declaredVertices;
    declaredVertices.reserve(polyline.hadlesList.size());
    for (const std::uint32_t handle : polyline.hadlesList) {
      if (handle == DRW::NoHandle || !declaredVertices.insert(handle).second) {
        return reject();
      }
      declaredVertexHandles.push_back(handle);

      const auto sourceIt = ObjectMap.find(handle);
      if (sourceIt == ObjectMap.end())
        continue;
      discoveredHandles.push_back(handle);
      DwgFrameClassification classification;
      if (!classifyDwgSourceFrame(dbuf, sourceIt->second, classification) ||
          classification.route != DwgFrameClassification::Route::Entity ||
          !isVertexType(classification.resolvedType)) {
        return reject();
      }
    }

    if (sequenceHandle == DRW::NoHandle ||
        declaredVertices.find(sequenceHandle) != declaredVertices.end()) {
      return reject();
    }
    const auto sequenceIt = ObjectMap.find(sequenceHandle);
    if (sequenceIt != ObjectMap.end()) {
      discoveredHandles.push_back(sequenceHandle);
      DwgFrameClassification classification;
      if (!classifyDwgSourceFrame(dbuf, sequenceIt->second, classification) ||
          classification.route != DwgFrameClassification::Route::Entity ||
          classification.resolvedType != dwgType::SEQEND) {
        return reject();
      }
    }

    const DwgMappedEntityOutcome parentOutcome =
        stagePendingPolyline(std::move(polyline), publication, intfa);
    if (parentOutcome == DwgMappedEntityOutcome::Rejected)
      return reject();
    if (parentOutcome == DwgMappedEntityOutcome::CommittedCompound) {
      return discoveredHandles.empty() ? parentOutcome : reject();
    }

    const auto stageMappedChild = [this, dbuf, &intfa,
                                   offsetSpace](DwgFrameMapLease &lease) {
      DwgFrameMapLease *const previousLease = m_activeEntityFrameLease;
      m_activeEntityFrameLease = nullptr;
      bool frameFailure = false;
      bool read = false;
      try {
        read =
            readMappedDwgEntity(dbuf, lease, intfa, &frameFailure, offsetSpace);
      } catch (...) {
        read = false;
        frameFailure = true;
      }
      m_activeEntityFrameLease = previousLease;
      return read && !frameFailure;
    };
    const auto restoreUnstagedLease = [this](DwgFrameMapLease &lease) {
      if (lease.isDetached())
        (void)restoreDwgSourceFrame(lease);
    };
    const auto stageDeclaredChild =
        [this, dbuf, &stageMappedChild,
         &restoreUnstagedLease](std::uint32_t handle) {
          const auto sourceIt = ObjectMap.find(handle);
          if (sourceIt == ObjectMap.end())
            return true;
          DwgFrameClassification classification;
          if (!classifyDwgSourceFrame(dbuf, sourceIt->second, classification)) {
            return false;
          }
          DwgFrameMapLease lease;
          if (!detachDwgSourceFrame(ObjectMap, sourceIt, lease))
            return false;
          lease.classification.emplace(std::move(classification));
          if (stageMappedChild(lease))
            return true;
          restoreUnstagedLease(lease);
          return false;
        };

    for (const std::uint32_t declared : declaredVertexHandles) {
      if (!stageDeclaredChild(declared))
        return reject();
    }
    if (!stageDeclaredChild(sequenceHandle))
      return reject();

    if (m_pendingPolylineStates.find(polylineHandle) ==
        m_pendingPolylineStates.end()) {
      return DwgMappedEntityOutcome::CommittedCompound;
    }
    const DwgMappedEntityOutcome outcome =
        tryCommitPendingPolyline(polylineHandle, intfa);
    return outcome == DwgMappedEntityOutcome::StagedCompound ? reject()
                                                             : outcome;
  } catch (...) {
    return reject();
  }
}

dwgReader::DwgMappedEntityOutcome
dwgReader::stagePendingAttribute(std::shared_ptr<DRW_Attrib> attribute,
                                 const DRW_DwgFramePublication &publication,
                                 DRW_Interface &intfa) {
  if (attribute == nullptr || attribute->handle == DRW::NoHandle ||
      attribute->parentHandle == DRW::NoHandle ||
      m_invalidInsertOwners.find(attribute->parentHandle) !=
          m_invalidInsertOwners.end()) {
    (void)reportDwgFrameTransitionFailure(
        DwgSourceFrameId{attribute ? attribute->handle : DRW::NoHandle});
    return DwgMappedEntityOutcome::Rejected;
  }

  const std::uint32_t owner = attribute->parentHandle;
  const auto pendingIt = m_pendingInsertStates.find(owner);
  if (pendingIt != m_pendingInsertStates.end()) {
    PendingInsertState &pending = pendingIt->second;
    if (!isExpectedAttribute(pending.entity, *attribute, version) ||
        std::any_of(pending.attributes.cbegin(), pending.attributes.cend(),
                    [&attribute](const StagedAttribState &current) {
                      return current.entity != nullptr &&
                             current.entity->handle == attribute->handle;
                    })) {
      terminalizeInsertGroup(owner, DwgInsertTerminalReason::MalformedGroup);
      (void)reportDwgFrameTransitionFailure(
          DwgSourceFrameId{attribute->handle});
      return DwgMappedEntityOutcome::Rejected;
    }
    try {
      pending.entity.attlist.reserve(pending.entity.attlist.size() + 1u);
      pending.attributes.reserve(pending.attributes.size() + 1u);
    } catch (...) {
      terminalizeInsertGroup(owner, DwgInsertTerminalReason::MalformedGroup);
      return DwgMappedEntityOutcome::Rejected;
    }

    StagedAttribState staged;
    staged.entity = std::move(attribute);
    if (!stageActiveEntityFrame(staged.frame, publication)) {
      terminalizeInsertGroup(owner, DwgInsertTerminalReason::MalformedGroup);
      return DwgMappedEntityOutcome::Rejected;
    }
    pending.entity.attlist.push_back(staged.entity);
    pending.attributes.push_back(std::move(staged));
    return tryCommitPendingInsert(owner, intfa);
  }

  auto orphanIt = m_orphanAttribStates.find(owner);
  if (orphanIt == m_orphanAttribStates.end()) {
    try {
      m_orphanAttribStates.reserve(m_orphanAttribStates.size() + 1u);
      orphanIt = m_orphanAttribStates.try_emplace(owner).first;
    } catch (...) {
      terminalizeOrphanAttribOwner(owner);
      return DwgMappedEntityOutcome::Rejected;
    }
  }
  if (std::any_of(orphanIt->second.attributes.cbegin(),
                  orphanIt->second.attributes.cend(),
                  [&attribute](const StagedAttribState &current) {
                    return current.entity != nullptr &&
                           current.entity->handle == attribute->handle;
                  })) {
    terminalizeOrphanAttribOwner(owner);
    (void)reportDwgFrameTransitionFailure(DwgSourceFrameId{attribute->handle});
    return DwgMappedEntityOutcome::Rejected;
  }
  try {
    orphanIt->second.attributes.reserve(orphanIt->second.attributes.size() +
                                        1u);
  } catch (...) {
    terminalizeOrphanAttribOwner(owner);
    return DwgMappedEntityOutcome::Rejected;
  }

  StagedAttribState staged;
  staged.entity = std::move(attribute);
  if (!stageActiveEntityFrame(staged.frame, publication)) {
    terminalizeOrphanAttribOwner(owner);
    return DwgMappedEntityOutcome::Rejected;
  }
  orphanIt->second.attributes.push_back(std::move(staged));
  return DwgMappedEntityOutcome::StagedCompound;
}

dwgReader::DwgMappedEntityOutcome
dwgReader::stagePendingSeqEnd(std::uint32_t handle, std::uint32_t owner,
                              const DRW_DwgFramePublication &publication,
                              DRW_Interface &intfa) {
  if (handle == DRW::NoHandle || owner == DRW::NoHandle ||
      publication.m_handle != handle ||
      m_invalidSeqEndHandles.find(handle) != m_invalidSeqEndHandles.end() ||
      m_invalidPolylineOwners.find(owner) != m_invalidPolylineOwners.end() ||
      m_consumedSeqEndHandles.find(handle) != m_consumedSeqEndHandles.end() ||
      m_stagedSeqEnds.find(handle) != m_stagedSeqEnds.end()) {
    (void)reportDwgFrameTransitionFailure(DwgSourceFrameId{handle});
    return DwgMappedEntityOutcome::Rejected;
  }
  const auto pendingIt = m_pendingInsertStates.find(owner);
  if (pendingIt != m_pendingInsertStates.end() &&
      pendingIt->second.entity.seqendH.ref != handle) {
    terminalizeInsertGroup(owner, DwgInsertTerminalReason::MalformedGroup);
    (void)reportDwgFrameTransitionFailure(DwgSourceFrameId{handle});
    return DwgMappedEntityOutcome::Rejected;
  }
  const auto polylineIt = m_pendingPolylineStates.find(owner);
  if (polylineIt != m_pendingPolylineStates.end() &&
      polylineIt->second.entity.seqEndH.ref != handle) {
    terminalizePendingPolylineState(owner,
                                    DwgInsertTerminalReason::MalformedGroup);
    (void)reportDwgFrameTransitionFailure(DwgSourceFrameId{handle});
    return DwgMappedEntityOutcome::Rejected;
  }
  try {
    m_stagedSeqEnds.reserve(m_stagedSeqEnds.size() + 1u);
  } catch (...) {
    if (pendingIt != m_pendingInsertStates.end()) {
      terminalizeInsertGroup(owner, DwgInsertTerminalReason::MalformedGroup);
    }
    return DwgMappedEntityOutcome::Rejected;
  }

  StagedSeqEndState staged;
  staged.owner = owner;
  if (!stageActiveEntityFrame(staged.frame, publication)) {
    if (pendingIt != m_pendingInsertStates.end()) {
      terminalizeInsertGroup(owner, DwgInsertTerminalReason::MalformedGroup);
    }
    return DwgMappedEntityOutcome::Rejected;
  }
  try {
    const auto inserted = m_stagedSeqEnds.emplace(handle, std::move(staged));
    if (!inserted.second) {
      (void)restoreStagedFrame(staged.frame);
      if (pendingIt != m_pendingInsertStates.end()) {
        terminalizeInsertGroup(owner, DwgInsertTerminalReason::MalformedGroup);
      }
      (void)reportDwgFrameTransitionFailure(DwgSourceFrameId{handle});
      return DwgMappedEntityOutcome::Rejected;
    }
  } catch (...) {
    (void)restoreStagedFrame(staged.frame);
    if (pendingIt != m_pendingInsertStates.end()) {
      terminalizeInsertGroup(owner, DwgInsertTerminalReason::MalformedGroup);
    }
    return DwgMappedEntityOutcome::Rejected;
  }
  const DwgMappedEntityOutcome insertOutcome =
      tryCommitPendingInsert(owner, intfa);
  if (insertOutcome == DwgMappedEntityOutcome::Rejected)
    return insertOutcome;
  const DwgMappedEntityOutcome polylineOutcome =
      tryCommitPendingPolyline(owner, intfa);
  if (polylineOutcome == DwgMappedEntityOutcome::Rejected ||
      polylineOutcome == DwgMappedEntityOutcome::CommittedCompound) {
    return polylineOutcome;
  }
  return insertOutcome;
}

dwgReader::DwgMappedEntityOutcome
dwgReader::stagePendingPolyline(DRW_Polyline &&polyline,
                                const DRW_DwgFramePublication &publication,
                                DRW_Interface &intfa) {
  const std::uint32_t handle = polyline.handle;
  if (handle == DRW::NoHandle || !polyline.vertlist.empty() ||
      m_invalidPolylineOwners.find(handle) != m_invalidPolylineOwners.end() ||
      m_pendingPolylineStates.find(handle) != m_pendingPolylineStates.end()) {
    (void)reportDwgFrameTransitionFailure(DwgSourceFrameId{handle});
    return DwgMappedEntityOutcome::Rejected;
  }

  const auto orphanIt = m_orphanPolylineVertexStates.find(handle);
  if (orphanIt != m_orphanPolylineVertexStates.end()) {
    const auto isExpectedExactlyOnce = [&polyline](
                                           const StagedVertexState &vertex) {
      return std::count(polyline.hadlesList.cbegin(),
                        polyline.hadlesList.cend(), vertex.entity.handle) == 1;
    };
    const bool invalidOrphan = std::any_of(
        orphanIt->second.vertices.cbegin(), orphanIt->second.vertices.cend(),
        [&polyline, &isExpectedExactlyOnce](const StagedVertexState &vertex) {
          return !isExpectedExactlyOnce(vertex) ||
                 !polyline.isDwgVertexCompatible(vertex.entity);
        });
    if (invalidOrphan) {
      terminalizeOrphanPolylineVertexOwner(handle);
      (void)reportDwgFrameTransitionFailure(DwgSourceFrameId{handle});
      return DwgMappedEntityOutcome::Rejected;
    }
  }
  if (consumePolylineStageFailurePointForTest(
          DwgPolylineStageFailurePoint::BeforeParentStateReserve)) {
    terminalizeOrphanPolylineVertexOwner(handle);
    return DwgMappedEntityOutcome::Rejected;
  }
  try {
    m_pendingPolylineStates.reserve(m_pendingPolylineStates.size() + 1u);
  } catch (...) {
    return DwgMappedEntityOutcome::Rejected;
  }

  if (consumePolylineStageFailurePointForTest(
          DwgPolylineStageFailurePoint::BeforeParentStateInsert)) {
    terminalizeOrphanPolylineVertexOwner(handle);
    return DwgMappedEntityOutcome::Rejected;
  }
  auto inserted = m_pendingPolylineStates.end();
  try {
    const auto result = m_pendingPolylineStates.try_emplace(handle);
    if (!result.second) {
      (void)reportDwgFrameTransitionFailure(DwgSourceFrameId{handle});
      return DwgMappedEntityOutcome::Rejected;
    }
    inserted = result.first;
  } catch (...) {
    return DwgMappedEntityOutcome::Rejected;
  }

  PendingPolylineState &pending = inserted->second;
  try {
    pending.entity = std::move(polyline);
  } catch (...) {
    m_pendingPolylineStates.erase(inserted);
    terminalizeOrphanPolylineVertexOwner(handle);
    return DwgMappedEntityOutcome::Rejected;
  }
  const std::size_t expectedVertexCount = pending.entity.hadlesList.size();
  const std::size_t adoptedVertexCount =
      orphanIt == m_orphanPolylineVertexStates.end()
          ? 0u
          : orphanIt->second.vertices.size();
  if (expectedVertexCount != 0u || adoptedVertexCount != 0u) {
    try {
      pending.vertices.reserve(
          std::max(expectedVertexCount, adoptedVertexCount));
    } catch (...) {
      m_pendingPolylineStates.erase(inserted);
      terminalizeOrphanPolylineVertexOwner(handle);
      return DwgMappedEntityOutcome::Rejected;
    }
  }
  if (consumePolylineStageFailurePointForTest(
          DwgPolylineStageFailurePoint::BeforeFrameStaging)) {
    m_pendingPolylineStates.erase(inserted);
    terminalizeOrphanPolylineVertexOwner(handle);
    return DwgMappedEntityOutcome::Rejected;
  }
  if (!stageActiveEntityFrame(pending.frame, publication)) {
    m_pendingPolylineStates.erase(inserted);
    terminalizeOrphanPolylineVertexOwner(handle);
    return DwgMappedEntityOutcome::Rejected;
  }
  if (orphanIt != m_orphanPolylineVertexStates.end()) {
    if (consumePolylineStageFailurePointForTest(
            DwgPolylineStageFailurePoint::BeforeOrphanAdoption)) {
      terminalizePendingPolylineState(handle,
                                      DwgInsertTerminalReason::MalformedGroup);
      return DwgMappedEntityOutcome::Rejected;
    }
    try {
      for (StagedVertexState &vertex : orphanIt->second.vertices) {
        if (vertex.entity.dwgSubtype() == DRW_Vertex::DwgSubtype::Vertex2D) {
          vertex.entity.basePoint.z = pending.entity.basePoint.z;
        }
        pending.vertices.push_back(std::move(vertex));
      }
    } catch (...) {
      terminalizePendingPolylineState(handle,
                                      DwgInsertTerminalReason::MalformedGroup);
      return DwgMappedEntityOutcome::Rejected;
    }
    m_orphanPolylineVertexStates.erase(orphanIt);
  }
  return tryCommitPendingPolyline(handle, intfa);
}

dwgReader::DwgMappedEntityOutcome
dwgReader::stageLegacyPolylineChain(DRW_Polyline &&polyline,
                                    const DRW_DwgFramePublication &publication,
                                    dwgBuffer *dbuf, DRW_Interface &intfa,
                                    DwgIntegrityAddressSpace offsetSpace) {
  if (dbuf == nullptr || version >= DRW::AC1018 ||
      m_activeEntityFrameLease == nullptr ||
      !m_activeEntityFrameLease->isDetached() ||
      m_activeEntityFrameLease->object.handle != polyline.handle) {
    return DwgMappedEntityOutcome::Rejected;
  }

  struct ParsedVertex {
    objHandle object;
    DRW_Vertex entity;
    DRW_DwgFramePublication publication;
  };

  const std::uint32_t parentHandle = polyline.handle;
  const std::uint32_t sequenceHandle = polyline.seqEndH.ref;
  const bool haveNextLinks = polyline.haveNextLinks != 0;
  const std::uint32_t savedNext = nextEntLink;
  const std::uint32_t savedPrev = prevEntLink;
  const bool savedNextImplicit = nextEntLinkImplicit;
  const auto restoreLinks = [this, savedNext, savedPrev, savedNextImplicit]() {
    nextEntLink = savedNext;
    prevEntLink = savedPrev;
    nextEntLinkImplicit = savedNextImplicit;
  };

  std::vector<ParsedVertex> vertices;
  std::vector<std::uint32_t> discoveredHandles;
  std::unordered_set<std::uint32_t> visitedHandles;
  const auto quarantineDiscovered = [this, &discoveredHandles]() {
    for (const std::uint32_t handle : discoveredHandles)
      (void)quarantineMappedDwgSourceFrame(handle);
  };
  const auto reject = [this, parentHandle, &restoreLinks,
                       &quarantineDiscovered]() {
    restoreLinks();
    terminalizePendingPolylineState(parentHandle,
                                    DwgInsertTerminalReason::MalformedGroup);
    quarantineDiscovered();
    return DwgMappedEntityOutcome::Rejected;
  };

  try {
    if (parentHandle == DRW::NoHandle || sequenceHandle == DRW::NoHandle)
      return reject();
    discoveredHandles.push_back(sequenceHandle);
    if (polyline.lastEH != DRW::NoHandle &&
        polyline.lastEH != polyline.firstEH) {
      discoveredHandles.push_back(polyline.lastEH);
    }

    std::uint32_t nextHandle = polyline.firstEH;
    bool reachedLast =
        nextHandle == DRW::NoHandle && polyline.lastEH == DRW::NoHandle;
    while (nextHandle != DRW::NoHandle) {
      if (!visitedHandles.insert(nextHandle).second)
        return reject();
      discoveredHandles.push_back(nextHandle);

      const auto sourceIt = ObjectMap.find(nextHandle);
      if (sourceIt == ObjectMap.end())
        return reject();
      DwgSourceFrameLease borrowed;
      if (!borrowDwgSourceFrame(ObjectMap, sourceIt, borrowed) ||
          borrowed.object.handle != nextHandle) {
        return reject();
      }

      DwgObjectFrame frame;
      if (!frame.readAt(*dbuf, version, borrowed.object.loc)) {
        recordObjectFrameFailure(borrowed.object, offsetSpace);
        return reject();
      }
      std::vector<std::uint8_t> &body = frame.body();
      dwgBuffer buffer(body.data(), body.size(), &decoder);
      const std::int16_t objectType = buffer.getObjType(version);
      buffer.resetPosition();
      if (!buffer.isGood() || (objectType != dwgType::VERTEX_2D &&
                               objectType != dwgType::VERTEX_3D &&
                               objectType != dwgType::VERTEX_MESH &&
                               objectType != dwgType::VERTEX_PFACE &&
                               objectType != dwgType::VERTEX_PFACE_FACE)) {
        return reject();
      }

      DRW_Vertex vertex;
      if (!vertex.parseDwg(version, &buffer, frame.bodyBitSize(),
                           polyline.basePoint.z) ||
          !buffer.isGood() || vertex.handle != nextHandle) {
        if (vertex.handle != nextHandle)
          parsedEntityHandleMismatch = true;
        return reject();
      }
      if (vertex.parentHandle != DRW::NoHandle &&
          vertex.parentHandle != parentHandle) {
        parsedEntityOwnerMismatch = true;
        return reject();
      }
      // The chain walker bypasses entryParse() so it can preserve the
      // parent walk's next-link state. Keep its EED reference
      // resolution equivalent to the mapped VERTEX path before staging.
      parseAttribs(&vertex);

      ParsedVertex parsed;
      parsed.object = borrowed.object;
      parsed.entity = std::move(vertex);
      parsed.publication = makeTypedEntityFramePublication(
          version, borrowed.object, objectType, parsed.entity);
      vertices.push_back(std::move(parsed));
      polyline.hadlesList.push_back(nextHandle);

      if (nextHandle == polyline.lastEH) {
        reachedLast = true;
        break;
      }
      nextHandle = vertices.back().entity.nextEntLink;
    }
    if (!reachedLast)
      return reject();

    const auto sequenceIt = ObjectMap.find(sequenceHandle);
    if (sequenceIt == ObjectMap.end())
      return reject();
    DwgSourceFrameLease borrowedSequence;
    if (!borrowDwgSourceFrame(ObjectMap, sequenceIt, borrowedSequence) ||
        borrowedSequence.object.handle != sequenceHandle) {
      return reject();
    }
    DwgObjectFrame sequenceFrame;
    if (!sequenceFrame.readAt(*dbuf, version, borrowedSequence.object.loc)) {
      recordObjectFrameFailure(borrowedSequence.object, offsetSpace);
      return reject();
    }
    std::vector<std::uint8_t> &sequenceBody = sequenceFrame.body();
    dwgBuffer sequenceBuffer(sequenceBody.data(), sequenceBody.size(),
                             &decoder);
    if (sequenceBuffer.getObjType(version) != dwgType::SEQEND ||
        !sequenceBuffer.isGood()) {
      return reject();
    }
    sequenceBuffer.resetPosition();
    DRW_SeqEnd sequenceEnd;
    if (!sequenceEnd.parseDwg(version, &sequenceBuffer,
                              sequenceFrame.bodyBitSize()) ||
        !sequenceBuffer.isGood() || sequenceEnd.handle != sequenceHandle) {
      if (sequenceEnd.handle != sequenceHandle)
        parsedEntityHandleMismatch = true;
      return reject();
    }
    if (sequenceEnd.parentHandle != parentHandle) {
      parsedEntityOwnerMismatch = true;
      return reject();
    }
    const DRW_DwgFramePublication sequencePublication =
        makeTypedEntityFramePublication(version, borrowedSequence.object,
                                        dwgType::SEQEND, sequenceEnd);

    if (stagePendingPolyline(std::move(polyline), publication, intfa) ==
        DwgMappedEntityOutcome::Rejected) {
      return reject();
    }

    const auto stageWithLease = [this](DwgFrameMapLease &lease,
                                       const auto &operation) {
      DwgFrameMapLease *const previousLease = m_activeEntityFrameLease;
      m_activeEntityFrameLease = &lease;
      DwgMappedEntityOutcome outcome = DwgMappedEntityOutcome::Rejected;
      try {
        outcome = operation();
      } catch (...) {
        outcome = DwgMappedEntityOutcome::Rejected;
      }
      m_activeEntityFrameLease = previousLease;
      return outcome;
    };
    const auto restoreUnstagedLease = [this](DwgFrameMapLease &lease) {
      if (lease.isDetached())
        (void)restoreDwgSourceFrame(lease);
    };

    for (ParsedVertex &vertex : vertices) {
      const auto sourceIt = ObjectMap.find(vertex.object.handle);
      if (sourceIt == ObjectMap.end() ||
          !(sourceFrameId(sourceIt->second) == sourceFrameId(vertex.object))) {
        return reject();
      }
      DwgFrameMapLease lease;
      if (!detachDwgSourceFrame(ObjectMap, sourceIt, lease))
        return reject();
      const DwgMappedEntityOutcome outcome =
          stageWithLease(lease, [this, &vertex, &intfa]() {
            return stagePendingPolylineVertex(std::move(vertex.entity),
                                              vertex.publication, intfa);
          });
      if (outcome == DwgMappedEntityOutcome::Rejected) {
        restoreUnstagedLease(lease);
        return reject();
      }
    }

    const auto stagedSequenceIt = ObjectMap.find(sequenceHandle);
    if (stagedSequenceIt == ObjectMap.end() ||
        !(sourceFrameId(stagedSequenceIt->second) ==
          sourceFrameId(borrowedSequence.object))) {
      return reject();
    }
    DwgFrameMapLease sequenceLease;
    if (!detachDwgSourceFrame(ObjectMap, stagedSequenceIt, sequenceLease)) {
      return reject();
    }
    const DwgMappedEntityOutcome outcome =
        stageWithLease(sequenceLease, [this, sequenceHandle, parentHandle,
                                       &sequencePublication, &intfa]() {
          return stagePendingSeqEnd(sequenceHandle, parentHandle,
                                    sequencePublication, intfa);
        });
    if (outcome != DwgMappedEntityOutcome::CommittedCompound) {
      restoreUnstagedLease(sequenceLease);
      return reject();
    }

    restoreLinks();
    if (haveNextLinks &&
        sequenceHandle != std::numeric_limits<std::uint32_t>::max()) {
      nextEntLink = sequenceHandle + 1u;
    }
    return outcome;
  } catch (...) {
    return reject();
  }
}

dwgReader::DwgMappedEntityOutcome dwgReader::stagePendingPolylineVertex(
    DRW_Vertex &&vertex, const DRW_DwgFramePublication &publication,
    DRW_Interface &intfa) {
  const std::uint32_t handle = vertex.handle;
  const std::uint32_t owner = vertex.parentHandle;
  if (handle == DRW::NoHandle || owner == DRW::NoHandle ||
      m_invalidPolylineOwners.find(owner) != m_invalidPolylineOwners.end()) {
    (void)reportDwgFrameTransitionFailure(DwgSourceFrameId{handle});
    return DwgMappedEntityOutcome::Rejected;
  }

  const auto pendingIt = m_pendingPolylineStates.find(owner);
  if (pendingIt != m_pendingPolylineStates.end()) {
    std::vector<std::uint32_t> conflictingOwners;
    try {
      for (const auto &candidate : m_pendingPolylineStates) {
        if (candidate.first != owner &&
            std::find(candidate.second.entity.hadlesList.cbegin(),
                      candidate.second.entity.hadlesList.cend(),
                      handle) != candidate.second.entity.hadlesList.cend()) {
          conflictingOwners.push_back(candidate.first);
        }
      }
    } catch (...) {
      terminalizePendingPolylineState(owner,
                                      DwgInsertTerminalReason::MalformedGroup);
      return DwgMappedEntityOutcome::Rejected;
    }
    if (!conflictingOwners.empty()) {
      terminalizePendingPolylineState(owner,
                                      DwgInsertTerminalReason::MalformedGroup);
      for (const std::uint32_t conflictingOwner : conflictingOwners) {
        terminalizePendingPolylineState(
            conflictingOwner, DwgInsertTerminalReason::MalformedGroup);
      }
      parsedEntityOwnerMismatch = true;
      (void)reportDwgFrameTransitionFailure(DwgSourceFrameId{handle});
      return DwgMappedEntityOutcome::Rejected;
    }

    PendingPolylineState &pending = pendingIt->second;
    const bool expected = std::find(pending.entity.hadlesList.cbegin(),
                                    pending.entity.hadlesList.cend(),
                                    handle) != pending.entity.hadlesList.cend();
    const bool duplicate =
        std::any_of(pending.vertices.cbegin(), pending.vertices.cend(),
                    [handle](const StagedVertexState &current) {
                      return current.entity.handle == handle;
                    });
    if (!expected || duplicate ||
        !pending.entity.isDwgVertexCompatible(vertex)) {
      terminalizePendingPolylineState(owner,
                                      DwgInsertTerminalReason::MalformedGroup);
      (void)reportDwgFrameTransitionFailure(DwgSourceFrameId{handle});
      return DwgMappedEntityOutcome::Rejected;
    }
    try {
      pending.vertices.reserve(pending.vertices.size() + 1u);
    } catch (...) {
      terminalizePendingPolylineState(owner,
                                      DwgInsertTerminalReason::MalformedGroup);
      return DwgMappedEntityOutcome::Rejected;
    }
    StagedVertexState staged;
    staged.entity = std::move(vertex);
    if (staged.entity.dwgSubtype() == DRW_Vertex::DwgSubtype::Vertex2D) {
      staged.entity.basePoint.z = pending.entity.basePoint.z;
    }
    if (!stageActiveEntityFrame(staged.frame, publication)) {
      terminalizePendingPolylineState(owner,
                                      DwgInsertTerminalReason::MalformedGroup);
      return DwgMappedEntityOutcome::Rejected;
    }
    pending.vertices.push_back(std::move(staged));
    return tryCommitPendingPolyline(owner, intfa);
  }

  const auto declaredParent = std::find_if(
      m_pendingPolylineStates.cbegin(), m_pendingPolylineStates.cend(),
      [handle](const auto &item) {
        return std::find(item.second.entity.hadlesList.cbegin(),
                         item.second.entity.hadlesList.cend(),
                         handle) != item.second.entity.hadlesList.cend();
      });
  if (declaredParent != m_pendingPolylineStates.cend()) {
    terminalizePendingPolylineState(declaredParent->first,
                                    DwgInsertTerminalReason::MalformedGroup);
    parsedEntityOwnerMismatch = true;
    (void)reportDwgFrameTransitionFailure(DwgSourceFrameId{handle});
    return DwgMappedEntityOutcome::Rejected;
  }

  auto orphanIt = m_orphanPolylineVertexStates.find(owner);
  if (orphanIt == m_orphanPolylineVertexStates.end()) {
    try {
      m_orphanPolylineVertexStates.reserve(m_orphanPolylineVertexStates.size() +
                                           1u);
      orphanIt = m_orphanPolylineVertexStates.try_emplace(owner).first;
    } catch (...) {
      return DwgMappedEntityOutcome::Rejected;
    }
  }
  const bool duplicate = std::any_of(
      orphanIt->second.vertices.cbegin(), orphanIt->second.vertices.cend(),
      [handle](const StagedVertexState &current) {
        return current.entity.handle == handle;
      });
  if (duplicate) {
    terminalizeOrphanPolylineVertexOwner(owner);
    (void)reportDwgFrameTransitionFailure(DwgSourceFrameId{handle});
    return DwgMappedEntityOutcome::Rejected;
  }
  try {
    orphanIt->second.vertices.reserve(orphanIt->second.vertices.size() + 1u);
  } catch (...) {
    terminalizeOrphanPolylineVertexOwner(owner);
    return DwgMappedEntityOutcome::Rejected;
  }
  StagedVertexState staged;
  staged.entity = std::move(vertex);
  if (!stageActiveEntityFrame(staged.frame, publication)) {
    terminalizeOrphanPolylineVertexOwner(owner);
    return DwgMappedEntityOutcome::Rejected;
  }
  orphanIt->second.vertices.push_back(std::move(staged));
  return DwgMappedEntityOutcome::StagedCompound;
}

bool dwgReader::preparePolylineCommit(std::uint32_t handle,
                                      PendingPolylineState &pending,
                                      DwgPreparedPolylineCommit &prepared) {
  if (!validateStagedFrame(pending.frame) || pending.entity.handle != handle ||
      !prepared.orderedVertices.empty() ||
      !prepared.claimedChildHandles.empty() ||
      prepared.sequenceEnd != nullptr || prepared.claimedSeqEnd ||
      pending.vertices.size() != pending.entity.hadlesList.size()) {
    return false;
  }
  const std::uint32_t sequenceHandle = pending.entity.seqEndH.ref;
  const auto sequenceIt = m_stagedSeqEnds.find(sequenceHandle);
  if (sequenceHandle == DRW::NoHandle || sequenceIt == m_stagedSeqEnds.end() ||
      sequenceIt->second.owner != handle ||
      !validateStagedFrame(sequenceIt->second.frame)) {
    return false;
  }
  prepared.sequenceEnd = &sequenceIt->second;
  if (consumePolylinePrepareFailurePointForTest(
          DwgPolylinePrepareFailurePoint::BeforeReservation)) {
    return false;
  }
  try {
    prepared.orderedVertices.reserve(pending.vertices.size());
    prepared.claimedChildHandles.reserve(pending.vertices.size());
  } catch (...) {
    return false;
  }
  for (const std::uint32_t expected : pending.entity.hadlesList) {
    if (expected == DRW::NoHandle)
      return false;
    const auto vertexIt =
        std::find_if(pending.vertices.begin(), pending.vertices.end(),
                     [expected, handle](const StagedVertexState &vertex) {
                       return vertex.entity.handle == expected &&
                              vertex.entity.parentHandle == handle;
                     });
    if (vertexIt == pending.vertices.end() ||
        !validateStagedFrame(vertexIt->frame) ||
        std::find(prepared.orderedVertices.cbegin(),
                  prepared.orderedVertices.cend(),
                  &*vertexIt) != prepared.orderedVertices.cend()) {
      return false;
    }
    prepared.orderedVertices.push_back(&*vertexIt);
  }
  const auto rollbackMarkers = [this, &prepared, sequenceHandle]() {
    for (const std::uint32_t child : prepared.claimedChildHandles)
      m_consumedCompoundChildHandles.erase(child);
    prepared.claimedChildHandles.clear();
    if (prepared.claimedSeqEnd) {
      m_consumedSeqEndHandles.erase(sequenceHandle);
      prepared.claimedSeqEnd = false;
    }
  };
  try {
    for (const StagedVertexState *vertex : prepared.orderedVertices) {
      if (consumePolylinePrepareFailurePointForTest(
              DwgPolylinePrepareFailurePoint::BeforeChildMarker)) {
        rollbackMarkers();
        return false;
      }
      if (!m_consumedCompoundChildHandles.insert(vertex->entity.handle)
               .second) {
        rollbackMarkers();
        return false;
      }
      prepared.claimedChildHandles.push_back(vertex->entity.handle);
    }
    if (consumePolylinePrepareFailurePointForTest(
            DwgPolylinePrepareFailurePoint::BeforeSeqEndMarker)) {
      rollbackMarkers();
      return false;
    }
    if (!m_consumedSeqEndHandles.insert(sequenceHandle).second) {
      rollbackMarkers();
      return false;
    }
    prepared.claimedSeqEnd = true;
  } catch (...) {
    rollbackMarkers();
    return false;
  }
  return true;
}

dwgReader::DwgMappedEntityOutcome
dwgReader::journalPreparedPolylineCommit(std::uint32_t handle,
                                         PendingPolylineState &pending,
                                         DwgPreparedPolylineCommit &prepared) {
  if (m_activeBlockTransaction == nullptr || m_activeBlockOutput == nullptr ||
      &m_activeBlockTransaction->output() != m_activeBlockOutput ||
      pending.frame.lease == std::nullopt) {
    return DwgMappedEntityOutcome::Rejected;
  }

  DwgBlockScopeTransaction &transaction = *m_activeBlockTransaction;
  DwgEntityOutput &output = *m_activeBlockOutput;
  const std::uint32_t sequenceHandle = pending.entity.seqEndH.ref;
  const auto releasePreparedMarkers = [this, &prepared, sequenceHandle]() {
    for (const std::uint32_t child : prepared.claimedChildHandles)
      m_consumedCompoundChildHandles.erase(child);
    prepared.claimedChildHandles.clear();
    if (prepared.claimedSeqEnd) {
      m_consumedSeqEndHandles.erase(sequenceHandle);
      prepared.claimedSeqEnd = false;
    }
  };
  std::size_t sourceCount = 0;
  std::uint64_t bodyByteCount = 0;
  const auto chargeFrame = [this, &sourceCount,
                            &bodyByteCount](const DwgStagedFrame &frame) {
    if (!validateStagedFrame(frame) || !frame.lease.has_value() ||
        sourceCount == std::numeric_limits<std::size_t>::max()) {
      return false;
    }
    const std::uint64_t frameBytes =
        frame.lease->classification.has_value()
            ? frame.lease->classification->bodyByteSize
            : frame.lease->bodyByteSize;
    std::uint64_t total = 0;
    if (!dwgSafety::add(bodyByteCount, frameBytes, total))
      return false;
    bodyByteCount = total;
    ++sourceCount;
    return true;
  };
  if (!chargeFrame(pending.frame)) {
    releasePreparedMarkers();
    return DwgMappedEntityOutcome::Rejected;
  }
  for (const StagedVertexState *vertex : prepared.orderedVertices) {
    if (vertex == nullptr || !chargeFrame(vertex->frame)) {
      releasePreparedMarkers();
      return DwgMappedEntityOutcome::Rejected;
    }
  }
  if (prepared.sequenceEnd == nullptr ||
      !chargeFrame(prepared.sequenceEnd->frame) ||
      !transaction.reserveAdmission(sourceCount, sourceCount + 1u,
                                    bodyByteCount)) {
    releasePreparedMarkers();
    return DwgMappedEntityOutcome::Rejected;
  }
  struct TransferredFrame {
    DwgStagedFrame *frame{nullptr};
    DwgSourceFrameId source;
  };
  std::vector<TransferredFrame> transferred;
  try {
    transferred.reserve(sourceCount);
  } catch (...) {
    releasePreparedMarkers();
    return DwgMappedEntityOutcome::Rejected;
  }
  const std::size_t eventStart = transaction.output().size();
  const auto rollback = [&]() {
    transaction.output().truncate(eventStart);
    bool restored = true;
    for (auto it = transferred.rbegin(); it != transferred.rend(); ++it) {
      DwgFrameMapLease restoredLease;
      if (it->frame == nullptr ||
          !transaction.releaseLast(it->source, restoredLease)) {
        restored = false;
        continue;
      }
      it->frame->lease.emplace(std::move(restoredLease));
    }
    releasePreparedMarkers();
    return restored;
  };
  const auto queueFrame = [this, &transaction, &output,
                           &transferred](DwgStagedFrame &frame) {
    if (!validateStagedFrame(frame) || !frame.lease.has_value())
      return false;
    const DwgSourceFrameId source = frame.lease->source;
    if (frame.publication.has_value()) {
      if (!output.appendFramePublication(*this, *frame.publication))
        return false;
    } else {
      auto sourceScope = output.bindSource(source);
      if (!output.appendFrameCompletion())
        return false;
    }
    if (!transaction.adopt(*frame.lease))
      return false;
    transferred.push_back({&frame, source});
    return true;
  };

  DRW_Polyline delivery;
  {
    try {
      delivery = pending.entity;
      for (const StagedVertexState *vertex : prepared.orderedVertices)
        delivery.addVertex(vertex->entity);
    } catch (...) {
      releasePreparedMarkers();
      return DwgMappedEntityOutcome::Rejected;
    }
  }
  bool committed = false;
  try {
    const DwgSourceFrameId parentSource = pending.frame.lease->source;
    {
      auto sourceScope = output.bindSource(parentSource);
      output.appendValue(delivery, &DRW_Interface::addPolyline);
    }
    committed = queueFrame(pending.frame);
    for (StagedVertexState *vertex : prepared.orderedVertices)
      committed = committed && queueFrame(vertex->frame);
    committed = committed && queueFrame(prepared.sequenceEnd->frame);
  } catch (...) {
    committed = false;
  }
  if (!committed) {
    (void)rollback();
    return DwgMappedEntityOutcome::Rejected;
  }

  for (const TransferredFrame &frame : transferred) {
    frame.frame->lease.reset();
    frame.frame->publication.reset();
  }
  m_stagedSeqEnds.erase(sequenceHandle);
  m_pendingPolylineStates.erase(handle);
  return DwgMappedEntityOutcome::CommittedCompound;
}

dwgReader::DwgMappedEntityOutcome dwgReader::deliverPreparedPolylineCommit(
    std::uint32_t handle, PendingPolylineState &pending,
    DwgPreparedPolylineCommit &prepared, DRW_Interface &intfa) {
  if (m_activeBlockTransaction != nullptr || m_activeBlockOutput != nullptr)
    return journalPreparedPolylineCommit(handle, pending, prepared);

  const std::uint32_t sequenceHandle = pending.entity.seqEndH.ref;
  const auto publishFrame = [this, &intfa](DwgStagedFrame &frame) {
    if (!validateStagedFrame(frame))
      return false;
    if (frame.lease->hasCoverage &&
        (!frame.publication.has_value() ||
         !publishDwgFramePublication(intfa, *frame.publication))) {
      return false;
    }
    if (!discardDetachedDwgSourceFrame(*frame.lease))
      return false;
    frame.lease.reset();
    frame.publication.reset();
    return true;
  };

  for (const StagedVertexState *vertex : prepared.orderedVertices)
    pending.entity.addVertex(vertex->entity);
  try {
    intfa.addPolyline(pending.entity);
  } catch (...) {
    terminalizePendingPolylineState(handle,
                                    DwgInsertTerminalReason::CallbackException);
    return DwgMappedEntityOutcome::Rejected;
  }
  if (!publishFrame(pending.frame)) {
    terminalizePendingPolylineState(handle,
                                    DwgInsertTerminalReason::ReceiptFailure);
    return DwgMappedEntityOutcome::Rejected;
  }
  for (StagedVertexState *vertex : prepared.orderedVertices) {
    if (!publishFrame(vertex->frame)) {
      terminalizePendingPolylineState(handle,
                                      DwgInsertTerminalReason::ReceiptFailure);
      return DwgMappedEntityOutcome::Rejected;
    }
  }
  if (!publishFrame(prepared.sequenceEnd->frame)) {
    terminalizePendingPolylineState(handle,
                                    DwgInsertTerminalReason::ReceiptFailure);
    return DwgMappedEntityOutcome::Rejected;
  }
  m_stagedSeqEnds.erase(sequenceHandle);
  m_pendingPolylineStates.erase(handle);
  return DwgMappedEntityOutcome::CommittedCompound;
}

dwgReader::DwgMappedEntityOutcome
dwgReader::tryCommitPendingPolyline(std::uint32_t handle,
                                    DRW_Interface &intfa) {
  const auto pendingIt = m_pendingPolylineStates.find(handle);
  if (pendingIt == m_pendingPolylineStates.end())
    return DwgMappedEntityOutcome::StagedCompound;
  PendingPolylineState &pending = pendingIt->second;
  if (!validateStagedFrame(pending.frame) || pending.entity.handle != handle) {
    terminalizePendingPolylineState(handle,
                                    DwgInsertTerminalReason::MalformedGroup);
    return DwgMappedEntityOutcome::Rejected;
  }
  if (pending.vertices.size() != pending.entity.hadlesList.size() ||
      pending.entity.seqEndH.ref == DRW::NoHandle ||
      m_stagedSeqEnds.find(pending.entity.seqEndH.ref) ==
          m_stagedSeqEnds.end()) {
    return DwgMappedEntityOutcome::StagedCompound;
  }
  DwgPreparedPolylineCommit prepared;
  if (!preparePolylineCommit(handle, pending, prepared)) {
    terminalizePendingPolylineState(handle,
                                    DwgInsertTerminalReason::MalformedGroup);
    return DwgMappedEntityOutcome::Rejected;
  }
  return deliverPreparedPolylineCommit(handle, pending, prepared, intfa);
}

bool dwgReader::prepareInsertCommit(std::uint32_t handle,
                                    PendingInsertState &pending,
                                    DwgPreparedInsertCommit &prepared) {
  if (!validateStagedFrame(pending.frame) || pending.entity.handle != handle ||
      pending.entity.attlist.size() != pending.attributes.size() ||
      !prepared.orderedAttributes.empty() ||
      !prepared.orderedEntities.empty() ||
      !prepared.claimedChildHandles.empty() ||
      prepared.sequenceEnd != nullptr || prepared.claimedSeqEnd) {
    return false;
  }

  const std::uint32_t sequenceHandle = pending.entity.seqendH.ref;
  if (sequenceHandle != DRW::NoHandle) {
    const auto sequenceIt = m_stagedSeqEnds.find(sequenceHandle);
    if (sequenceIt == m_stagedSeqEnds.end() ||
        sequenceIt->second.owner != handle ||
        !validateStagedFrame(sequenceIt->second.frame)) {
      return false;
    }
    prepared.sequenceEnd = &sequenceIt->second;
  }

  try {
    prepared.orderedAttributes.reserve(pending.attributes.size());
    prepared.orderedEntities.reserve(pending.attributes.size());
    prepared.claimedChildHandles.reserve(pending.attributes.size());
  } catch (...) {
    return false;
  }

  const auto appendAttribute = [&pending, this,
                                &prepared](StagedAttribState &attribute) {
    if (attribute.entity == nullptr ||
        !isExpectedAttribute(pending.entity, *attribute.entity, version) ||
        !validateStagedFrame(attribute.frame)) {
      return false;
    }
    prepared.orderedAttributes.push_back(&attribute);
    prepared.orderedEntities.push_back(attribute.entity);
    return true;
  };
  try {
    if (version < DRW::AC1018 && pending.entity.attribHandles.size() == 2u) {
      for (StagedAttribState &attribute : pending.attributes) {
        if (!appendAttribute(attribute))
          return false;
      }
    } else {
      for (const dwgHandle &expected : pending.entity.attribHandles) {
        if (expected.ref == DRW::NoHandle)
          return false;
        const auto attributeIt =
            std::find_if(pending.attributes.begin(), pending.attributes.end(),
                         [&expected](const StagedAttribState &attribute) {
                           return attribute.entity != nullptr &&
                                  attribute.entity->handle == expected.ref;
                         });
        if (attributeIt == pending.attributes.end() ||
            !appendAttribute(*attributeIt)) {
          return false;
        }
      }
    }
  } catch (...) {
    return false;
  }

  const auto rollbackMarkers = [this, &prepared, sequenceHandle]() {
    for (const std::uint32_t childHandle : prepared.claimedChildHandles)
      m_consumedCompoundChildHandles.erase(childHandle);
    prepared.claimedChildHandles.clear();
    if (prepared.claimedSeqEnd) {
      m_consumedSeqEndHandles.erase(sequenceHandle);
      prepared.claimedSeqEnd = false;
    }
  };
  try {
    for (const StagedAttribState *attribute : prepared.orderedAttributes) {
      const auto inserted =
          m_consumedCompoundChildHandles.insert(attribute->entity->handle);
      if (!inserted.second) {
        rollbackMarkers();
        return false;
      }
      prepared.claimedChildHandles.push_back(attribute->entity->handle);
    }
    if (prepared.sequenceEnd != nullptr) {
      const auto inserted = m_consumedSeqEndHandles.insert(sequenceHandle);
      if (!inserted.second) {
        rollbackMarkers();
        return false;
      }
      prepared.claimedSeqEnd = true;
    }
  } catch (...) {
    rollbackMarkers();
    return false;
  }
  return true;
}

dwgReader::DwgMappedEntityOutcome
dwgReader::journalPreparedInsertCommit(std::uint32_t handle,
                                       PendingInsertState &pending,
                                       DwgPreparedInsertCommit &prepared) {
  if (m_activeBlockTransaction == nullptr || m_activeBlockOutput == nullptr ||
      &m_activeBlockTransaction->output() != m_activeBlockOutput ||
      pending.frame.lease == std::nullopt) {
    return DwgMappedEntityOutcome::Rejected;
  }

  DwgBlockScopeTransaction &transaction = *m_activeBlockTransaction;
  DwgEntityOutput &output = *m_activeBlockOutput;
  const std::uint32_t sequenceHandle = pending.entity.seqendH.ref;
  const auto releasePreparedMarkers = [this, &prepared, sequenceHandle]() {
    for (const std::uint32_t child : prepared.claimedChildHandles)
      m_consumedCompoundChildHandles.erase(child);
    prepared.claimedChildHandles.clear();
    if (prepared.claimedSeqEnd) {
      m_consumedSeqEndHandles.erase(sequenceHandle);
      prepared.claimedSeqEnd = false;
    }
  };
  std::size_t sourceCount = 0;
  std::uint64_t bodyByteCount = 0;
  const auto chargeFrame = [this, &sourceCount,
                            &bodyByteCount](const DwgStagedFrame &frame) {
    if (!validateStagedFrame(frame) || !frame.lease.has_value() ||
        sourceCount == std::numeric_limits<std::size_t>::max()) {
      return false;
    }
    const std::uint64_t frameBytes =
        frame.lease->classification.has_value()
            ? frame.lease->classification->bodyByteSize
            : frame.lease->bodyByteSize;
    std::uint64_t total = 0;
    if (!dwgSafety::add(bodyByteCount, frameBytes, total))
      return false;
    bodyByteCount = total;
    ++sourceCount;
    return true;
  };
  if (!chargeFrame(pending.frame)) {
    releasePreparedMarkers();
    return DwgMappedEntityOutcome::Rejected;
  }
  for (const StagedAttribState *attribute : prepared.orderedAttributes) {
    if (attribute == nullptr || !chargeFrame(attribute->frame)) {
      releasePreparedMarkers();
      return DwgMappedEntityOutcome::Rejected;
    }
  }
  if ((prepared.sequenceEnd != nullptr &&
       !chargeFrame(prepared.sequenceEnd->frame)) ||
      !transaction.reserveAdmission(sourceCount, sourceCount + 1u,
                                    bodyByteCount)) {
    releasePreparedMarkers();
    return DwgMappedEntityOutcome::Rejected;
  }
  struct TransferredFrame {
    DwgStagedFrame *frame{nullptr};
    DwgSourceFrameId source;
  };
  std::vector<TransferredFrame> transferred;
  try {
    transferred.reserve(sourceCount);
  } catch (...) {
    releasePreparedMarkers();
    return DwgMappedEntityOutcome::Rejected;
  }
  const std::size_t eventStart = transaction.output().size();
  const auto rollback = [&]() {
    transaction.output().truncate(eventStart);
    bool restored = true;
    for (auto it = transferred.rbegin(); it != transferred.rend(); ++it) {
      DwgFrameMapLease restoredLease;
      if (it->frame == nullptr ||
          !transaction.releaseLast(it->source, restoredLease)) {
        restored = false;
        continue;
      }
      it->frame->lease.emplace(std::move(restoredLease));
    }
    releasePreparedMarkers();
    return restored;
  };
  const auto queueFrame = [this, &transaction, &output,
                           &transferred](DwgStagedFrame &frame) {
    if (!validateStagedFrame(frame) || !frame.lease.has_value())
      return false;
    const DwgSourceFrameId source = frame.lease->source;
    if (frame.publication.has_value()) {
      if (!output.appendFramePublication(*this, *frame.publication))
        return false;
    } else {
      auto sourceScope = output.bindSource(source);
      if (!output.appendFrameCompletion())
        return false;
    }
    if (!transaction.adopt(*frame.lease))
      return false;
    transferred.push_back({&frame, source});
    return true;
  };

  DRW_Insert delivery;
  {
    try {
      delivery = pending.entity;
      delivery.attlist = prepared.orderedEntities;
    } catch (...) {
      releasePreparedMarkers();
      return DwgMappedEntityOutcome::Rejected;
    }
  }
  bool committed = false;
  try {
    const DwgSourceFrameId parentSource = pending.frame.lease->source;
    {
      auto sourceScope = output.bindSource(parentSource);
      output.appendValue(delivery, &DRW_Interface::addInsert);
    }
    committed = queueFrame(pending.frame);
    for (StagedAttribState *attribute : prepared.orderedAttributes)
      committed = committed && queueFrame(attribute->frame);
    if (prepared.sequenceEnd != nullptr)
      committed = committed && queueFrame(prepared.sequenceEnd->frame);
  } catch (...) {
    committed = false;
  }
  if (!committed) {
    (void)rollback();
    return DwgMappedEntityOutcome::Rejected;
  }

  for (const TransferredFrame &frame : transferred) {
    frame.frame->lease.reset();
    frame.frame->publication.reset();
  }
  if (prepared.sequenceEnd != nullptr)
    m_stagedSeqEnds.erase(sequenceHandle);
  m_pendingInsertStates.erase(handle);
  return DwgMappedEntityOutcome::CommittedCompound;
}

dwgReader::DwgMappedEntityOutcome dwgReader::deliverPreparedInsertCommit(
    std::uint32_t handle, PendingInsertState &pending,
    DwgPreparedInsertCommit &prepared, DRW_Interface &intfa) {
  if (m_activeBlockTransaction != nullptr || m_activeBlockOutput != nullptr)
    return journalPreparedInsertCommit(handle, pending, prepared);

  const std::uint32_t sequenceHandle = pending.entity.seqendH.ref;
  pending.entity.attlist.swap(prepared.orderedEntities);

  const auto publishFrame = [this, &intfa](DwgStagedFrame &frame) {
    if (!validateStagedFrame(frame))
      return false;
    if (frame.lease->hasCoverage &&
        (!frame.publication.has_value() ||
         !publishDwgFramePublication(intfa, *frame.publication))) {
      return false;
    }
    if (!discardDetachedDwgSourceFrame(*frame.lease))
      return false;
    frame.lease.reset();
    frame.publication.reset();
    return true;
  };

  try {
    intfa.addInsert(pending.entity);
  } catch (...) {
    terminalizeInsertGroup(handle, DwgInsertTerminalReason::CallbackException);
    return DwgMappedEntityOutcome::Rejected;
  }
  if (!publishFrame(pending.frame)) {
    terminalizeInsertGroup(handle, DwgInsertTerminalReason::ReceiptFailure);
    return DwgMappedEntityOutcome::Rejected;
  }
  for (StagedAttribState *attribute : prepared.orderedAttributes) {
    if (!publishFrame(attribute->frame)) {
      terminalizeInsertGroup(handle, DwgInsertTerminalReason::ReceiptFailure);
      return DwgMappedEntityOutcome::Rejected;
    }
  }
  if (prepared.sequenceEnd != nullptr &&
      !publishFrame(prepared.sequenceEnd->frame)) {
    terminalizeInsertGroup(handle, DwgInsertTerminalReason::ReceiptFailure);
    return DwgMappedEntityOutcome::Rejected;
  }

  if (version < DRW::AC1018 && pending.entity.haveNextLinks != 0 &&
      sequenceHandle != std::numeric_limits<std::uint32_t>::max()) {
    nextEntLink = sequenceHandle + 1;
  }
  if (prepared.sequenceEnd != nullptr)
    m_stagedSeqEnds.erase(sequenceHandle);
  m_pendingInsertStates.erase(handle);
  return DwgMappedEntityOutcome::CommittedCompound;
}

dwgReader::DwgMappedEntityOutcome
dwgReader::tryCommitPendingInsert(std::uint32_t handle, DRW_Interface &intfa) {
  const auto pendingIt = m_pendingInsertStates.find(handle);
  if (pendingIt == m_pendingInsertStates.end())
    return DwgMappedEntityOutcome::StagedCompound;

  PendingInsertState &pending = pendingIt->second;
  if (!validateStagedFrame(pending.frame) || pending.entity.handle != handle ||
      pending.entity.attlist.size() != pending.attributes.size()) {
    abandonPendingInsertState(handle);
    return DwgMappedEntityOutcome::Rejected;
  }
  if (!hasCompleteAttributeList(pending.entity, version))
    return DwgMappedEntityOutcome::StagedCompound;

  const std::uint32_t sequenceHandle = pending.entity.seqendH.ref;
  if (sequenceHandle != DRW::NoHandle &&
      m_stagedSeqEnds.find(sequenceHandle) == m_stagedSeqEnds.end()) {
    return DwgMappedEntityOutcome::StagedCompound;
  }

  DwgPreparedInsertCommit prepared;
  if (!prepareInsertCommit(handle, pending, prepared)) {
    abandonPendingInsertState(handle);
    return DwgMappedEntityOutcome::Rejected;
  }
  return deliverPreparedInsertCommit(handle, pending, prepared, intfa);
}

dwgReader::~dwgReader() {
  mapCleanUp(ltypemap);
  mapCleanUp(layermap);
  mapCleanUp(blockmap);
  mapCleanUp(stylemap);
  mapCleanUp(dimstylemap);
  mapCleanUp(vportmap);
  mapCleanUp(classesmap);
  mapCleanUp(blockRecordmap);
  mapCleanUp(appIdmap);
  mapCleanUp(viewmap);
  mapCleanUp(ucsmap);
}

bool dwgReader::dwgClassBitPosition(const dwgBuffer &buffer,
                                    std::uint64_t &position) noexcept {
  std::uint64_t bytePosition = 0;
  return buffer.isGood() &&
         dwgSafety::multiply(buffer.getPosition(), 8, bytePosition) &&
         dwgSafety::add(bytePosition, buffer.getBitPos(), position);
}

bool dwgReader::setDwgClassBitRange(DRW_DwgClassBitRange &range,
                                    std::uint64_t start, std::uint64_t end,
                                    DRW_DwgFrameOffsetSpace offsetSpace,
                                    bool sectionRelative) noexcept {
  if (end < start)
    return false;
  range.m_startBit = start;
  range.m_endBit = end;
  range.m_present = true;
  range.m_sectionRelative = sectionRelative;
  range.m_offsetSpace = offsetSpace;
  return true;
}

void dwgReader::beginDwgClassCoverage() noexcept {
  m_dwgClassCoverageReport.m_entries.clear();
  m_dwgClassCoverageReport.m_status = DRW_DwgClassCoverageStatus::InProgress;
  m_dwgClassCoverageReport.m_complete = false;
  m_dwgClassNumberOrdinals.clear();
  m_dwgClassCoveragePublished = false;
  m_dwgClassCoverageCaptureFailed = false;
}

DRW_DwgClassCoverageEntry
dwgReader::makeDwgClassCoverageEntry(const DRW_Class &value,
                                     std::int32_t sectionDescriptorId) const {
  DRW_DwgClassCoverageEntry entry;
  entry.m_classNumber = value.classNum;
  entry.m_recordName = value.recName;
  entry.m_className = value.className;
  entry.m_appName = value.appName;
  entry.m_proxyFlag = value.proxyFlag;
  entry.m_wasAProxyFlag = value.wasaProxyFlag;
  entry.m_entityFlagRaw = value.entityFlagRaw;
  entry.m_entityFlag = value.entityFlag;
  entry.m_instanceCount = value.instanceCount;
  entry.m_dwgVersion = value.dwgVersion;
  entry.m_maintenanceVersion = value.maintenanceVersion;
  entry.m_unknown1 = value.unknown1;
  entry.m_unknown2 = value.unknown2;
  entry.m_sectionDescriptorId = sectionDescriptorId;
  return entry;
}

void dwgReader::recordDwgClassCoverageFailure(
    DRW_DwgClassCoverageEntry coverage,
    DRW_DwgClassCoverageReason reason) noexcept {
  coverage.m_streamOrdinal = m_dwgClassCoverageReport.m_entries.size();
  coverage.m_state = DRW_DwgClassCoverageState::Failed;
  coverage.m_reason = reason;
  try {
    m_dwgClassCoverageReport.m_entries.push_back(std::move(coverage));
  } catch (...) {
    m_dwgClassCoverageCaptureFailed = true;
  }
}

bool dwgReader::stageDwgClass(std::vector<DwgStagedClass> &stagedClasses,
                              std::unique_ptr<DRW_Class> value,
                              DRW_DwgClassCoverageEntry coverage) {
  if (value == nullptr) {
    m_dwgClassCoverageCaptureFailed = true;
    return false;
  }
  coverage.m_streamOrdinal = m_dwgClassCoverageReport.m_entries.size();
  coverage.m_state = DRW_DwgClassCoverageState::Parsed;
  coverage.m_reason = DRW_DwgClassCoverageReason::None;
  try {
    m_dwgClassCoverageReport.m_entries.push_back(std::move(coverage));
    const std::size_t coverageIndex =
        m_dwgClassCoverageReport.m_entries.size() - 1;
    try {
      stagedClasses.push_back(DwgStagedClass{std::move(value), coverageIndex});
    } catch (...) {
      DRW_DwgClassCoverageEntry &entry =
          m_dwgClassCoverageReport.m_entries[coverageIndex];
      entry.m_state = DRW_DwgClassCoverageState::Failed;
      entry.m_reason = DRW_DwgClassCoverageReason::Publish;
      m_dwgClassCoverageCaptureFailed = true;
      return false;
    }
  } catch (...) {
    m_dwgClassCoverageCaptureFailed = true;
    return false;
  }
  return true;
}

void dwgReader::finalizeDwgClassCoverage(DRW_Interface &intfa,
                                         bool classReadCompleted) {
  if (m_dwgClassCoverageReport.m_status ==
          DRW_DwgClassCoverageStatus::NotAvailable ||
      m_dwgClassCoveragePublished) {
    return;
  }

  m_dwgClassCoverageReport.m_complete =
      classReadCompleted && !m_dwgClassCoverageCaptureFailed &&
      std::all_of(m_dwgClassCoverageReport.m_entries.cbegin(),
                  m_dwgClassCoverageReport.m_entries.cend(),
                  [](const DRW_DwgClassCoverageEntry &entry) {
                    return entry.m_state ==
                           DRW_DwgClassCoverageState::Published;
                  });
  m_dwgClassCoverageReport.m_status =
      m_dwgClassCoverageReport.m_complete
          ? DRW_DwgClassCoverageStatus::FinalizedComplete
          : DRW_DwgClassCoverageStatus::FinalizedPartial;

  m_dwgClassCoveragePublished = true;
  try {
    intfa.addDwgClassCoverageReport(m_dwgClassCoverageReport);
  } catch (...) {
    m_dwgClassCoverageReport.m_complete = false;
    m_dwgClassCoverageReport.m_status =
        DRW_DwgClassCoverageStatus::FinalizedPartial;
    for (DRW_DwgClassCoverageEntry &entry :
         m_dwgClassCoverageReport.m_entries) {
      if (entry.m_state == DRW_DwgClassCoverageState::Published)
        entry.m_reason = DRW_DwgClassCoverageReason::Callback;
    }
  }
}

void dwgReader::finalizeDwgClassCoverageNoThrow(
    DRW_Interface &intfa, bool classReadCompleted) noexcept {
  try {
    finalizeDwgClassCoverage(intfa, classReadCompleted);
  } catch (...) {
    m_dwgClassCoverageReport.m_complete = false;
    m_dwgClassCoverageReport.m_status =
        DRW_DwgClassCoverageStatus::FinalizedPartial;
  }
}

bool dwgReader::publishDwgClasses(std::vector<DwgStagedClass> &stagedClasses) {
  const std::size_t maxCount =
      static_cast<std::size_t>(std::numeric_limits<int>::max());
  if (classesmap.size() > maxCount || stagedClasses.size() > maxCount ||
      stagedClasses.size() > maxCount - classesmap.size()) {
    m_dwgClassCoverageCaptureFailed = true;
    return false;
  }
  for (const auto &staged : stagedClasses) {
    if (staged.m_value == nullptr ||
        staged.m_coverageIndex >= m_dwgClassCoverageReport.m_entries.size()) {
      m_dwgClassCoverageCaptureFailed = true;
      return false;
    }
  }
  if (!DRW::reserve(classesmap, static_cast<int>(classesmap.size() +
                                                 stagedClasses.size())) ||
      !DRW::reserve(m_dwgClassNumberOrdinals,
                    static_cast<int>(stagedClasses.size()))) {
    m_dwgClassCoverageCaptureFailed = true;
    return false;
  }

  std::vector<std::uint32_t> published;
  if (!DRW::reserve(published, static_cast<int>(stagedClasses.size()))) {
    m_dwgClassCoverageCaptureFailed = true;
    return false;
  }
  const auto rollback = [&published, this] {
    for (const std::uint32_t classNumber : published) {
      classesmap.erase(classNumber);
      m_dwgClassNumberOrdinals.erase(classNumber);
    }
  };

  try {
    for (auto &staged : stagedClasses) {
      DRW_DwgClassCoverageEntry &coverage =
          m_dwgClassCoverageReport.m_entries[staged.m_coverageIndex];
      const auto result =
          classesmap.emplace(staged.m_value->classNum, staged.m_value.get());
      if (!result.second) {
        coverage.m_state = DRW_DwgClassCoverageState::Failed;
        coverage.m_reason = DRW_DwgClassCoverageReason::Publish;
        rollback();
        return false;
      }
      // `published` is pre-reserved and stores a trivially movable
      // value, so register the map insertion before the second map can
      // allocate. Any exception below can then roll this pointer back.
      published.push_back(staged.m_value->classNum);
      const auto ordinalResult = m_dwgClassNumberOrdinals.emplace(
          staged.m_value->classNum, coverage.m_streamOrdinal);
      if (!ordinalResult.second) {
        coverage.m_state = DRW_DwgClassCoverageState::Failed;
        coverage.m_reason = DRW_DwgClassCoverageReason::Publish;
        rollback();
        return false;
      }
    }
    for (DwgStagedClass &staged : stagedClasses) {
      m_dwgClassCoverageReport.m_entries[staged.m_coverageIndex].m_state =
          DRW_DwgClassCoverageState::Published;
      staged.m_value.release();
    }
  } catch (...) {
    rollback();
    m_dwgClassCoverageCaptureFailed = true;
    return false;
  }
  return true;
}

void dwgReader::parseAttribs(DRW_Entity *e) {
  if (nullptr == e) {
    return;
  }

  std::uint32_t ltref = e->lTypeH.ref;
  std::uint32_t lyref = e->layerH.ref;
  auto lt_it = ltypemap.find(ltref);
  if (lt_it != ltypemap.end()) {
    e->lineType = (lt_it->second)->name;
  }
  auto ly_it = layermap.find(lyref);
  if (ly_it != layermap.end()) {
    e->layer = (ly_it->second)->name;
  }

  // Drain any deferred EED handle lookups now that the symbol tables
  // are populated. parseDwg() pushed placeholder DRW_Variants for
  // APPID names (DXF 1001) and layer-table refs (DXF 1003 with the
  // isLayerRef flag); fill in their string content here.
  for (const auto &p : e->pendingAppIdResolutions) {
    if (p.indexInExtData >= e->extData.size())
      continue;
    auto &v = e->extData[p.indexInExtData];
    if (!v)
      continue;
    auto it = appIdmap.find(p.handleRef);
    if (it != appIdmap.end() && it->second != nullptr) {
      v->addString(1001, it->second->name);
    } else {
      char fallback[24];
      std::snprintf(fallback, sizeof(fallback), "ACAD_%X", p.handleRef);
      v->addString(1001, std::string{fallback});
    }
  }
  e->pendingAppIdResolutions.clear();

  for (const auto &p : e->pendingLayerRefResolutions) {
    if (p.indexInExtData >= e->extData.size())
      continue;
    auto &v = e->extData[p.indexInExtData];
    if (!v)
      continue;
    const std::string name = findTableName(DRW::LAYER, p.handleRef);
    if (!name.empty())
      v->setLayerRefName(name);
  }
  e->pendingLayerRefResolutions.clear();
}

void dwgReader::parseAttribs(DRW_TableEntry *e) {
  if (e == nullptr)
    return;

  for (const auto &p : e->pendingAppIdResolutions) {
    if (p.indexInExtData >= e->extData.size() ||
        e->extData[p.indexInExtData] == nullptr)
      continue;
    auto it = appIdmap.find(p.handleRef);
    char fallback[24];
    std::snprintf(fallback, sizeof(fallback), "ACAD_%X", p.handleRef);
    e->extData[p.indexInExtData]->addString(1001, it != appIdmap.end() &&
                                                          it->second != nullptr
                                                      ? it->second->name
                                                      : std::string{fallback});
  }
  e->pendingAppIdResolutions.clear();

  for (const auto &p : e->pendingLayerRefResolutions) {
    if (p.indexInExtData >= e->extData.size() ||
        e->extData[p.indexInExtData] == nullptr)
      continue;
    const std::string name = findTableName(DRW::LAYER, p.handleRef);
    if (!name.empty())
      e->extData[p.indexInExtData]->setLayerRefName(name);
  }
  e->pendingLayerRefResolutions.clear();
}

std::string dwgReader::findTableName(DRW::TTYPE table, std::int32_t handle) {
  std::string name;
  switch (table) {
  case DRW::STYLE: {
    auto st_it = stylemap.find(handle);
    if (st_it != stylemap.end())
      name = (st_it->second)->name;
    break;
  }
  case DRW::DIMSTYLE: {
    auto ds_it = dimstylemap.find(handle);
    if (ds_it != dimstylemap.end())
      name = (ds_it->second)->name;
    break;
  }
  case DRW::BLOCK_RECORD: { // use DRW_Block because name are more correct
    //        auto bk_it = blockmap.find(handle);
    //        if (bk_it != blockmap.end())
    auto bk_it = blockRecordmap.find(handle);
    if (bk_it != blockRecordmap.end())
      name = (bk_it->second)->name;
    break;
  }
    /*    case DRW::VPORT:{
            auto vp_it = vportmap.find(handle);
            if (vp_it != vportmap.end())
                name = (vp_it->second)->name;
            break;}*/
  case DRW::LAYER: {
    auto ly_it = layermap.find(handle);
    if (ly_it != layermap.end())
      name = (ly_it->second)->name;
    break;
  }
  case DRW::LTYPE: {
    auto lt_it = ltypemap.find(handle);
    if (lt_it != ltypemap.end())
      name = (lt_it->second)->name;
    break;
  }
  default:
    break;
  }
  return name;
}

bool dwgReader::readDwgHeader(DRW_Header &hdr, dwgBuffer *buf,
                              dwgBuffer *hBuf) {
  // The R2010+ bitsize_hi gate inside parseDwg keys off the APP maintenance
  // version (byte 0x12), not byte 0x0B — see appMaintenanceVersion in
  // dwgreader.h.
  bool ret = hdr.parseDwg(version, buf, hBuf, appMaintenanceVersion);
  // RLZ: copy objectControl handles
  return ret;
}

bool dwgReader::checkSentinel(dwgBuffer *buf, enum secEnum::DWGSection sec,
                              bool start) {
  if (buf == nullptr || !buf->isGood())
    return false;
  std::uint8_t readBytes[16];
  for (int i = 0; i < 16; i++) {
    readBytes[i] = buf->getRawChar8();
    DRW_DBGH(readBytes[i]);
    DRW_DBG(" ");
  }
  if (!buf->isGood())
    return false;
  const std::uint8_t *expected = nullptr;
  switch (sec) {
  case secEnum::FILEHEADER:
    if (!start)
      expected = dwgSentinels::FILE_HEADER_END;
    break;
  case secEnum::HEADER:
    expected = start ? dwgSentinels::HEADER_BEGIN : dwgSentinels::HEADER_END;
    break;
  case secEnum::CLASSES:
    expected = start ? dwgSentinels::CLASSES_BEGIN : dwgSentinels::CLASSES_END;
    break;
  default:
    break;
  }
  if (expected != nullptr) {
    for (int i = 0; i < 16; i++) {
      if (readBytes[i] != expected[i]) {
        DRW_DBG("\ncheckSentinel: mismatch at byte ");
        DRW_DBG(i);
        DRW_DBG(" got ");
        DRW_DBGH(readBytes[i]);
        DRW_DBG(" expected ");
        DRW_DBGH(expected[i]);
        DRW_DBG("\n");
        return false;
      }
    }
  }
  return true;
}

bool dwgReader::readDwgClassesTail(dwgBuffer &buffer) {
  const std::uint64_t start = buffer.getPosition();
  if (start > buffer.size() || buffer.size() - start < 16)
    return false;

  // Prefer the current eight-byte form, then the legacy R2007 two-byte
  // field, and finally the R2004 no-tail form.  Probe each candidate without
  // poisoning the publishing cursor when a candidate is not the file's
  // actual trailer layout.
  constexpr std::array<std::size_t, 3> candidateSizes = {8, 2, 0};
  for (const std::size_t tailSize : candidateSizes) {
    if (tailSize > buffer.size() - start ||
        buffer.size() - start - tailSize < 16)
      continue;
    dwgBuffer probe = buffer.forkIndependent();
    if (!probe.setPosition(start + tailSize))
      continue;
    probe.setBitPos(0);
    if (!checkSentinel(&probe, secEnum::CLASSES, false) || !probe.isGood())
      continue;
    if (!buffer.setPosition(start + tailSize))
      return false;
    buffer.setBitPos(0);
    return true;
  }

  // Preserve the historical warn-only end-sentinel policy for a malformed
  // but bounded trailer.  The caller will still inspect the sentinel; this
  // fallback only prevents a compatibility probe from turning a warning
  // into a hard cursor failure.
  if (!buffer.setPosition(start))
    return false;
  buffer.setBitPos(0);
  return true;
}

/*********** objects map ************************/
/** Note: object map are split in sections with max size 2035?
 *  heach section are 2 bytes size + data bytes + 2 bytes crc
 *  size value are data bytes + 2 and to calculate crc are used
 *  2 bytes size + data bytes
 *  last section are 2 bytes size + 2 bytes crc (size value always 2)
 **/
bool dwgReader::readDwgHandles(dwgBuffer *dbuf, std::uint64_t offset,
                               std::uint64_t size, std::uint64_t locationLimit,
                               DwgIntegrityAddressSpace offsetSpace,
                               std::int32_t sectionDescriptorId) {
  DRW_DBG("\ndwgReader::readDwgHandles\n");
  m_dwgSourceFrameLedger.clear();
  m_dwgSourceFrameIndexes.clear();
  m_dwgFramePhaseSnapshots.clear();
  m_dwgFrameCoverageStatus = DRW_DwgFrameCoverageStatus::NotAvailable;
  m_dwgFrameCoverageIntegrityViolation = false;
  m_dwgFrameCoveragePublished = false;
  const auto recordFailure =
      [&](DwgIntegrityCheckKind kind, std::uint64_t location = 0,
          bool hasLocation = false, std::uint64_t expected = 0,
          std::uint64_t observed = 0, bool hasValues = false) {
        DwgIntegrityDiagnostic diagnostic;
        diagnostic.severity = DwgIntegritySeverity::Error;
        diagnostic.offsetSpace = offsetSpace;
        diagnostic.phase = DwgIntegrityPhase::ObjectMap;
        diagnostic.kind = kind;
        diagnostic.logicalSectionId = secEnum::HANDLES;
        diagnostic.sectionDescriptorId = sectionDescriptorId;
        if (hasLocation && offsetSpace != DwgIntegrityAddressSpace::None) {
          diagnostic.fileOffset = location;
          diagnostic.hasFileOffset = true;
        }
        if (hasValues) {
          diagnostic.expected = expected;
          diagnostic.observed = observed;
          diagnostic.hasExpected = true;
          diagnostic.hasObserved = true;
        }
        addIntegrityDiagnostic(std::move(diagnostic));
      };
  if (dbuf == nullptr || size > dwgSafety::MaxBufferSize ||
      size > dbuf->size() || offset > dbuf->size() - size ||
      size > std::numeric_limits<std::size_t>::max()) {
    recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, offset, true);
    return false;
  }
  if (!dbuf->setPosition(offset)) {
    recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, offset, true);
    return false;
  }

  std::uint64_t maxPos = offset + size;
  DRW_DBG("\nSection HANDLES offset= ");
  DRW_DBG(offset);
  DRW_DBG("\nSection HANDLES size= ");
  DRW_DBG(size);
  DRW_DBG("\nSection HANDLES maxPos= ");
  DRW_DBG(maxPos);

  // Each entry is >= 2 bytes (a 1-byte-minimum modular-char handle delta +
  // a 1-byte-minimum modular-char location delta), so size/2 is a safe
  // upper bound on entry count. Avoids repeated rehashing while the loop
  // below fills ObjectMap -- large DWGs have hundreds of thousands of
  // handles.
  std::unordered_map<std::uint32_t, objHandle> stagedMap;
  std::unordered_set<std::uint32_t> stagedOffsets;
  std::vector<objHandle> stagedEntries;
  if (size / 2 > static_cast<std::uint64_t>(std::numeric_limits<int>::max()) ||
      !DRW::reserve(stagedMap, static_cast<int>(size / 2)) ||
      !DRW::reserve(stagedOffsets, static_cast<int>(size / 2)) ||
      !DRW::reserve(stagedEntries, static_cast<int>(size / 2))) {
    recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, offset, true);
    return false;
  }

  std::uint64_t startPos = offset;
  bool end = false;
  bool sawDataGroup = false;
  std::uint16_t terminatorCrc = 0;
  std::vector<std::uint8_t> tmpByteStr;
  /* According to Open Design Specification for .dwg files Version 5.4.1
   * chapter 23.1 (page 251), section list is terminated by empty section
   * (section consisting only of the checksum). When we find, we finish
   * reading sections.
   */
  while (!end) {
    // The group size is part of the group and is followed by a two-byte
    // CRC. Keep both reads inside the HANDLES section boundary; the
    // backing buffer can contain later DWG sections.
    if (startPos > maxPos || maxPos - startPos < 2 ||
        !dbuf->setPosition(startPos)) {
      recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, startPos, true);
      return false;
    }
    DRW_DBG("\nstart handles section buf->curPosition()= ");
    DRW_DBG(dbuf->getPosition());
    DRW_DBG("\n");
    std::uint16_t pageSize = dbuf->getBERawShort16();
    DRW_DBG("object map section size= ");
    DRW_DBG(pageSize);
    DRW_DBG("\n");
    if (!dbuf->isGood() || pageSize < 2 ||
        pageSize > dwgSafety::MaxHandleMapGroupSize ||
        pageSize > maxPos - startPos - 2) {
      recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, startPos, true, 2,
                    pageSize, true);
      DRW_DBG("object map section size out of range\n");
      return false;
    }
    if (!dbuf->setPosition(startPos)) {
      recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, startPos, true);
      return false;
    }
    if (!DRW::resize(tmpByteStr, pageSize)) {
      recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, startPos, true);
      return false;
    }
    if (!dbuf->getBytes(tmpByteStr.data(), pageSize)) {
      recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, startPos, true);
      return false;
    }
    if (!dbuf->isGood()) {
      recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, startPos, true);
      return false;
    }
    dwgBuffer buff(tmpByteStr.data(), pageSize, &decoder);
    if (pageSize != 2) {
      sawDataGroup = true;
      if (!buff.setPosition(2)) {
        recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, startPos, true);
        return false;
      }
      std::uint64_t lastHandle = 0;
      std::int64_t lastLoc = 0;
      // read data
      while (buff.getPosition() < pageSize) {
        std::uint64_t prevPos = buff.getPosition();
        const std::uint64_t encodedHandleDelta = buff.getUModularChar();
        const std::int64_t locationDelta = buff.getModularChar();
        const bool locationOverflow =
            (locationDelta < 0 &&
             lastLoc <
                 std::numeric_limits<std::int64_t>::min() - locationDelta) ||
            (locationDelta > 0 &&
             lastLoc >
                 std::numeric_limits<std::int64_t>::max() - locationDelta);
        if (!buff.isGood() || buff.getPosition() <= prevPos ||
            encodedHandleDelta == 0 ||
            encodedHandleDelta > std::numeric_limits<std::uint32_t>::max() ||
            lastHandle > std::numeric_limits<std::uint32_t>::max() -
                             encodedHandleDelta ||
            locationOverflow) {
          recordFailure(DwgIntegrityCheckKind::ObjectMapProgress,
                        startPos + prevPos, true);
          return false;
        }
        const auto handleDelta = static_cast<std::uint32_t>(encodedHandleDelta);
        lastHandle += handleDelta;
        lastLoc += locationDelta;
        DRW_DBG("object map lastHandle= ");
        DRW_DBGH(static_cast<std::uint32_t>(lastHandle));
        DRW_DBG(" lastLoc= ");
        DRW_DBG(static_cast<long long>(lastLoc));
        DRW_DBG("\n");
        if (!buff.isGood() || buff.getPosition() <= prevPos) {
          recordFailure(DwgIntegrityCheckKind::ObjectMapProgress,
                        startPos + prevPos, true);
          return false;
        }
        if (lastHandle == 0 || lastLoc < 0 ||
            lastLoc > std::numeric_limits<std::uint32_t>::max() ||
            (locationLimit != std::numeric_limits<std::uint64_t>::max() &&
             static_cast<std::uint64_t>(lastLoc) >= locationLimit)) {
          recordFailure(DwgIntegrityCheckKind::ObjectMapProgress,
                        startPos + prevPos, true);
          return false;
        }
        const auto handleKey = static_cast<std::uint32_t>(lastHandle);
        if (ObjectMap.find(handleKey) != ObjectMap.end() ||
            stagedMap.find(handleKey) != stagedMap.end()) {
          recordFailure(DwgIntegrityCheckKind::ObjectMapProgress,
                        startPos + prevPos, true, 0, handleKey, true);
          DRW_DBG("duplicate object-map handle\n");
          return false;
        }
        const auto objectOffset = static_cast<std::uint32_t>(lastLoc);
        if (!stagedOffsets.insert(objectOffset).second) {
          recordFailure(DwgIntegrityCheckKind::ObjectMapDuplicateOffset,
                        startPos + prevPos, true, 0, objectOffset, true);
          DRW_DBG("duplicate object-map offset\n");
          return false;
        }
        const objHandle entry(0, handleKey, objectOffset,
                              static_cast<std::uint64_t>(stagedEntries.size()),
                              frameOffsetSpace(offsetSpace));
        stagedMap.emplace(handleKey, entry);
        stagedEntries.push_back(entry);
      }
    } else {
      end = true;
    }
    // verify crc
    std::uint16_t crcCalc = buff.crc8(0xc0c1, 0, pageSize);
    std::uint16_t crcRead = dbuf->getBERawShort16();
    DRW_DBG("object map section crc8 read= ");
    DRW_DBG(crcRead);
    DRW_DBG("\nobject map section crc8 calculated= ");
    DRW_DBG(crcCalc);
    DRW_DBG("\nobject section buf->curPosition()= ");
    DRW_DBG(dbuf->getPosition());
    DRW_DBG("\n");
    if (!dbuf->isGood()) {
      recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, startPos, true);
      return false;
    }
    if (crcCalc != crcRead) {
      recordFailure(DwgIntegrityCheckKind::ObjectMapCrc, startPos, true,
                    crcRead, crcCalc, true);
      return false;
    }
    if (end)
      terminatorCrc = crcCalc;
    startPos = dbuf->getPosition();
  }

  if (!end || !sawDataGroup || stagedMap.empty() || !dbuf->isGood()) {
    recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, startPos, true);
    return false;
  }

  // Some R2010 files retain a duplicate empty-page trailer after the
  // HANDLES terminator, inside the section's declared decompressed size.
  // Accept only a second terminator with the checksum calculated for 00 02;
  // arbitrary trailing bytes remain an error.
  if (maxPos - startPos == 4) {
    const std::array<std::uint8_t, 4> trailer = {
        dbuf->getRawChar8(), dbuf->getRawChar8(), dbuf->getRawChar8(),
        dbuf->getRawChar8()};
    if (!dbuf->isGood()) {
      recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, startPos, true);
      return false;
    }
    const std::uint16_t trailerCrc =
        static_cast<std::uint16_t>(trailer[2] << 8 | trailer[3]);
    if (trailer[0] == 0 && trailer[1] == 2 && trailerCrc == terminatorCrc) {
      startPos = maxPos;
    } else {
      recordFailure(DwgIntegrityCheckKind::ObjectMapCrc, startPos, true);
      return false;
    }
  }
  if (startPos != maxPos) {
    recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, startPos, true,
                  maxPos, startPos, true);
    return false;
  }
  const auto objectMapSize = ObjectMap.size();
  const auto stagedMapSize = stagedMap.size();
  const auto maxContainerSize =
      static_cast<std::size_t>(std::numeric_limits<int>::max());
  if (objectMapSize > maxContainerSize || stagedMapSize > maxContainerSize ||
      objectMapSize > maxContainerSize - stagedMapSize) {
    recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, startPos, true);
    return false;
  }
  if (!DRW::reserve(ObjectMap,
                    static_cast<int>(objectMapSize + stagedMapSize))) {
    recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, startPos, true);
    return false;
  }
  std::unordered_map<std::uint32_t, std::size_t> stagedFrameIndexes;
  std::vector<DRW_DwgFrameCoverageEntry> stagedFrameLedger;
  if (stagedEntries.size() > maxContainerSize ||
      !DRW::reserve(stagedFrameIndexes,
                    static_cast<int>(stagedEntries.size())) ||
      !DRW::reserve(stagedFrameLedger,
                    static_cast<int>(stagedEntries.size()))) {
    recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, startPos, true);
    return false;
  }
  for (const objHandle &entry : stagedEntries) {
    DRW_DwgFrameCoverageEntry frameEntry;
    frameEntry.m_handle = entry.handle;
    frameEntry.m_sourceOffset = entry.loc;
    frameEntry.m_sourceMapOrdinal = entry.sourceOrdinal;
    frameEntry.m_sourceOffsetSpace = entry.sourceOffsetSpace;
    const std::size_t index = stagedFrameLedger.size();
    if (!stagedFrameIndexes.emplace(entry.handle, index).second) {
      recordFailure(DwgIntegrityCheckKind::ObjectMapProgress, startPos, true, 0,
                    entry.handle, true);
      return false;
    }
    stagedFrameLedger.push_back(frameEntry);
  }
  for (const auto &entry : stagedMap)
    ObjectMap.emplace(entry.first, entry.second);
  m_dwgSourceFrameLedger = std::move(stagedFrameLedger);
  m_dwgSourceFrameIndexes = std::move(stagedFrameIndexes);
  m_dwgFrameCoverageStatus = DRW_DwgFrameCoverageStatus::InProgress;
  m_dwgFrameCoveragePublished = false;
  return true;
}

/*********** objects ************************/
/**
 * Reads all the object referenced in the object map section of the DWG file
 * (using their object file offsets)
 */
bool dwgReader::readDwgTables(DRW_Header &hdr, dwgBuffer *dbuf,
                              DwgIntegrityAddressSpace offsetSpace) {
  DRW_DBG("\ndwgReader::readDwgTables start\n");
  bool ret = true;
  bool ret2 = true;
  objHandle oc;
  std::vector<DRW_UnsupportedObject> stagedRawControls;
  std::vector<DRW_DwgFramePublication> stagedTableFramePublications;
  std::unordered_set<std::uint32_t> stagedRawControlHandles;
  std::unordered_set<std::uint32_t> claimedTableHandles;
  struct StagedTableMapEntry {
    DwgObjectMap::node_type node;
    DwgSourceFrameId source;
    DRW_DwgFrameDisposition disposition{DRW_DwgFrameDisposition::Pending};
    DRW_DwgFrameCoverageReason reason{DRW_DwgFrameCoverageReason::None};
    std::uint32_t publicationCount{0};
    bool hasCoverage{false};
  };
  std::vector<StagedTableMapEntry> erasedObjectHandles;
  const bool hasFrameCoverage =
      m_dwgFrameCoverageStatus != DRW_DwgFrameCoverageStatus::NotAvailable;
  try {
    erasedObjectHandles.reserve(ObjectMap.size());
  } catch (...) {
    return false;
  }
  const auto clearTableState = [&] {
    const auto clearMap = [](auto &table) {
      mapCleanUp(table);
      table.clear();
    };
    clearMap(ltypemap);
    clearMap(layermap);
    clearMap(stylemap);
    clearMap(dimstylemap);
    clearMap(vportmap);
    clearMap(blockRecordmap);
    clearMap(appIdmap);
    clearMap(viewmap);
    clearMap(ucsmap);
    m_layerNameOrder.clear();
    m_ltypeNameOrder.clear();
    m_deferredRawObjects.clear();
    m_deferredTableFramePublications.clear();
  };
  const auto eraseObject = [&](DwgObjectMap::iterator it) {
    if (it == ObjectMap.end()) {
      ret = false;
      return;
    }

    DwgSourceFrameLease lease;
    if (!borrowDwgSourceFrame(ObjectMap, it, lease)) {
      ret = false;
      return;
    }
    // Capacity is reserved for every currently mapped handle before the
    // table phase begins, so stage the metadata before detaching the node.
    erasedObjectHandles.emplace_back();
    StagedTableMapEntry &staged = erasedObjectHandles.back();
    staged.source = lease.source;
    staged.hasCoverage = lease.hasCoverage;
    if (hasFrameCoverage) {
      const auto sourceIt = m_dwgSourceFrameIndexes.find(lease.source.handle);
      if (sourceIt == m_dwgSourceFrameIndexes.end() ||
          sourceIt->second >= m_dwgSourceFrameLedger.size()) {
        erasedObjectHandles.pop_back();
        ret = false;
        return;
      }
      const DRW_DwgFrameCoverageEntry &entry =
          m_dwgSourceFrameLedger[sourceIt->second];
      staged.disposition = entry.m_disposition;
      staged.reason = entry.m_reason;
      staged.publicationCount = entry.m_publicationCount;
    }

    staged.node = ObjectMap.extract(it);

    if (hasFrameCoverage &&
        !markDwgFrameOutcome(lease.source, DRW_DwgFrameDisposition::Deferred,
                             DRW_DwgFrameCoverageReason::TableDeferred)) {
      ObjectMap.insert(std::move(erasedObjectHandles.back().node));
      erasedObjectHandles.pop_back();
      ret = false;
    }
  };
  const auto restoreErasedObjects = [&] {
    bool restored = true;
    for (StagedTableMapEntry &entry : erasedObjectHandles) {
      const auto inserted = ObjectMap.insert(std::move(entry.node));
      if (!inserted.inserted) {
        restored = false;
        continue;
      }
      if (!entry.hasCoverage)
        continue;
      const auto sourceIt = m_dwgSourceFrameIndexes.find(entry.source.handle);
      if (sourceIt == m_dwgSourceFrameIndexes.end() ||
          sourceIt->second >= m_dwgSourceFrameLedger.size()) {
        restored = false;
        continue;
      }
      DRW_DwgFrameCoverageEntry &sourceEntry =
          m_dwgSourceFrameLedger[sourceIt->second];
      const DwgSourceFrameId expected{
          sourceEntry.m_handle, sourceEntry.m_sourceOffset,
          sourceEntry.m_sourceMapOrdinal, sourceEntry.m_sourceOffsetSpace};
      if (!(entry.source == expected) ||
          sourceEntry.m_disposition != DRW_DwgFrameDisposition::Deferred ||
          sourceEntry.m_publicationCount != 0) {
        restored = false;
        continue;
      }
      sourceEntry.m_disposition = entry.disposition;
      sourceEntry.m_reason = entry.reason;
      sourceEntry.m_publicationCount = entry.publicationCount;
    }
    return restored;
  };

  // A reader can be retried after a malformed table phase. Do not let either
  // prior table entries or handles consumed by this failed attempt escape.
  clearTableState();
  const auto abortTablePhase = [&] {
    clearTableState();
    (void)restoreErasedObjects();
    return false;
  };
  const auto parseControl = [&](const objHandle &object,
                                const DwgTableDescriptor &descriptor,
                                DRW_ObjControl &control) {
    DwgObjectFrame frame;
    if (!frame.readAt(*dbuf, version, object.loc)) {
      recordObjectFrameFailure(object, offsetSpace);
      return false;
    }
    dwgBuffer buffer(frame.body().data(), frame.body().size(), &decoder);
    if (buffer.getObjType(version) != descriptor.controlType ||
        !buffer.isGood())
      return false;
    buffer.resetPosition();
    if (!control.parseDwg(version, &buffer, frame.bodyBitSize()) ||
        !buffer.isGood() || control.handle != object.handle)
      return false;
    if (version <= DRW::AC1018)
      return true;

    // Control-object handle lists use their own null/xdictionary/child
    // schema; they are not the standard object owner/reactor tail.
    return true;
  };
  const auto parseRawControl = [&](const objHandle &object,
                                   std::int16_t expectedType,
                                   DRW_ObjControl &control) {
    DwgObjectFrame frame;
    if (!frame.readAt(*dbuf, version, object.loc)) {
      recordObjectFrameFailure(object, offsetSpace);
      return false;
    }
    dwgBuffer buffer(frame.body().data(), frame.body().size(), &decoder);
    if (buffer.getObjType(version) != expectedType || !buffer.isGood())
      return false;
    buffer.resetPosition();
    if (!control.parseDwg(version, &buffer, frame.bodyBitSize()) ||
        !buffer.isGood() || control.handle != object.handle)
      return false;
    if (version <= DRW::AC1018)
      return true;

    // Control-object handle lists use their own null/xdictionary/child
    // schema; they are not the standard object owner/reactor tail.
    return true;
  };
  const auto claimControlHandles = [&](const DRW_ObjControl &control) {
    for (const std::uint32_t handle : control.handlesList) {
      if (ObjectMap.find(handle) == ObjectMap.end()) {
        DRW_DBG("WARNING: control handle not found ");
        DRW_DBGH(handle);
        DRW_DBG("\\n");
        return false;
      }
      if (!claimedTableHandles.insert(handle).second) {
        return false;
      }
    }
    return true;
  };
  const auto stageControlReceipt = [&](const objHandle &object,
                                       const DwgTableDescriptor &descriptor,
                                       const DRW_ObjControl &control) {
    const auto sourceIt =
        std::find_if(erasedObjectHandles.crbegin(), erasedObjectHandles.crend(),
                     [&object](const auto &erased) {
                       return erased.source.handle == object.handle;
                     });
    if (sourceIt == erasedObjectHandles.crend())
      return false;

    try {
      DRW_DwgFramePublication publication;
      publication.m_version = version;
      publication.m_handle = sourceIt->source.handle;
      publication.m_sourceOffset = sourceIt->source.offset;
      publication.m_sourceMapOrdinal = sourceIt->source.ordinal;
      publication.m_sourceOffsetSpace = sourceIt->source.offsetSpace;
      publication.m_hasSourceLocation = true;
      publication.m_encodedType = descriptor.controlType;
      publication.m_resolvedType = descriptor.controlType;
      publication.m_isEntity = false;
      publication.m_recordName = descriptor.controlReceiptName;
      publication.setCommonLinkEvidence(
          DRW_DwgCommonLinkEvidence::NotApplicable);
      publication.m_controlHandles.assign(control.handlesList.cbegin(),
                                          control.handlesList.cend());
      publication.m_carrier = DRW_DwgFramePublication::Carrier::Control;
      stagedTableFramePublications.push_back(std::move(publication));
    } catch (...) {
      return false;
    }
    return true;
  };
  const auto parseTableRecord = [&](const objHandle &object,
                                    const DwgTableDescriptor &descriptor,
                                    auto &record) {
    DwgObjectFrame frame;
    if (!frame.readAt(*dbuf, version, object.loc)) {
      recordObjectFrameFailure(object, offsetSpace);
      return false;
    }
    dwgBuffer typeBuffer(frame.body().data(), frame.body().size(), &decoder);
    if (typeBuffer.getObjType(version) != descriptor.recordType ||
        !typeBuffer.isGood()) {
      return false;
    }
    dwgBuffer buffer(frame.body().data(), frame.body().size(), &decoder);
    if (!record->parseDwg(version, &buffer, frame.bodyBitSize()) ||
        !buffer.isGood() || record->handle != object.handle)
      return false;
    if (version <= DRW::AC1018)
      return true;

    RawObjectShell links;
    dwgBuffer linkBuffer(frame.body().data(), frame.body().size(), &decoder);
    if (!links.parseDwg(version, &linkBuffer, frame.bodyBitSize()) ||
        !linkBuffer.isGood() || !links.hasDwgCommonLinkTail() ||
        links.handle != object.handle)
      return false;
    record->parentHandle = links.parentHandle;
    record->reactorHandles = links.reactorHandles;
    record->xDictHandle = links.xDictHandle;
    record->setDwgCommonObjectState(links.reactorCount(),
                                    links.extensionDictionaryFlag(),
                                    links.hasDataStorageBinaryData());
    record->setDwgCommonLinkTailValidated(true);
    return true;
  };
  const auto addRawControl = [&](const objHandle &object,
                                 std::int16_t objectType) {
    if (object.handle == DRW::NoHandle ||
        !stagedRawControlHandles.insert(object.handle).second)
      return false;
    DwgObjectFrame frame;
    if (!frame.readAt(*dbuf, version, object.loc)) {
      recordObjectFrameFailure(object, offsetSpace);
      return false;
    }

    dwgBuffer typeBuffer(frame.body().data(), frame.body().size(), &decoder);
    if (typeBuffer.getObjType(version) != objectType || !typeBuffer.isGood())
      return false;

    DRW_UnsupportedObject raw;
    raw.m_version = version;
    raw.m_objectType = objectType;
    raw.m_handle = object.handle;
    raw.m_bodyBitSize = frame.bodyBitSize();
    raw.m_objectOffset = object.loc;
    raw.m_objectSize = static_cast<std::uint32_t>(frame.body().size());
    raw.m_rawBytes = frame.body();
    stagedRawControls.push_back(std::move(raw));
    return true;
  };
  const auto insertTableRecord = [&](auto &map, auto record,
                                     const char *recordName,
                                     std::uint32_t expectedHandle,
                                     std::int16_t recordType) {
    if (record == nullptr || record->handle != expectedHandle) {
      DRW_DBG("mismatched ");
      DRW_DBG(recordName);
      DRW_DBG(" handle: ");
      DRW_DBGH(record == nullptr ? 0 : record->handle);
      DRW_DBG(" expected: ");
      DRW_DBGH(expectedHandle);
      DRW_DBG("\n");
      ret = false;
      return false;
    }
    const auto sourceIt =
        std::find_if(erasedObjectHandles.crbegin(), erasedObjectHandles.crend(),
                     [expectedHandle](const auto &erased) {
                       return erased.source.handle == expectedHandle;
                     });
    if (sourceIt == erasedObjectHandles.crend()) {
      ret = false;
      return false;
    }
    const DRW_TableEntry &tableRecord = *record;
    DRW_DwgFramePublication publication;
    publication.m_version = version;
    publication.m_handle = sourceIt->source.handle;
    publication.m_sourceOffset = sourceIt->source.offset;
    publication.m_sourceMapOrdinal = sourceIt->source.ordinal;
    publication.m_sourceOffsetSpace = sourceIt->source.offsetSpace;
    publication.m_hasSourceLocation = true;
    publication.m_encodedType = recordType;
    publication.m_resolvedType = recordType;
    publication.m_isEntity = false;
    publication.m_recordName = recordName;
    publication.setCommonLinkEvidence(drwDwgCommonLinkEvidenceForLinks(
        tableRecord.hasDwgCommonLinkTail(), tableRecord.parentHandle,
        tableRecord.reactorHandles, tableRecord.reactorCount(),
        tableRecord.xDictHandle));
    publication.m_parentHandle = tableRecord.parentHandle;
    publication.m_reactorHandles = tableRecord.reactorHandles;
    publication.m_xDictHandle = tableRecord.xDictHandle;
    publication.m_numReactors = tableRecord.reactorCount();
    publication.m_xDictFlag = tableRecord.extensionDictionaryFlag();
    publication.m_carrier = DRW_DwgFramePublication::Carrier::Typed;
    try {
      stagedTableFramePublications.push_back(std::move(publication));
    } catch (...) {
      ret = false;
      return false;
    }
    const bool inserted = map.emplace(record->handle, record.get()).second;
    if (!inserted) {
      DRW_DBG("duplicate ");
      DRW_DBG(recordName);
      DRW_DBG(" handle: ");
      DRW_DBGH(record->handle);
      DRW_DBG("\n");
      ret = false;
    } else {
      record.release();
    }
    return inserted;
  };

  // parse linetypes, start with linetype Control
  auto mit = ObjectMap.find(hdr.linetypeCtrl);
  if (mit == ObjectMap.end()) {
    DRW_DBG("\nWARNING: LineType control not found\n");
    ret = false;
  } else {
    DRW_DBG("\n**********Parsing LineType control*******\n");
    oc = mit->second;
    eraseObject(mit);
    DRW_ObjControl ltControl;
    ret2 = parseControl(oc, kLTypeTable, ltControl);
    ret2 = ret2 && claimControlHandles(ltControl);
    ret2 = ret2 && stageControlReceipt(oc, kLTypeTable, ltControl);
    if (!ret2) {
      ltControl.handlesList.clear();
      DRW_DBG("\nWARNING: LineType control parse failed\n");
    }
    if (ret)
      ret = ret2;
    for (auto it = ltControl.handlesList.begin();
         it != ltControl.handlesList.end(); ++it) {
      mit = ObjectMap.find(*it);
      if (mit == ObjectMap.end()) {
        DRW_DBG("\nWARNING: LineType not found\n");
        m_ltypeNameOrder.emplace_back(); // keep proxy index alignment
      } else {
        oc = mit->second;
        eraseObject(mit);
        DRW_DBG("\nLineType Handle= ");
        DRW_DBGH(oc.handle);
        DRW_DBG(" loc.: ");
        DRW_DBG(oc.loc);
        DRW_DBG("\n");
        auto lt = std::make_unique<DRW_LType>();
        ret2 = parseTableRecord(oc, kLTypeTable, lt);
        ret = ret && ret2;
        if (ret2) {
          const std::string ltypeName = lt->name;
          if (insertTableRecord(ltypemap, std::move(lt), "linetype", oc.handle,
                                kLTypeTable.recordType))
            m_ltypeNameOrder.push_back(ltypeName); // proxy op18 index space
          else
            m_ltypeNameOrder.emplace_back();
        } else {
          m_ltypeNameOrder.emplace_back(); // keep proxy index alignment
          DRW_DBG(
              "\nWARNING: LineType record parseDwg failed (handle skipped)\n");
        }
      }
    }
  }

  if (!ret)
    return abortTablePhase();

  // parse layers, start with layer Control
  mit = ObjectMap.find(hdr.layerCtrl);
  if (mit == ObjectMap.end()) {
    DRW_DBG("\nWARNING: Layer control not found\n");
    ret = false;
  } else {
    DRW_DBG("\n**********Parsing Layer control*******\n");
    oc = mit->second;
    eraseObject(mit);
    DRW_ObjControl layControl;
    ret2 = parseControl(oc, kLayerTable, layControl);
    ret2 = ret2 && claimControlHandles(layControl);
    ret2 = ret2 && stageControlReceipt(oc, kLayerTable, layControl);
    if (!ret2) {
      layControl.handlesList.clear();
      DRW_DBG("\nWARNING: Layer control parse failed\n");
    }
    if (ret)
      ret = ret2;
    for (auto it = layControl.handlesList.begin();
         it != layControl.handlesList.end(); ++it) {
      mit = ObjectMap.find(*it);
      if (mit == ObjectMap.end()) {
        DRW_DBG("\nWARNING: Layer not found (handle skipped)\n");
        m_layerNameOrder.emplace_back(); // keep proxy index alignment
      } else {
        oc = mit->second;
        eraseObject(mit);
        DRW_DBG("Layer Handle= ");
        DRW_DBGH(oc.handle);
        DRW_DBG(" ");
        DRW_DBG(oc.loc);
        DRW_DBG("\n");
        auto la = std::make_unique<DRW_Layer>();
        ret2 = parseTableRecord(oc, kLayerTable, la);
        ret = ret && ret2;
        if (ret2) {
          const std::string layerName = la->name;
          if (insertTableRecord(layermap, std::move(la), "layer", oc.handle,
                                kLayerTable.recordType))
            m_layerNameOrder.push_back(layerName); // proxy op16 index space
          else
            m_layerNameOrder.emplace_back();
        } else {
          m_layerNameOrder.emplace_back(); // keep proxy index alignment
          DRW_DBG("\nWARNING: Layer record parseDwg failed (handle skipped)\n");
        }
      }
    }
  }

  if (!ret)
    return abortTablePhase();

  // set linetype in layer
  for (auto it = layermap.begin(); it != layermap.end(); ++it) {
    DRW_Layer *ly = it->second;
    std::uint32_t ref = ly->lTypeH.ref;
    auto lt_it = ltypemap.find(ref);
    if (lt_it != ltypemap.end()) {
      ly->lineType = (lt_it->second)->name;
    }
  }

  // parse text styles, start with style Control
  mit = ObjectMap.find(hdr.styleCtrl);
  if (mit == ObjectMap.end()) {
    DRW_DBG("\nWARNING: Style control not found\n");
    ret = false;
  } else {
    DRW_DBG("\n**********Parsing Style control*******\n");
    oc = mit->second;
    eraseObject(mit);
    DRW_ObjControl styControl;
    ret2 = parseControl(oc, kStyleTable, styControl);
    ret2 = ret2 && claimControlHandles(styControl);
    ret2 = ret2 && stageControlReceipt(oc, kStyleTable, styControl);
    if (!ret2) {
      styControl.handlesList.clear();
      DRW_DBG("\nWARNING: Text Style control parse failed\n");
    }
    if (ret)
      ret = ret2;
    for (auto it = styControl.handlesList.begin();
         it != styControl.handlesList.end(); ++it) {
      mit = ObjectMap.find(*it);
      if (mit == ObjectMap.end()) {
        DRW_DBG("\nWARNING: Style not found (handle skipped)\n");
      } else {
        oc = mit->second;
        eraseObject(mit);
        DRW_DBG("Style Handle= ");
        DRW_DBGH(oc.handle);
        DRW_DBG(" ");
        DRW_DBG(oc.loc);
        DRW_DBG("\n");
        auto sty = std::make_unique<DRW_Textstyle>();
        ret2 = parseTableRecord(oc, kStyleTable, sty);
        ret = ret && ret2;
        if (ret2) {
          insertTableRecord(stylemap, std::move(sty), "text style", oc.handle,
                            kStyleTable.recordType);
        } else {
          DRW_DBG("\nWARNING: Style record parseDwg failed (handle skipped)\n");
        }
      }
    }
  }

  if (!ret)
    return abortTablePhase();

  // parse dim styles, start with dimstyle Control
  mit = ObjectMap.find(hdr.dimstyleCtrl);
  if (mit == ObjectMap.end()) {
    DRW_DBG("\nWARNING: Dimension Style control not found\n");
    ret = false;
  } else {
    DRW_DBG("\n**********Parsing Dimension Style control*******\n");
    oc = mit->second;
    eraseObject(mit);
    DRW_ObjControl dimstyControl;
    ret2 = parseControl(oc, kDimStyleTable, dimstyControl);
    ret2 = ret2 && claimControlHandles(dimstyControl);
    ret2 = ret2 && stageControlReceipt(oc, kDimStyleTable, dimstyControl);
    if (!ret2) {
      dimstyControl.handlesList.clear();
      DRW_DBG("\nWARNING: Dimension Style control parse failed\n");
    }
    if (ret)
      ret = ret2;
    for (auto it = dimstyControl.handlesList.begin();
         it != dimstyControl.handlesList.end(); ++it) {
      mit = ObjectMap.find(*it);
      if (mit == ObjectMap.end()) {
        DRW_DBG("\nWARNING: Dimension Style not found (handle skipped)\n");
      } else {
        oc = mit->second;
        eraseObject(mit);
        DRW_DBG("Dimstyle Handle= ");
        DRW_DBGH(oc.handle);
        DRW_DBG(" ");
        DRW_DBG(oc.loc);
        DRW_DBG("\n");
        auto sty = std::make_unique<DRW_Dimstyle>();
        ret2 = parseTableRecord(oc, kDimStyleTable, sty);
        ret = ret && ret2;
        if (ret2) {
          insertTableRecord(dimstylemap, std::move(sty), "dimension style",
                            oc.handle, kDimStyleTable.recordType);
        } else {
          DRW_DBG("\nWARNING: Dimension Style record parseDwg failed (handle "
                  "skipped)\n");
        }
      }
    }
  }

  if (!ret)
    return abortTablePhase();

  // parse vports, start with vports Control
  mit = ObjectMap.find(hdr.vportCtrl);
  if (mit == ObjectMap.end()) {
    DRW_DBG("\nWARNING: vports control not found\n");
    ret = false;
  } else {
    DRW_DBG("\n**********Parsing vports control*******\n");
    oc = mit->second;
    eraseObject(mit);
    DRW_ObjControl vportControl;
    ret2 = parseControl(oc, kVPortTable, vportControl);
    ret2 = ret2 && claimControlHandles(vportControl);
    ret2 = ret2 && stageControlReceipt(oc, kVPortTable, vportControl);
    if (!ret2) {
      vportControl.handlesList.clear();
      DRW_DBG("\nWARNING: VPorts control parse failed\n");
    }
    if (ret)
      ret = ret2;
    for (auto it = vportControl.handlesList.begin();
         it != vportControl.handlesList.end(); ++it) {
      mit = ObjectMap.find(*it);
      if (mit == ObjectMap.end()) {
        DRW_DBG("\nWARNING: vport not found (handle skipped)\n");
      } else {
        oc = mit->second;
        eraseObject(mit);
        DRW_DBG("Vport Handle= ");
        DRW_DBGH(oc.handle);
        DRW_DBG(" ");
        DRW_DBG(oc.loc);
        DRW_DBG("\n");
        auto vp = std::make_unique<DRW_Vport>();
        ret2 = parseTableRecord(oc, kVPortTable, vp);
        ret = ret && ret2;
        if (ret2) {
          insertTableRecord(vportmap, std::move(vp), "viewport", oc.handle,
                            kVPortTable.recordType);
        } else {
          DRW_DBG("\nWARNING: Vport record parseDwg failed (handle skipped)\n");
        }
      }
    }
  }

  if (!ret)
    return abortTablePhase();

  // parse Block_records , start with Block_record Control
  mit = ObjectMap.find(hdr.blockCtrl);
  if (mit == ObjectMap.end()) {
    DRW_DBG("\nWARNING: Block_record control not found\n");
    ret = false;
  } else {
    DRW_DBG("\n**********Parsing Block_record control*******\n");
    oc = mit->second;
    eraseObject(mit);
    DRW_ObjControl blockControl;
    ret2 = parseControl(oc, kBlockTable, blockControl);
    ret2 = ret2 && claimControlHandles(blockControl);
    ret2 = ret2 && stageControlReceipt(oc, kBlockTable, blockControl);
    if (!ret2) {
      blockControl.handlesList.clear();
      DRW_DBG("\nWARNING: Block Record control parse failed\n");
    }
    if (ret)
      ret = ret2;
    for (auto it = blockControl.handlesList.begin();
         it != blockControl.handlesList.end(); ++it) {
      mit = ObjectMap.find(*it);
      if (mit == ObjectMap.end()) {
        DRW_DBG("\nWARNING: block record not found (handle skipped)\n");
      } else {
        oc = mit->second;
        eraseObject(mit);
        DRW_DBG("block record Handle= ");
        DRW_DBGH(oc.handle);
        DRW_DBG(" ");
        DRW_DBG(oc.loc);
        DRW_DBG("\n");
        auto br = std::make_unique<DRW_Block_Record>();
        ret2 = parseTableRecord(oc, kBlockTable, br);
        ret = ret && ret2;
        if (ret2) {
          insertTableRecord(blockRecordmap, std::move(br), "block record",
                            oc.handle, kBlockTable.recordType);
        } else {
          DRW_DBG("\nWARNING: Block_record record parseDwg failed (handle "
                  "skipped)\n");
        }
      }
    }
  }

  if (!ret)
    return abortTablePhase();

  // parse appId , start with appId Control
  mit = ObjectMap.find(hdr.appidCtrl);
  if (mit == ObjectMap.end()) {
    DRW_DBG("\nWARNING: AppId control not found\n");
    ret = false;
  } else {
    DRW_DBG("\n**********Parsing AppId control*******\n");
    oc = mit->second;
    eraseObject(mit);
    DRW_DBG("AppId Control Obj Handle= ");
    DRW_DBGH(oc.handle);
    DRW_DBG(" ");
    DRW_DBG(oc.loc);
    DRW_DBG("\n");
    DRW_ObjControl appIdControl;
    ret2 = parseControl(oc, kAppIdTable, appIdControl);
    ret2 = ret2 && claimControlHandles(appIdControl);
    ret2 = ret2 && stageControlReceipt(oc, kAppIdTable, appIdControl);
    if (!ret2) {
      appIdControl.handlesList.clear();
      DRW_DBG("\nWARNING: AppId control parse failed\n");
    }
    if (ret)
      ret = ret2;
    for (auto it = appIdControl.handlesList.begin();
         it != appIdControl.handlesList.end(); ++it) {
      mit = ObjectMap.find(*it);
      if (mit == ObjectMap.end()) {
        DRW_DBG("\nWARNING: AppId not found (handle skipped)\n");
      } else {
        oc = mit->second;
        eraseObject(mit);
        DRW_DBG("AppId Handle= ");
        DRW_DBGH(oc.handle);
        DRW_DBG(" ");
        DRW_DBG(oc.loc);
        DRW_DBG("\n");
        auto ai = std::make_unique<DRW_AppId>();
        ret2 = parseTableRecord(oc, kAppIdTable, ai);
        ret = ret && ret2;
        if (ret2) {
          insertTableRecord(appIdmap, std::move(ai), "AppId", oc.handle,
                            kAppIdTable.recordType);
        } else {
          DRW_DBG("\nWARNING: AppId record parseDwg failed (handle skipped)\n");
        }
      }
    }
  }

  if (!ret)
    return abortTablePhase();

  // parse View / UCS / VPortEntHeader controls
  // These controls are optional when their header handle is absent, but a
  // present control must decode completely before its records are used.
  mit = ObjectMap.find(hdr.viewCtrl);
  if (mit == ObjectMap.end()) {
    DRW_DBG("\nWARNING: View control not found\n");
  } else {
    DRW_DBG("\n**********Parsing View control*******\n");
    oc = mit->second;
    eraseObject(mit);
    DRW_DBG("View Control Obj Handle= ");
    DRW_DBGH(oc.handle);
    DRW_DBG(" ");
    DRW_DBG(oc.loc);
    DRW_DBG("\n");
    DRW_ObjControl viewControl;
    const bool parsed = parseControl(oc, kViewTable, viewControl);
    const bool claimed = parsed && claimControlHandles(viewControl);
    const bool staged =
        claimed && stageControlReceipt(oc, kViewTable, viewControl);
    if (!staged) {
      DRW_DBG("\nWARNING: View control parse failed\n");
      ret = false;
    } else {
      // per-record loop — populate viewmap so libdwgr.cpp processDwg
      // fires intfa.addView for each named view
      for (auto it = viewControl.handlesList.begin();
           it != viewControl.handlesList.end(); ++it) {
        mit = ObjectMap.find(*it);
        if (mit == ObjectMap.end()) {
          DRW_DBG("\nWARNING: View record not found (handle skipped)\n");
        } else {
          oc = mit->second;
          eraseObject(mit);
          DRW_DBG("View Handle= ");
          DRW_DBGH(oc.handle);
          DRW_DBG(" ");
          DRW_DBG(oc.loc);
          DRW_DBG("\n");
          auto vw = std::make_unique<DRW_View>();
          if (!parseTableRecord(oc, kViewTable, vw)) {
            ret = false;
            DRW_DBG(
                "\nWARNING: View record parseDwg failed (handle skipped)\n");
          } else {
            insertTableRecord(viewmap, std::move(vw), "view", oc.handle,
                              kViewTable.recordType);
          }
        }
      }
    }
  }

  if (!ret)
    return abortTablePhase();

  mit = ObjectMap.find(hdr.ucsCtrl);
  if (mit == ObjectMap.end()) {
    DRW_DBG("\nWARNING: Ucs control not found\n");
  } else {
    oc = mit->second;
    eraseObject(mit);
    DRW_DBG("\n**********Parsing Ucs control*******\n");
    DRW_DBG("Ucs Control Obj Handle= ");
    DRW_DBGH(oc.handle);
    DRW_DBG(" ");
    DRW_DBG(oc.loc);
    DRW_DBG("\n");
    DRW_ObjControl ucsControl;
    const bool parsed = parseControl(oc, kUcsTable, ucsControl);
    const bool claimed = parsed && claimControlHandles(ucsControl);
    const bool staged =
        claimed && stageControlReceipt(oc, kUcsTable, ucsControl);
    if (!staged) {
      DRW_DBG("\nWARNING: Ucs control parse failed\n");
      ret = false;
    } else {
      // per-record loop — populate ucsmap so libdwgr.cpp processDwg
      // fires intfa.addUCS for each named UCS
      for (auto it = ucsControl.handlesList.begin();
           it != ucsControl.handlesList.end(); ++it) {
        mit = ObjectMap.find(*it);
        if (mit == ObjectMap.end()) {
          DRW_DBG("\nWARNING: Ucs record not found (handle skipped)\n");
        } else {
          oc = mit->second;
          eraseObject(mit);
          DRW_DBG("Ucs Handle= ");
          DRW_DBGH(oc.handle);
          DRW_DBG(" ");
          DRW_DBG(oc.loc);
          DRW_DBG("\n");
          auto u = std::make_unique<DRW_UCS>();
          if (!parseTableRecord(oc, kUcsTable, u)) {
            ret = false;
            DRW_DBG("\nWARNING: Ucs record parseDwg failed (handle skipped)\n");
          } else {
            insertTableRecord(ucsmap, std::move(u), "UCS", oc.handle,
                              kUcsTable.recordType);
          }
        }
      }
    }
  }

  if (!ret)
    return abortTablePhase();

  if (version < DRW::AC1018) { // r2000-
    mit = ObjectMap.find(hdr.vpEntHeaderCtrl);
    if (mit == ObjectMap.end()) {
      DRW_DBG("\nWARNING: vpEntHeader control not found\n");
    } else {
      DRW_DBG("\n**********Parsing vpEntHeader control*******\n");
      oc = mit->second;
      DRW_DBG("vpEntHeader Control Obj Handle= ");
      DRW_DBGH(oc.handle);
      DRW_DBG(" ");
      DRW_DBG(oc.loc);
      DRW_DBG("\n");
      DRW_ObjControl vpEntHeaderCtrl;
      const bool parsed = parseRawControl(
          oc, DRW_ViewportEntityHeader::kDwgControlType, vpEntHeaderCtrl);
      const bool claimed = parsed && claimControlHandles(vpEntHeaderCtrl);
      if (!claimed) {
        DRW_DBG("\nWARNING: vpEntHeader control parse failed\n");
        ret = false;
      } else if (!addRawControl(oc,
                                DRW_ViewportEntityHeader::kDwgControlType)) {
        DRW_DBG("\nWARNING: vpEntHeader control raw preservation failed\n");
        ret = false;
      }
      // The control is needed here to discover the type-71 records,
      // but it is not itself an OBJECTS record. Remove it after the
      // control list has been consumed so the ordinary object pass does
      // not misclassify the fixed control type as an unsupported object.
      eraseObject(ObjectMap.find(oc.handle));
    }
  }

  if (!ret)
    return abortTablePhase();

  // EED in table records is parsed before APPID is available to every
  // caller. Resolve its deferred APPID/layer handles once all tables have
  // been collected, matching the entity path above.
  for (auto &item : ltypemap)
    parseAttribs(item.second);
  for (auto &item : layermap)
    parseAttribs(item.second);
  for (auto &item : stylemap)
    parseAttribs(item.second);
  for (auto &item : dimstylemap)
    parseAttribs(item.second);
  for (auto &item : vportmap)
    parseAttribs(item.second);
  for (auto &item : blockRecordmap)
    parseAttribs(item.second);
  for (auto &item : appIdmap)
    parseAttribs(item.second);
  for (auto &item : viewmap)
    parseAttribs(item.second);
  for (auto &item : ucsmap)
    parseAttribs(item.second);

  if (ret) {
    m_deferredRawObjects.clear();
    for (auto &raw : stagedRawControls)
      m_deferredRawObjects.push_back(std::move(raw));
    std::sort(stagedTableFramePublications.begin(),
              stagedTableFramePublications.end(),
              [](const DRW_DwgFramePublication &lhs,
                 const DRW_DwgFramePublication &rhs) {
                return lhs.m_sourceMapOrdinal < rhs.m_sourceMapOrdinal;
              });
    m_deferredTableFramePublications = std::move(stagedTableFramePublications);
  } else {
    clearTableState();
    ret = restoreErasedObjects() && ret;
  }
  return ret;
}

bool dwgReader::publishDeferredTableFramePublications(DRW_Interface &intfa) {
  for (const DRW_DwgFramePublication &publication :
       m_deferredTableFramePublications) {
    if (!publishDwgFramePublication(intfa, publication))
      return false;
  }
  m_deferredTableFramePublications.clear();
  return true;
}

bool dwgReader::readDwgBlocks(DRW_Interface &intfa, dwgBuffer *dbuf,
                              DwgIntegrityAddressSpace offsetSpace) {
  bool ret = true;
  if (dbuf == nullptr)
    return false;
  DRW_DBG("\nobject map total size= ");
  DRW_DBG(ObjectMap.size());
  m_consumedCompoundChildHandles.clear();

  const auto parseBlock = [this](DRW_Block &block, dwgBuffer &buffer,
                                 std::uint32_t bodyBitSize) {
    try {
      return block.parseDwg(version, &buffer, bodyBitSize) && buffer.isGood();
    } catch (...) {
      buffer.invalidate();
      return false;
    }
  };

  auto quarantineOwnedEntities = [&](const DRW_Block_Record &record) {
    const auto quarantine = [&](std::uint32_t handle) {
      if (handle == DRW::NoHandle)
        return;
      const auto objectIt = ObjectMap.find(handle);
      const auto deferredIt = objObjectMap.find(handle);
      if (objectIt != ObjectMap.end() && deferredIt != objObjectMap.end()) {
        (void)reportDwgFrameTransitionFailure(sourceFrameId(objectIt->second),
                                              objectIt->second.loc, true);
        ret = false;
        return;
      }
      if (objectIt != ObjectMap.end()) {
        (void)discardDwgSourceFrame(ObjectMap, objectIt);
        return;
      }
      if (deferredIt != objObjectMap.end())
        (void)discardDwgSourceFrame(objObjectMap, deferredIt);
    };
    quarantine(record.block);
    quarantine(record.endBlock);
    for (const std::uint32_t handle : record.entMap) {
      quarantine(handle);
    }
  };

  const auto recordBlockFailure =
      [this](const DRW_Block_Record &record, std::uint32_t handle,
             std::int16_t type, DwgEntityFailurePhase phase) {
        objHandle object;
        object.handle = handle;
        recordEntityFailure(object, type, phase, record.handle);
      };

  // BLOCK_RECORD ownership is a global graph, not a per-record property.
  // Preflight all modern claims before publishing any block scope so a
  // duplicate handle cannot emit the first block and fail only at the
  // second one.  Legacy files have no entMap, but their BLOCK/ENDBLK pair
  // still benefits from the same duplicate/missing-handle quarantine.
  std::unordered_map<std::uint32_t, const DRW_Block_Record *> claimedHandles;
  std::unordered_set<const DRW_Block_Record *> invalidOwnershipRecords;
  const auto claimHandle = [&](const DRW_Block_Record *record,
                               std::uint32_t handle) {
    if (handle == DRW::NoHandle) {
      recordBlockFailure(*record, handle, -1,
                         DwgEntityFailurePhase::BlockFinalize);
      invalidOwnershipRecords.insert(record);
      return;
    }
    const auto objectIt = ObjectMap.find(handle);
    const auto deferredIt = objObjectMap.find(handle);
    if (objectIt != ObjectMap.end() && deferredIt != objObjectMap.end()) {
      (void)reportDwgFrameTransitionFailure(sourceFrameId(objectIt->second),
                                            objectIt->second.loc, true);
      recordBlockFailure(*record, handle, -1,
                         DwgEntityFailurePhase::BlockFinalize);
      invalidOwnershipRecords.insert(record);
      return;
    }
    if (objectIt == ObjectMap.end()) {
      recordBlockFailure(*record, handle, -1,
                         DwgEntityFailurePhase::BlockFinalize);
      invalidOwnershipRecords.insert(record);
      return;
    }
    const auto [it, inserted] = claimedHandles.emplace(handle, record);
    if (!inserted) {
      recordBlockFailure(*record, handle, -1,
                         DwgEntityFailurePhase::BlockFinalize);
      invalidOwnershipRecords.insert(record);
      invalidOwnershipRecords.insert(it->second);
    }
  };
  for (const auto &item : blockRecordmap) {
    const DRW_Block_Record *record = item.second;
    claimHandle(record, record->block);
    claimHandle(record, record->endBlock);
    if (version > DRW::AC1015) {
      for (const std::uint32_t handle : record->entMap)
        claimHandle(record, handle);
    }
  }
  if (!invalidOwnershipRecords.empty()) {
    for (const DRW_Block_Record *record : invalidOwnershipRecords) {
      quarantineOwnedEntities(*record);
    }
    ret = false;
  }

  if (version >= DRW::AC1018) {
    try {
      std::vector<const DRW_Block_Record *> records;
      records.reserve(blockRecordmap.size());
      for (const auto &item : blockRecordmap)
        records.push_back(item.second);
      if (!preflightMappedPolylineOwnership(records, dbuf))
        return false;
    } catch (...) {
      return false;
    }
  }

  for (auto it = blockRecordmap.begin(); it != blockRecordmap.end(); ++it) {
    DRW_Block_Record *bkr = it->second;
    if (invalidOwnershipRecords.find(bkr) != invalidOwnershipRecords.end())
      continue;
    DRW_DBG("\nParsing Block, record handle= ");
    DRW_DBGH(it->first);
    DRW_DBG(" Name= ");
    DRW_DBG(bkr->name);
    DRW_DBG("\n");
    DRW_DBG("\nFinding Block, handle= ");
    DRW_DBGH(bkr->block);
    DRW_DBG("\n");
    auto mit = ObjectMap.find(bkr->block);
    if (mit == ObjectMap.end()) {
      DRW_DBG("\nWARNING: block entity not found\n");
      recordBlockFailure(*bkr, bkr->block, dwgType::BLOCK,
                         DwgEntityFailurePhase::Frame);
      quarantineOwnedEntities(*bkr);
      ret = false;
      continue;
    }
    objHandle oc = mit->second;
    DRW_DBG("Block Handle= ");
    DRW_DBGH(oc.handle);
    DRW_DBG(" Location: ");
    DRW_DBG(oc.loc);
    DRW_DBG("\n");
    DwgObjectFrame frame;
    if (!frame.readAt(*dbuf, version, oc.loc)) {
      recordObjectFrameFailure(oc, offsetSpace);
      DRW_DBG("Invalid block entity frame\n");
      recordBlockFailure(*bkr, oc.handle, dwgType::BLOCK,
                         DwgEntityFailurePhase::Frame);
      quarantineOwnedEntities(*bkr);
      ret = false;
      continue;
    }
    auto &body = frame.body();
    dwgBuffer buff(body.data(), body.size(), &decoder);
    DRW_Block bk;
    dwgBuffer typeBuffer = buff.forkIndependent();
    if (typeBuffer.getObjType(version) != dwgType::BLOCK ||
        !typeBuffer.isGood()) {
      DRW_DBG("Invalid block entity type\n");
      recordBlockFailure(*bkr, oc.handle, dwgType::BLOCK,
                         DwgEntityFailurePhase::TypedBody);
      quarantineOwnedEntities(*bkr);
      ret = false;
      continue;
    }
    if (!parseBlock(bk, buff, frame.bodyBitSize())) {
      DRW_DBG("Invalid block entity body\n");
      recordBlockFailure(*bkr, oc.handle, dwgType::BLOCK,
                         DwgEntityFailurePhase::TypedBody);
      quarantineOwnedEntities(*bkr);
      ret = false;
      continue;
    }
    if (bk.handle != oc.handle || bk.handle != bkr->block) {
      DRW_DBG("BLOCK handle does not match its BLOCK_RECORD\n");
      recordBlockFailure(*bkr, oc.handle, dwgType::BLOCK,
                         DwgEntityFailurePhase::Identity);
      quarantineOwnedEntities(*bkr);
      ret = false;
      continue;
    }
    const objHandle blockObject = oc;
    parseAttribs(&bk);
    // complete block entity with block record data
    bk.basePoint = bkr->basePoint;
    bk.flags = bkr->flags;
    bk.insUnits = bkr->insUnits;
    bk.xrefPath = bkr->xrefPath;

    // Validate the ownership graph before publishing any callbacks. The
    // R2004+ BLOCK_HEADER vector is authoritative; a missing, duplicate,
    // or special block/ENDBLK handle would otherwise leave a partial block
    // scope and force the later ObjectMap sweep to guess ownership.
    if (version > DRW::AC1015) {
      std::unordered_set<std::uint32_t> ownedHandles;
      if (bkr->entMap.size() >
              static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
          !DRW::reserve(ownedHandles, static_cast<int>(bkr->entMap.size()))) {
        quarantineOwnedEntities(*bkr);
        ret = false;
        continue;
      }
      bool validOwnership = true;
      std::uint32_t invalidOwnershipHandle = DRW::NoHandle;
      for (const std::uint32_t entityHandle : bkr->entMap) {
        if (entityHandle == DRW::NoHandle || entityHandle == bkr->block ||
            entityHandle == bkr->endBlock ||
            ObjectMap.find(entityHandle) == ObjectMap.end() ||
            !ownedHandles.insert(entityHandle).second) {
          validOwnership = false;
          invalidOwnershipHandle = entityHandle;
          break;
        }
      }
      if (validOwnership) {
        // BLOCK_RECORD.entMap contains entity handles only. Fixed
        // OBJECTS types and custom classes with entityFlag == 0 must
        // not be allowed into the entity walk, where they would be
        // deferred and later decoded under the wrong ownership.
        for (const std::uint32_t entityHandle : bkr->entMap) {
          const auto entityIt = ObjectMap.find(entityHandle);
          if (entityIt == ObjectMap.end()) {
            validOwnership = false;
            break;
          }
          DwgFrameClassification classification;
          if (!classifyDwgSourceFrame(dbuf, entityIt->second, classification)) {
            recordObjectFrameFailure(entityIt->second, offsetSpace);
            validOwnership = false;
            break;
          }
          if (classification.route != DwgFrameClassification::Route::Entity) {
            validOwnership = false;
            break;
          }
        }
      }
      if (!validOwnership) {
        DRW_DBG("Invalid BLOCK_RECORD owned-entity handle list\n");
        recordBlockFailure(*bkr, invalidOwnershipHandle, -1,
                           DwgEntityFailurePhase::BlockFinalize);
        quarantineOwnedEntities(*bkr);
        ret = false;
        continue;
      }
    }

    auto endIt = ObjectMap.find(bkr->endBlock);
    if (endIt == ObjectMap.end()) {
      DRW_DBG("\nWARNING: end block entity not found\n");
      recordBlockFailure(*bkr, bkr->endBlock, dwgType::ENDBLK,
                         DwgEntityFailurePhase::Frame);
      quarantineOwnedEntities(*bkr);
      ret = false;
      continue;
    }
    oc = endIt->second;
    DRW_DBG("End block Handle= ");
    DRW_DBGH(oc.handle);
    DRW_DBG(" Location: ");
    DRW_DBG(oc.loc);
    DRW_DBG("\n");
    DwgObjectFrame endFrame;
    if (!endFrame.readAt(*dbuf, version, oc.loc)) {
      recordObjectFrameFailure(oc, offsetSpace);
      DRW_DBG("Invalid end block entity frame\n");
      recordBlockFailure(*bkr, oc.handle, dwgType::ENDBLK,
                         DwgEntityFailurePhase::Frame);
      quarantineOwnedEntities(*bkr);
      ret = false;
      continue;
    }
    auto &endBody = endFrame.body();
    dwgBuffer buff1(endBody.data(), endBody.size(), &decoder);
    DRW_Block end;
    end.isEnd = true;
    dwgBuffer endTypeBuffer = buff1.forkIndependent();
    if (endTypeBuffer.getObjType(version) != dwgType::ENDBLK ||
        !endTypeBuffer.isGood()) {
      DRW_DBG("Invalid end block entity type\n");
      recordBlockFailure(*bkr, oc.handle, dwgType::ENDBLK,
                         DwgEntityFailurePhase::TypedBody);
      quarantineOwnedEntities(*bkr);
      ret = false;
      continue;
    }
    if (!parseBlock(end, buff1, endFrame.bodyBitSize())) {
      DRW_DBG("Invalid end block entity body\n");
      recordBlockFailure(*bkr, oc.handle, dwgType::ENDBLK,
                         DwgEntityFailurePhase::TypedBody);
      quarantineOwnedEntities(*bkr);
      ret = false;
      continue;
    }
    if (end.handle != oc.handle || end.handle != bkr->endBlock) {
      DRW_DBG("ENDBLK handle does not match its BLOCK_RECORD\n");
      recordBlockFailure(*bkr, oc.handle, dwgType::ENDBLK,
                         DwgEntityFailurePhase::Identity);
      quarantineOwnedEntities(*bkr);
      ret = false;
      continue;
    }
    const objHandle endBlockObject = oc;
    parseAttribs(&end);
    // BLOCK and ENDBLK delimit the same BLOCK_RECORD scope.  Named blocks
    // carry that owner in both records; modelspace/paperspace use the
    // ownerless form in files handled here.  Reject a mismatched pair
    // before publishing either scope boundary.
    if (bk.parentHandle != end.parentHandle) {
      DRW_DBG("Mismatched BLOCK/ENDBLK owner handles\n");
      recordBlockFailure(*bkr, bkr->handle, -1,
                         DwgEntityFailurePhase::Identity);
      quarantineOwnedEntities(*bkr);
      ret = false;
      continue;
    }
    const bool isSpaceBlockRecord = isSpaceBlockRecordName(bk.name);
    const bool validDelimiterOwner = isSpaceBlockRecord
                                         ? bk.parentHandle == DRW::NoHandle
                                         : bk.parentHandle == bkr->handle;
    if (!validDelimiterOwner) {
      DRW_DBG("BLOCK/ENDBLK owner does not match BLOCK_RECORD\n");
      recordBlockFailure(*bkr, bkr->handle, -1,
                         DwgEntityFailurePhase::Identity);
      quarantineOwnedEntities(*bkr);
      ret = false;
      continue;
    }
    // Keep delimiter receipts private until the complete scope has been
    // accepted. addBlock() necessarily precedes the owned-entity walk,
    // so a later child failure cannot establish a complete BLOCK graph
    // record. Preserve the source parent before the model/paper-space
    // routing below normalizes bk.parentHandle for interface delivery.
    const DRW_DwgFramePublication blockPublication =
        makeTypedEntityFramePublication(version, blockObject, dwgType::BLOCK,
                                        bk);
    const DRW_DwgFramePublication endBlockPublication =
        makeTypedEntityFramePublication(version, endBlockObject,
                                        dwgType::ENDBLK, end);
    const std::size_t entityFailuresBefore = m_entityParseFailures;
    const std::size_t entityDiagnosticsBefore =
        m_entityFailureDiagnostics.size();

    // A modern block made solely of independently deliverable entities
    // can be admitted as one callback transaction. Compound and custom
    // entities keep their established staged delivery path below.
    bool journalEligible = version >= DRW::AC1018;
    if (journalEligible) {
      for (const std::uint32_t entityHandle : bkr->entMap) {
        const auto entityIt = ObjectMap.find(entityHandle);
        DwgFrameClassification classification;
        if (entityIt == ObjectMap.end() ||
            !classifyDwgSourceFrame(dbuf, entityIt->second, classification) ||
            classification.route != DwgFrameClassification::Route::Entity) {
          journalEligible = false;
          break;
        }
      }
    }
    if (journalEligible) {
      DwgBlockScopeTransaction transaction(*this);
      const auto discardUnadopted = [this](DwgFrameMapLease &lease) {
        if (!lease.isDetached())
          return;
        if (lease.hasCoverage)
          (void)quarantineDwgFrame(lease.source);
        else
          (void)suppressDwgFrame(lease.source, false);
        (void)discardDetachedDwgSourceFrame(lease);
      };
      const auto stageDelimiter = [this, &transaction, &discardUnadopted](
                                      std::uint32_t handle,
                                      std::uint32_t bodyByteSize) {
        const auto delimiterIt = ObjectMap.find(handle);
        if (delimiterIt == ObjectMap.end())
          return false;
        if (!transaction.reserveAdmission(1u, 2u, bodyByteSize))
          return false;
        DwgFrameMapLease lease;
        if (!detachDwgSourceFrame(ObjectMap, delimiterIt, lease))
          return false;
        lease.bodyByteSize = bodyByteSize;
        if (!stageDetachedDwgSourceFrame(lease) || !transaction.adopt(lease)) {
          discardUnadopted(lease);
          return false;
        }
        return true;
      };

      bool journalled = false;
      try {
        journalled = stageDelimiter(blockObject.handle,
                                    static_cast<std::uint32_t>(body.size())) &&
                     stageDelimiter(endBlockObject.handle,
                                    static_cast<std::uint32_t>(endBody.size()));
        const DwgSourceFrameId blockSource = sourceFrameId(blockObject);
        const DwgSourceFrameId endBlockSource = sourceFrameId(endBlockObject);
        DRW_Block deliveryBlock = bk;
        const bool deferredEntityWalk =
            deliveryBlock.parentHandle == DRW::NoHandle;
        if (deferredEntityWalk)
          deliveryBlock.parentHandle = bkr->handle;

        if (journalled) {
          auto blockScope = transaction.output().bindSource(blockSource);
          transaction.output().appendValue(deliveryBlock,
                                           &DRW_Interface::addBlock);
        }
        if (journalled && deferredEntityWalk) {
          auto endBlockScope = transaction.output().bindSource(endBlockSource);
          journalled = transaction.output().appendEndBlock();
        }
        if (journalled) {
          journalled = walkJournalledBlockRecordEntities(
              bkr, dbuf, intfa, transaction,
              deferredEntityWalk ? DRW::NoHandle : bk.parentHandle, bkr->handle,
              offsetSpace);
        }
        if (journalled && !deferredEntityWalk) {
          auto endBlockScope = transaction.output().bindSource(endBlockSource);
          journalled = transaction.output().appendEndBlock();
        }
        std::optional<DRW_DwgBlockReachability> reachability;
        if (journalled) {
          const auto receiptSource = [this](const DwgSourceFrameId &source)
              -> std::optional<DRW_DwgSourceFrame> {
            const auto sourceIt = m_dwgSourceFrameIndexes.find(source.handle);
            if (source.handle == DRW::NoHandle ||
                sourceIt == m_dwgSourceFrameIndexes.cend() ||
                sourceIt->second >= m_dwgSourceFrameLedger.size()) {
              return std::nullopt;
            }
            const DRW_DwgFrameCoverageEntry &entry =
                m_dwgSourceFrameLedger[sourceIt->second];
            if (entry.m_handle != source.handle ||
                entry.m_sourceOffset != source.offset ||
                entry.m_sourceMapOrdinal != source.ordinal ||
                entry.m_sourceOffsetSpace != source.offsetSpace) {
              return std::nullopt;
            }
            return DRW_DwgSourceFrame{entry.m_handle, entry.m_sourceOffset,
                                      entry.m_sourceMapOrdinal,
                                      entry.m_sourceOffsetSpace, true};
          };
          const auto recordSource = sourceFrameIdForHandle(bkr->handle);
          const auto recordReceipt = receiptSource(recordSource);
          // Direct reader probes can exercise a block scope without
          // first parsing its BLOCK_RECORD table frame.  Such a
          // scope has no public record source to certify; genuine
          // DWG reads always retain that source in the ledger.
          if (recordReceipt.has_value()) {
            const auto recordIndex = m_dwgSourceFrameIndexes.find(bkr->handle);
            if (recordIndex == m_dwgSourceFrameIndexes.cend() ||
                recordIndex->second >= m_dwgSourceFrameLedger.size() ||
                m_dwgSourceFrameLedger[recordIndex->second].m_disposition !=
                    DRW_DwgFrameDisposition::Published ||
                m_dwgSourceFrameLedger[recordIndex->second]
                        .m_publicationCount != 1u) {
              journalled = false;
            } else {
              const auto blockReceipt = receiptSource(blockSource);
              const auto endBlockReceipt = receiptSource(endBlockSource);
              if (!blockReceipt.has_value() || !endBlockReceipt.has_value()) {
                journalled = false;
              } else {
                DRW_DwgBlockReachability receipt;
                receipt.m_version = version;
                receipt.m_complete = true;
                receipt.m_blockRecord = *recordReceipt;
                receipt.m_block = *blockReceipt;
                receipt.m_endBlock = *endBlockReceipt;
                try {
                  receipt.m_entities.reserve(bkr->entMap.size());
                  for (const std::uint32_t entityHandle : bkr->entMap) {
                    const auto entityReceipt =
                        receiptSource(sourceFrameIdForHandle(entityHandle));
                    if (!entityReceipt.has_value()) {
                      journalled = false;
                      break;
                    }
                    receipt.m_entities.push_back(*entityReceipt);
                  }
                } catch (...) {
                  journalled = false;
                }
                if (journalled)
                  reachability.emplace(std::move(receipt));
              }
            }
          }
        }
        if (journalled) {
          journalled =
              transaction.output().appendFramePublication(
                  *this, blockPublication,
                  {nullptr, nullptr,
                   reachability.has_value() ? &*reachability : nullptr}) &&
              transaction.output().appendFramePublication(*this,
                                                          endBlockPublication);
        }
        if (journalled) {
          journalled = transaction.replay(intfa);
          if (journalled)
            bkr->name = bk.name;
        }
      } catch (...) {
        journalled = false;
      }

      if (!journalled) {
        (void)transaction.abort();
        if (hasPendingCompoundStateForBlock(*bkr))
          (void)abandonStagedCompoundState();
        if (m_entityFailureDiagnostics.size() == entityDiagnosticsBefore) {
          recordBlockFailure(*bkr, bkr->handle, -1,
                             DwgEntityFailurePhase::BlockFinalize);
        }
        quarantineOwnedEntities(*bkr);
        ret = false;
      }
      continue;
    }

    /**read & send block entities**/
    // Modelspace / paperspace block_records have no DWG-side parent
    // handle (the legacy "330 not set like dxf in ModelSpace & PaperSpace"
    // case).  Their entities are still walked here, but post-endBlock so
    // they land in the interface's modelspace container rather than in
    // the just-opened addBlock scope.  Walking in entMap / firstEH..lastEH
    // order also guarantees POLYLINE parents precede their VERTEX
    // children, which the staged POLYLINE chain's bounded source lookup
    // requires.
    const bool deferredEntityWalk = (bk.parentHandle == DRW::NoHandle);
    if (deferredEntityWalk) {
      bk.parentHandle = bkr->handle;
    }
    bool blockScopeOpened = false;
    bool blockScopeFailure = false;
    bool blockEntityWalkSucceeded = true;
    try {
      intfa.addBlock(bk);
      blockScopeOpened = true;
      // and update block record name
      bkr->name = bk.name;

      if (!deferredEntityWalk) {
        const bool walked = walkBlockRecordEntities(
            bkr, dbuf, intfa, bk.parentHandle, bkr->handle, offsetSpace);
        blockEntityWalkSucceeded = walked;
        ret = walked && ret;
        blockScopeFailure = blockScopeFailure || !walked;
      }
    } catch (...) {
      // Interface callbacks are outside the byte parser's control. Do
      // not let one callback exception escape with a half-admitted
      // block; the scope is closed below exactly once when opened.
      ret = false;
      blockScopeFailure = true;
    }

    if (blockScopeOpened) {
      try {
        intfa.endBlock();
      } catch (...) {
        ret = false;
        blockScopeFailure = true;
      }
    }

    if (deferredEntityWalk && !blockScopeFailure) {
      // currentBlock has just been reset to the interface's modelspace
      // container; dispatched entities flow there.
      try {
        const bool walked = walkBlockRecordEntities(
            bkr, dbuf, intfa, DRW::NoHandle, bkr->handle, offsetSpace);
        blockEntityWalkSucceeded = walked;
        ret = walked && ret;
        blockScopeFailure = blockScopeFailure || !walked;
      } catch (...) {
        ret = false;
        blockScopeFailure = true;
      }
    }

    blockScopeFailure = blockScopeFailure || !blockEntityWalkSucceeded ||
                        m_entityParseFailures != entityFailuresBefore;

    if (blockScopeOpened && !blockScopeFailure &&
        blockEntityWalkSucceeded
        // An unresolved INSERT delays only the delimiters of the block
        // that owns it; independent scopes can complete normally.
        && !hasPendingCompoundStateForBlock(*bkr)) {
      const auto commitDelimiter = [&](std::uint32_t handle,
                                       DwgSourceFrameLease &lease) {
        const auto delimiterIt = ObjectMap.find(handle);
        return delimiterIt != ObjectMap.end() &&
               borrowDwgSourceFrame(ObjectMap, delimiterIt, lease);
      };
      DwgSourceFrameLease blockLease;
      DwgSourceFrameLease endBlockLease;
      if (!commitDelimiter(blockObject.handle, blockLease) ||
          !commitDelimiter(endBlockObject.handle, endBlockLease)) {
        ret = false;
        blockScopeFailure = true;
      } else {
        const auto blockIt = ObjectMap.find(blockObject.handle);
        const auto endIt = ObjectMap.find(endBlockObject.handle);
        if (blockIt == ObjectMap.end() || endIt == ObjectMap.end()) {
          ret = false;
          blockScopeFailure = true;
        } else {
          // Both entries were borrowed above. Node extraction is
          // allocation-free, so commit the delimiter pair without
          // a state where one delimiter has been removed and the
          // other remains in the source map.
          DwgObjectMap::node_type blockNode = ObjectMap.extract(blockIt);
          DwgObjectMap::node_type endBlockNode = ObjectMap.extract(endIt);
          if (blockNode.empty() || endBlockNode.empty()) {
            if (!blockNode.empty())
              ObjectMap.insert(std::move(blockNode));
            if (!endBlockNode.empty())
              ObjectMap.insert(std::move(endBlockNode));
            ret = false;
            blockScopeFailure = true;
          }
        }
      }
    }

    if (blockScopeOpened && !blockScopeFailure && blockEntityWalkSucceeded &&
        !hasPendingCompoundStateForBlock(*bkr) &&
        m_dwgFrameCoverageStatus != DRW_DwgFrameCoverageStatus::NotAvailable) {
      try {
        if (!publishDwgFramePublication(intfa, blockPublication) ||
            !publishDwgFramePublication(intfa, endBlockPublication)) {
          ret = false;
          blockScopeFailure = true;
        }
      } catch (...) {
        ret = false;
        blockScopeFailure = true;
      }
    }

    if (blockScopeFailure) {
      if (m_entityFailureDiagnostics.size() == entityDiagnosticsBefore) {
        recordBlockFailure(*bkr, bkr->handle, -1,
                           DwgEntityFailurePhase::BlockFinalize);
      }
      quarantineOwnedEntities(*bkr);
    }
  }

  return ret;
}

bool dwgReader::walkBlockRecordEntities(DRW_Block_Record *bkr, dwgBuffer *dbuf,
                                        DRW_Interface &intfa,
                                        std::uint32_t expectedOwner,
                                        std::uint32_t rawBlockOwner,
                                        DwgIntegrityAddressSpace offsetSpace) {
  // Per-entity parseDwg failures are warnings, not section failures —
  // we keep walking so a single bad entity doesn't drop the rest of
  // the block. Structural failures (entity-not-found in ObjectMap)
  // remain in `ret` so the caller knows the block walk was incomplete.
  bool ret = true;
  objHandle oc;
  const std::uint32_t previousOwner = expectedBlockEntityOwner;
  const std::uint32_t previousRawOwner = rawBlockEntityOwner;
  const bool previousOwnerlessSpaceWalk = ownerlessSpaceWalk;
  expectedBlockEntityOwner = expectedOwner;
  rawBlockEntityOwner =
      rawBlockOwner != DRW::NoHandle ? rawBlockOwner : expectedOwner;
  ownerlessSpaceWalk =
      expectedOwner == DRW::NoHandle && isSpaceBlockRecordName(bkr->name);
  const auto recordWalkFailure = [this, bkr](const objHandle &object,
                                             std::int16_t type,
                                             DwgEntityFailurePhase phase) {
    recordEntityFailure(object, type, phase, bkr->handle);
  };
  const auto unresolvedCompoundHandle = [this, bkr]() -> std::uint32_t {
    const auto containsEntity = [bkr](std::uint32_t handle) {
      return std::find(bkr->entMap.cbegin(), bkr->entMap.cend(), handle) !=
             bkr->entMap.cend();
    };
    const auto belongsToBlock = [bkr,
                                 &containsEntity](const DRW_Entity &entity) {
      return containsEntity(entity.handle) ||
             entity.parentHandle == bkr->handle ||
             (entity.parentHandle == DRW::NoHandle &&
              isSpaceBlockRecordName(bkr->name));
    };
    for (const auto &item : m_pendingInsertStates) {
      if (belongsToBlock(item.second.entity))
        return item.first;
    }
    for (const auto &item : m_pendingPolylineStates) {
      if (belongsToBlock(item.second.entity))
        return item.first;
    }
    for (const auto &item : m_orphanAttribStates) {
      for (const StagedAttribState &attribute : item.second.attributes) {
        if (attribute.entity != nullptr &&
            containsEntity(attribute.entity->handle)) {
          return attribute.entity->handle;
        }
      }
    }
    for (const auto &item : m_orphanPolylineVertexStates) {
      for (const StagedVertexState &vertex : item.second.vertices) {
        if (containsEntity(vertex.entity.handle))
          return vertex.entity.handle;
      }
    }
    for (const auto &item : m_stagedSeqEnds) {
      if (containsEntity(item.first))
        return item.first;
    }
    return static_cast<std::uint32_t>(DRW::NoHandle);
  };

  const auto restoreState = [this, previousOwner, previousRawOwner,
                             previousOwnerlessSpaceWalk]() noexcept {
    expectedBlockEntityOwner = previousOwner;
    rawBlockEntityOwner = previousRawOwner;
    ownerlessSpaceWalk = previousOwnerlessSpaceWalk;
  };

  if (version >= DRW::AC1018) {
    bool hasDuplicateSource = false;
    std::uint32_t duplicateSourceHandle = DRW::NoHandle;
    std::unordered_set<std::uint32_t> sourceHandles;
    try {
      sourceHandles.reserve(bkr->entMap.size());
      for (const std::uint32_t handle : bkr->entMap) {
        if (!sourceHandles.insert(handle).second) {
          hasDuplicateSource = true;
          duplicateSourceHandle = handle;
        }
      }
    } catch (...) {
      restoreState();
      return false;
    }

    if (hasDuplicateSource) {
      // A BLOCK_RECORD names each physical source frame once. Consumed
      // markers suppress later walks; they never legitimize a duplicate
      // entry in this record after another entity has been published.
      for (const std::uint32_t handle : sourceHandles) {
        auto objectIt = ObjectMap.find(handle);
        if (objectIt != ObjectMap.end()) {
          (void)discardDwgSourceFrame(ObjectMap, objectIt);
          continue;
        }
        auto deferredIt = objObjectMap.find(handle);
        if (deferredIt != objObjectMap.end())
          (void)discardDwgSourceFrame(objObjectMap, deferredIt);
      }
      objHandle duplicate;
      duplicate.handle = duplicateSourceHandle;
      recordWalkFailure(duplicate, -1, DwgEntityFailurePhase::BlockFinalize);
      ++m_entityParseFailures;
      restoreState();
      return false;
    }

    bool ownershipPreflight = false;
    try {
      const std::vector<const DRW_Block_Record *> records = {bkr};
      ownershipPreflight = preflightMappedPolylineOwnership(records, dbuf);
    } catch (...) {
      ownershipPreflight = false;
    }
    if (!ownershipPreflight) {
      restoreState();
      return false;
    }
  }

  try {
    if (version < DRW::AC1018) { // pre 2004
      std::uint32_t nextH = bkr->firstEH;
      std::unordered_set<std::uint32_t> visitedHandles;
      while (nextH != 0) {
        if (!visitedHandles.insert(nextH).second) {
          // R13-R2000 stores ownership as a next_entity chain.  A
          // corrupt cycle must not hang the reader indefinitely.
          DRW_DBG("\nWARNING: Cyclic entity chain in block\n");
          ret = false;
          ++m_entityParseFailures;
          break;
        }
        auto mit = ObjectMap.find(nextH);
        if (mit == ObjectMap.end()) {
          // A broken/garbage nextEntLink at the chain end (common in real
          // R13–R2000 files) must NOT fail the BLOCKS section: the
          // remaining entities still sit in ObjectMap and are recovered by
          // the subsequent readDwgEntities sweep. Treat as a soft warning
          // (libreDWG parity) — stop chasing this chain but keep ret true.
          DRW_DBG("\nWARNING: Entity of block not found\n");
          ++m_entityParseFailures;
          break;
        }
        bool frameFailure = false;
        bool read = false;
        DwgFrameClassification classification;
        if (requiresLegacyCompoundHandling(dbuf, mit->second,
                                           &classification)) {
          DwgSourceFrameLease lease;
          if (!takeDwgSourceFrame(ObjectMap, mit, lease)) {
            ret = false;
            ++m_entityParseFailures;
            break;
          }
          oc = lease.object;
          read = readDwgEntity(dbuf, oc, intfa, &frameFailure, offsetSpace);
          if (!read) {
            (void)markDwgFrameOutcome(lease.source,
                                      DRW_DwgFrameDisposition::Failed);
          }
        } else {
          DwgFrameMapLease lease;
          if (!detachDwgSourceFrame(ObjectMap, mit, lease)) {
            ret = false;
            ++m_entityParseFailures;
            break;
          }
          lease.classification.emplace(std::move(classification));
          oc = lease.object;
          read = readMappedDwgEntity(dbuf, lease, intfa, &frameFailure,
                                     offsetSpace);
        }
        if (!read) {
          ++m_entityParseFailures;
        }
        const bool identityFailure =
            parsedEntityHandleMismatch || parsedEntityOwnerMismatch;
        ret = !frameFailure && !identityFailure && ret;
        if (nextH == bkr->lastEH)
          nextH = 0; // redundant, but prevent read errors
        else if (nextEntLinkImplicit && nextEntLink > bkr->lastEH)
          // An inferred chain may advance beyond the declared tail
          // when a compound entity consumed its SEQEND.  Equality is
          // still a valid next entity and must be visited.
          nextH = 0;
        else if (nextEntLinkImplicit && nextEntLink == bkr->lastEH &&
                 ObjectMap.find(nextEntLink) == ObjectMap.end())
          // A compound entity may have consumed the declared tail
          // (normally its SEQEND) while advancing the inferred chain.
          nextH = 0;
        else
          nextH = nextEntLink;
      }
    } else { // 2004+
      for (auto it = bkr->entMap.begin(); it != bkr->entMap.end(); ++it) {
        std::uint32_t nextH = *it;
        if (m_quarantinedEntityHandles.find(nextH) !=
            m_quarantinedEntityHandles.end())
          continue;
        auto mit = ObjectMap.find(nextH);
        if (mit == ObjectMap.end()) {
          if (m_consumedCompoundChildHandles.find(nextH) !=
                  m_consumedCompoundChildHandles.end() ||
              m_consumedSeqEndHandles.find(nextH) !=
                  m_consumedSeqEndHandles.end())
            continue;
          // Soft warning, not a section failure (libreDWG parity): a
          // missing entMap handle is recovered by the readDwgEntities
          // sweep. See the pre-2004 branch above for the rationale.
          DRW_DBG("\nWARNING: Entity of block not found\n");
          objHandle missing;
          missing.handle = nextH;
          recordWalkFailure(missing, -1, DwgEntityFailurePhase::BlockFinalize);
          ++m_entityParseFailures;
          continue;
        }
        bool frameFailure = false;
        bool read = false;
        DwgFrameClassification classification;
        if (requiresLegacyCompoundHandling(dbuf, mit->second,
                                           &classification)) {
          DwgSourceFrameLease lease;
          if (!takeDwgSourceFrame(ObjectMap, mit, lease)) {
            ret = false;
            recordWalkFailure(mit->second, classification.resolvedType,
                              DwgEntityFailurePhase::Frame);
            ++m_entityParseFailures;
            continue;
          }
          oc = lease.object;
          read = readDwgEntity(dbuf, oc, intfa, &frameFailure, offsetSpace);
          if (!read) {
            (void)markDwgFrameOutcome(lease.source,
                                      DRW_DwgFrameDisposition::Failed);
          }
        } else {
          DwgFrameMapLease lease;
          if (!detachDwgSourceFrame(ObjectMap, mit, lease)) {
            ret = false;
            recordWalkFailure(mit->second, classification.resolvedType,
                              DwgEntityFailurePhase::Frame);
            ++m_entityParseFailures;
            continue;
          }
          lease.classification.emplace(std::move(classification));
          oc = lease.object;
          read = readMappedDwgEntity(dbuf, lease, intfa, &frameFailure,
                                     offsetSpace);
        }
        DRW_DBG("\nBlocks, parsing entity: ");
        DRW_DBGH(oc.handle);
        DRW_DBG(", pos: ");
        DRW_DBG(oc.loc);
        DRW_DBG("\n");
        if (!read) {
          recordWalkFailure(oc, classification.resolvedType,
                            DwgEntityFailurePhase::TypedBody);
          ++m_entityParseFailures;
        }
        const bool identityFailure =
            parsedEntityHandleMismatch || parsedEntityOwnerMismatch;
        ret = !frameFailure && !identityFailure && ret;
      }
    }
    const bool hasUnresolvedCompound = hasPendingCompoundStateForBlock(*bkr);
    if (hasUnresolvedCompound) {
      ret = false;
      objHandle unresolved;
      unresolved.handle = unresolvedCompoundHandle();
      recordWalkFailure(unresolved, -1, DwgEntityFailurePhase::Aggregate);
      if (!abandonStagedCompoundState())
        ret = false;
    }
    restoreState();
    return ret;
  } catch (...) {
    restoreState();
    return false;
  }
}

bool dwgReader::walkJournalledBlockRecordEntities(
    DRW_Block_Record *bkr, dwgBuffer *dbuf, DRW_Interface &intfa,
    DwgBlockScopeTransaction &transaction, std::uint32_t expectedOwner,
    std::uint32_t rawBlockOwner, DwgIntegrityAddressSpace offsetSpace) {
  if (bkr == nullptr || dbuf == nullptr || version < DRW::AC1018)
    return false;

  const std::uint32_t previousOwner = expectedBlockEntityOwner;
  const std::uint32_t previousRawOwner = rawBlockEntityOwner;
  const bool previousOwnerlessSpaceWalk = ownerlessSpaceWalk;
  expectedBlockEntityOwner = expectedOwner;
  rawBlockEntityOwner =
      rawBlockOwner != DRW::NoHandle ? rawBlockOwner : expectedOwner;
  ownerlessSpaceWalk =
      expectedOwner == DRW::NoHandle && isSpaceBlockRecordName(bkr->name);
  const auto restoreState = [this, previousOwner, previousRawOwner,
                             previousOwnerlessSpaceWalk]() noexcept {
    expectedBlockEntityOwner = previousOwner;
    rawBlockEntityOwner = previousRawOwner;
    ownerlessSpaceWalk = previousOwnerlessSpaceWalk;
  };
  const auto discardUnadopted = [this](DwgFrameMapLease &lease) {
    if (!lease.isDetached())
      return;
    if (lease.hasCoverage)
      (void)quarantineDwgFrame(lease.source);
    else
      (void)suppressDwgFrame(lease.source, false);
    (void)discardDetachedDwgSourceFrame(lease);
  };

  try {
    std::unordered_set<std::uint32_t> handles;
    handles.reserve(bkr->entMap.size());
    for (const std::uint32_t handle : bkr->entMap) {
      if (handle == DRW::NoHandle || !handles.insert(handle).second) {
        restoreState();
        return false;
      }
      const auto sourceIt = ObjectMap.find(handle);
      if (sourceIt == ObjectMap.end()) {
        if (m_consumedCompoundChildHandles.find(handle) !=
                m_consumedCompoundChildHandles.end() ||
            m_consumedSeqEndHandles.find(handle) !=
                m_consumedSeqEndHandles.end()) {
          continue;
        }
        restoreState();
        return false;
      }
      DwgFrameClassification classification;
      if (!classifyDwgSourceFrame(dbuf, sourceIt->second, classification) ||
          classification.route != DwgFrameClassification::Route::Entity) {
        restoreState();
        return false;
      }
      if (!requiresAggregateDelivery(classification.resolvedType) &&
          !transaction.reserveAdmission(1u, 3u, classification.bodyByteSize)) {
        restoreState();
        return false;
      }

      DwgFrameMapLease lease;
      if (!detachDwgSourceFrame(ObjectMap, sourceIt, lease)) {
        restoreState();
        return false;
      }
      lease.bodyByteSize = classification.bodyByteSize;
      lease.classification.emplace(std::move(classification));
      bool frameFailure = false;
      const bool parsed = readMappedDwgEntity(
          dbuf, lease, intfa, &frameFailure, offsetSpace, &transaction.output(),
          DwgMappedEntityCompletion::Journal, &transaction);
      if (!parsed || frameFailure) {
        discardUnadopted(lease);
        if (m_entityParseFailures != std::numeric_limits<std::size_t>::max()) {
          ++m_entityParseFailures;
        }
        restoreState();
        return false;
      }
      if (lease.isDetached() && !transaction.adopt(lease)) {
        discardUnadopted(lease);
        restoreState();
        return false;
      }
    }
    if (hasPendingCompoundStateForBlock(*bkr)) {
      restoreState();
      return false;
    }
  } catch (...) {
    restoreState();
    return false;
  }
  restoreState();
  return true;
}

void dwgReader::linkDataStorage(DRW_Entity &entity) {
  if (version <= DRW::AC1024 || entity.hasDsData == 0)
    return;

  // Linking is normally called once during entity traversal.  Keep the
  // operation idempotent for callers that replay the traversal, rather than
  // turning a second visit into a duplicate-record claim.
  if (entity.hasDataStorageRecord)
    return;

  const bool hasExplicitKey = !entity.dataStorageHandleKey.empty();
  const bool hasExplicitHandle = entity.dataStorageHandle != DRW::NoHandle;
  const bool hasExplicitLink = hasExplicitKey || hasExplicitHandle;

  struct Candidate {
    std::size_t sectionIndex = 0;
    std::size_t recordIndex = 0;
    const DRW_DataStorageRecord *record = nullptr;
  };
  std::vector<Candidate> candidates;

  for (std::size_t sectionIndex = 0;
       sectionIndex < m_dataStorageSections.size(); ++sectionIndex) {
    const DRW_DataStorageSection &section = m_dataStorageSections[sectionIndex];
    if (section.m_version != DRW::UNKNOWNV && section.m_version != version)
      continue;

    const DRW_DataStorageRecord *byKey = nullptr;
    const DRW_DataStorageRecord *byHandle = nullptr;
    if (hasExplicitKey)
      byKey = section.findRecordByHandleKey(entity.dataStorageHandleKey);
    if (hasExplicitHandle)
      byHandle = section.findRecordByHandle(entity.dataStorageHandle);

    // An encoded lexical key and an encoded numeric handle are two views
    // of one identity.  They must agree; never use one to repair the
    // other or silently fall through to a different section.
    if (hasExplicitKey && hasExplicitHandle) {
      if (byKey == nullptr || byHandle == nullptr || byKey != byHandle)
        continue;
      candidates.push_back(
          {sectionIndex,
           static_cast<std::size_t>(byKey - section.records.data()), byKey});
      continue;
    }

    const DRW_DataStorageRecord *record = nullptr;
    if (hasExplicitKey)
      record = byKey;
    else if (hasExplicitHandle)
      record = byHandle;
    else if (entity.handle != DRW::NoHandle)
      record = section.findRecordByHandle(entity.handle);
    if (record == nullptr)
      continue;

    candidates.push_back(
        {sectionIndex,
         static_cast<std::size_t>(record - section.records.data()), record});
  }

  // There must be one section identity and one preferred record.  This is
  // deliberately checked before mutating the entity or link accounting.
  if (candidates.size() != 1u) {
    ++m_dataStorageLinkFailures;
    return;
  }

  const Candidate &candidate = candidates.front();
  const std::size_t sectionIndex = candidate.sectionIndex;
  const std::size_t recordIndex = candidate.recordIndex;
  const DRW_DataStorageRecord *record = candidate.record;
  const DRW_DataStorageSection &section = m_dataStorageSections[sectionIndex];
  if (recordIndex >= section.records.size()) {
    ++m_dataStorageLinkFailures;
    return;
  }

  if (m_dataStorageLinkedRecords.find({sectionIndex, recordIndex}) !=
      m_dataStorageLinkedRecords.end()) {
    ++m_dataStorageLinkFailures;
    return;
  }

  entity.hasDataStorageRecord = true;
  entity.dataStorageHandle = record->handle;
  entity.dataStorageHandleKey = record->handleKey;
  entity.dataStorageData = record->payload;
  entity.dataStorageSegmentIndex = record->segmentIndex;
  entity.dataStorageSchemaIndex = record->schemaIndex;
  entity.hasDataStoragePayloadMarker = record->hasPayloadMarker;
  entity.dataStoragePayloadMarkerOffset = record->payloadMarkerOffset;
  entity.dataStoragePayloadMarkerLength = record->payloadMarkerLength;
  entity.dataStoragePayloadMarkerSection = record->payloadMarkerSection;
  m_dataStorageLinkedRecords.emplace(sectionIndex, recordIndex);
}

void dwgReader::finalizeDataStorageLinks() {
  m_dataStorageOrphanRecords = 0;
  for (std::size_t sectionIndex = 0;
       sectionIndex < m_dataStorageSections.size(); ++sectionIndex) {
    DRW_DataStorageSection &section = m_dataStorageSections[sectionIndex];
    section.orphanRecordCount = 0;
    section.diagnostics.erase(
        std::remove_if(section.diagnostics.begin(), section.diagnostics.end(),
                       [](const DRW_DataStorageDiagnostic &diagnostic) {
                         return diagnostic.code == "datastorage-orphan-record";
                       }),
        section.diagnostics.end());
    for (std::size_t recordIndex = 0; recordIndex < section.records.size();
         ++recordIndex) {
      const DRW_DataStorageRecord &record = section.records[recordIndex];
      if (m_dataStorageLinkedRecords.find({sectionIndex, recordIndex}) !=
          m_dataStorageLinkedRecords.end()) {
        continue;
      }

      ++section.orphanRecordCount;
      ++m_dataStorageOrphanRecords;
      DRW_DataStorageDiagnostic diagnostic;
      diagnostic.code = "datastorage-orphan-record";
      diagnostic.message =
          "DataStorage record for handle " + record.handleKey +
          " was not referenced by a parsed modeler/surface entity";
      diagnostic.handle = record.handle;
      diagnostic.hasHandle = true;
      diagnostic.offset = record.recordOffset;
      diagnostic.hasOffset = true;
      section.diagnostics.push_back(std::move(diagnostic));
    }
  }
}

bool dwgReader::readMappedDwgEntity(dwgBuffer *dbuf, DwgFrameMapLease &lease,
                                    DRW_Interface &intfa, bool *frameFailure,
                                    DwgIntegrityAddressSpace offsetSpace,
                                    DwgEntityOutput *output,
                                    DwgMappedEntityCompletion completion,
                                    DwgBlockScopeTransaction *transaction) {
  if ((completion == DwgMappedEntityCompletion::Journal &&
       (output == nullptr || transaction == nullptr ||
        m_activeBlockTransaction != nullptr ||
        m_activeBlockOutput != nullptr)) ||
      (completion == DwgMappedEntityCompletion::Immediate &&
       (output != nullptr || transaction != nullptr))) {
    if (frameFailure != nullptr)
      *frameFailure = true;
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }
  if (!lease.isDetached() ||
      lease.origin != DwgFrameMapLease::Origin::ObjectMap ||
      lease.object.handle != lease.source.handle ||
      m_activeEntityFrameLease != nullptr) {
    if (frameFailure != nullptr)
      *frameFailure = true;
    recordEntityFailure(lease.object,
                        lease.classification.has_value()
                            ? lease.classification->resolvedType
                            : -1,
                        DwgEntityFailurePhase::Frame);
    return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                           true);
  }

  DwgFrameClassification observed;
  if (!lease.classification.has_value() ||
      !classifyDwgSourceFrame(dbuf, lease.object, observed) ||
      !classificationsMatch(*lease.classification, observed)) {
    recordObjectFrameFailure(lease.object, offsetSpace);
    if (lease.hasCoverage) {
      (void)markDwgFrameOutcome(lease.source, DRW_DwgFrameDisposition::Failed);
    }
    recordDwgFramePhaseSnapshot(lease,
                                DwgFramePhaseSnapshot::Destination::Failed,
                                DRW_DwgFrameDisposition::Failed);
    if (frameFailure != nullptr)
      *frameFailure = true;
    recordEntityFailure(lease.object,
                        lease.classification.has_value()
                            ? lease.classification->resolvedType
                            : -1,
                        DwgEntityFailurePhase::Classify);
    (void)discardDetachedDwgSourceFrame(lease);
    return false;
  }

  class ActiveLeaseScope {
  public:
    ActiveLeaseScope(DwgFrameMapLease *&slot,
                     DwgMappedEntityOutcome *&outcomeSlot,
                     DwgFrameMapLease &active, DwgMappedEntityOutcome &outcome)
        : m_slot(slot), m_outcomeSlot(outcomeSlot), m_previous(slot),
          m_previousOutcome(outcomeSlot) {
      m_slot = &active;
      m_outcomeSlot = &outcome;
    }

    ~ActiveLeaseScope() {
      m_slot = m_previous;
      m_outcomeSlot = m_previousOutcome;
    }

  private:
    DwgFrameMapLease *&m_slot;
    DwgMappedEntityOutcome *&m_outcomeSlot;
    DwgFrameMapLease *m_previous;
    DwgMappedEntityOutcome *m_previousOutcome;
  };

  DwgMappedEntityOutcome outcome = DwgMappedEntityOutcome::PublishedSimple;
  ActiveLeaseScope activeLeaseScope(
      m_activeEntityFrameLease, m_activeMappedEntityOutcome, lease, outcome);
  class ActiveBlockJournalScope {
  public:
    ActiveBlockJournalScope(DwgBlockScopeTransaction *&transactionSlot,
                            DwgEntityOutput *&outputSlot,
                            DwgBlockScopeTransaction *transaction,
                            DwgEntityOutput *output) noexcept
        : m_transactionSlot(transactionSlot), m_outputSlot(outputSlot),
          m_previousTransaction(transactionSlot), m_previousOutput(outputSlot) {
      if (transaction != nullptr) {
        m_transactionSlot = transaction;
        m_outputSlot = output;
      }
    }

    ~ActiveBlockJournalScope() {
      m_transactionSlot = m_previousTransaction;
      m_outputSlot = m_previousOutput;
    }

  private:
    DwgBlockScopeTransaction *&m_transactionSlot;
    DwgEntityOutput *&m_outputSlot;
    DwgBlockScopeTransaction *m_previousTransaction;
    DwgEntityOutput *m_previousOutput;
  };
  ActiveBlockJournalScope activeBlockJournalScope(
      m_activeBlockTransaction, m_activeBlockOutput, transaction, output);
  DwgImmediateEntityOutput immediateOutput(*this, intfa);
  DwgEntityOutput &entityOutput =
      output != nullptr ? *output
                        : static_cast<DwgEntityOutput &>(immediateOutput);
  DwgEntityOutput::SourceScope sourceScope =
      entityOutput.bindSource(lease.source);
  bool parsed = false;
  try {
    parsed = readDwgEntityWithOutput(dbuf, lease.object, intfa, entityOutput,
                                     frameFailure, offsetSpace);
  } catch (...) {
    parsed = false;
    if (frameFailure != nullptr)
      *frameFailure = true;
  }
  if (!parsed)
    outcome = DwgMappedEntityOutcome::Rejected;
  else if (completion == DwgMappedEntityCompletion::Journal &&
           outcome == DwgMappedEntityOutcome::PublishedSimple) {
    if (!stageDetachedDwgSourceFrame(lease)) {
      if (frameFailure != nullptr)
        *frameFailure = true;
      outcome = DwgMappedEntityOutcome::Rejected;
    } else {
      outcome = DwgMappedEntityOutcome::JournalledSimple;
    }
  }

  const auto discardFailed = [this, &lease]() {
    if (lease.hasCoverage) {
      const auto sourceIt = m_dwgSourceFrameIndexes.find(lease.source.handle);
      if (sourceIt == m_dwgSourceFrameIndexes.end() ||
          sourceIt->second >= m_dwgSourceFrameLedger.size()) {
        return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                               true);
      }
      const DRW_DwgFrameDisposition disposition =
          m_dwgSourceFrameLedger[sourceIt->second].m_disposition;
      if (disposition == DRW_DwgFrameDisposition::Pending &&
          !markDwgFrameOutcome(lease.source, DRW_DwgFrameDisposition::Failed)) {
        return false;
      }
      if (disposition != DRW_DwgFrameDisposition::Pending &&
          disposition != DRW_DwgFrameDisposition::Failed &&
          disposition != DRW_DwgFrameDisposition::Published) {
        return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                               true);
      }
    }
    return discardDetachedDwgSourceFrame(lease);
  };

  switch (outcome) {
  case DwgMappedEntityOutcome::PublishedSimple:
    if (completion != DwgMappedEntityCompletion::Immediate) {
      if (frameFailure != nullptr)
        *frameFailure = true;
      return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                             true);
    }
    if (!lease.isDetached()) {
      if (frameFailure != nullptr)
        *frameFailure = true;
      return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                             true);
    }
    if (lease.hasCoverage) {
      const auto sourceIt = m_dwgSourceFrameIndexes.find(lease.source.handle);
      if (sourceIt == m_dwgSourceFrameIndexes.end() ||
          sourceIt->second >= m_dwgSourceFrameLedger.size() ||
          m_dwgSourceFrameLedger[sourceIt->second].m_disposition !=
              DRW_DwgFrameDisposition::Published) {
        if (frameFailure != nullptr)
          *frameFailure = true;
        return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                               true);
      }
    }
    if (!discardDetachedDwgSourceFrame(lease)) {
      if (frameFailure != nullptr)
        *frameFailure = true;
      return false;
    }
    return true;

  case DwgMappedEntityOutcome::JournalledSimple:
    if (completion != DwgMappedEntityCompletion::Journal ||
        !lease.isDetached()) {
      if (frameFailure != nullptr)
        *frameFailure = true;
      return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                             true);
    }
    if (lease.hasCoverage) {
      const auto sourceIt = m_dwgSourceFrameIndexes.find(lease.source.handle);
      if (sourceIt == m_dwgSourceFrameIndexes.end() ||
          sourceIt->second >= m_dwgSourceFrameLedger.size() ||
          m_dwgSourceFrameLedger[sourceIt->second].m_disposition !=
              DRW_DwgFrameDisposition::Staged ||
          m_dwgSourceFrameLedger[sourceIt->second].m_publicationCount != 0) {
        if (frameFailure != nullptr)
          *frameFailure = true;
        return reportDwgFrameTransitionFailure(lease.source, lease.object.loc,
                                               true);
      }
    }
    return true;

  case DwgMappedEntityOutcome::StagedCompound:
  case DwgMappedEntityOutcome::CommittedCompound:
  case DwgMappedEntityOutcome::DeferredObject:
    if (!lease.isDetached())
      return true;
    if (frameFailure != nullptr)
      *frameFailure = true;
    (void)discardFailed();
    return false;

  case DwgMappedEntityOutcome::Rejected:
    if (!lease.isDetached())
      return false;
    if (!discardFailed() && frameFailure != nullptr)
      *frameFailure = true;
    return false;
  }
  return false;
}

bool dwgReader::isMappedInsertSequenceEnd(dwgBuffer *dbuf,
                                          const objHandle &object) {
  if (dbuf == nullptr)
    return false;

  dwgBuffer frameProbe = dbuf->forkIndependent();
  DwgObjectFrame frame;
  if (!frame.readAt(frameProbe, version, object.loc))
    return false;

  std::vector<std::uint8_t> &body = frame.body();
  dwgBuffer sequenceBuffer(body.data(), body.size(), &decoder);
  if (sequenceBuffer.getObjType(version) != dwgType::SEQEND ||
      !sequenceBuffer.isGood()) {
    return false;
  }
  sequenceBuffer.resetPosition();

  DRW_SeqEnd sequenceEnd;
  if (!sequenceEnd.parseDwg(version, &sequenceBuffer, frame.bodyBitSize()) ||
      !sequenceBuffer.isGood() || sequenceEnd.handle != object.handle ||
      sequenceEnd.parentHandle == DRW::NoHandle) {
    return false;
  }

  if (m_pendingInsertStates.find(sequenceEnd.parentHandle) !=
      m_pendingInsertStates.end()) {
    return true;
  }

  const auto objectIt = ObjectMap.find(sequenceEnd.parentHandle);
  const auto deferredIt = objObjectMap.find(sequenceEnd.parentHandle);
  if ((objectIt == ObjectMap.end()) == (deferredIt == objObjectMap.end()))
    return false;

  const objHandle &parent =
      objectIt != ObjectMap.end() ? objectIt->second : deferredIt->second;
  dwgBuffer parentProbe = dbuf->forkIndependent();
  DwgFrameClassification parentClassification;
  if (!classifyDwgSourceFrame(&parentProbe, parent, parentClassification) ||
      parentClassification.route != DwgFrameClassification::Route::Entity) {
    return false;
  }
  return parentClassification.resolvedType == dwgType::INSERT ||
         parentClassification.resolvedType == dwgType::MINSERT;
}

bool dwgReader::preflightMappedPolylineOwnership(
    const std::vector<const DRW_Block_Record *> &records, dwgBuffer *dbuf) {
  if (version < DRW::AC1018)
    return true;
  if (dbuf == nullptr)
    return false;

  struct PolylineClaims {
    const DRW_Block_Record *block{nullptr};
    std::uint32_t parent{DRW::NoHandle};
    std::int16_t type{-1};
    std::vector<std::uint32_t> children;
  };

  const auto isPolylineType = [](std::int16_t type) {
    return type == dwgType::POLYLINE_2D || type == dwgType::POLYLINE_3D ||
           type == dwgType::POLYLINE_PFACE || type == dwgType::POLYLINE_MESH;
  };
  const auto discardSource = [this](std::uint32_t handle) {
    const auto objectIt = ObjectMap.find(handle);
    if (objectIt != ObjectMap.end())
      return discardDwgSourceFrame(ObjectMap, objectIt);
    const auto deferredIt = objObjectMap.find(handle);
    return deferredIt == objObjectMap.end() ||
           discardDwgSourceFrame(objObjectMap, deferredIt);
  };

  try {
    std::vector<PolylineClaims> claims;
    std::unordered_set<std::uint32_t> invalidParents;

    for (const DRW_Block_Record *block : records) {
      if (block == nullptr)
        continue;
      for (const std::uint32_t parentHandle : block->entMap) {
        const auto objectIt = ObjectMap.find(parentHandle);
        if (objectIt == ObjectMap.end())
          continue;

        DwgFrameClassification classification;
        if (!classifyDwgSourceFrame(dbuf, objectIt->second, classification) ||
            classification.route != DwgFrameClassification::Route::Entity ||
            !isPolylineType(classification.resolvedType)) {
          continue;
        }

        dwgBuffer frameProbe = dbuf->forkIndependent();
        DwgObjectFrame frame;
        if (!frame.readAt(frameProbe, version, objectIt->second.loc))
          continue;
        std::vector<std::uint8_t> &body = frame.body();
        dwgBuffer bodyBuffer(body.data(), body.size(), &decoder);
        DRW_Polyline polyline;
        if (!polyline.parseDwg(version, &bodyBuffer, frame.bodyBitSize()) ||
            !bodyBuffer.isGood() || polyline.handle != parentHandle) {
          continue;
        }

        PolylineClaims parentClaims;
        parentClaims.block = block;
        parentClaims.parent = parentHandle;
        parentClaims.type = classification.resolvedType;
        parentClaims.children.reserve(polyline.hadlesList.size() + 1u);
        std::unordered_set<std::uint32_t> seenChildren;
        seenChildren.reserve(polyline.hadlesList.size() + 1u);
        for (const std::uint32_t child : polyline.hadlesList) {
          if (child == DRW::NoHandle || !seenChildren.insert(child).second) {
            invalidParents.insert(parentHandle);
            continue;
          }
          parentClaims.children.push_back(child);
        }
        const std::uint32_t sequenceEnd = polyline.seqEndH.ref;
        if (sequenceEnd == DRW::NoHandle ||
            !seenChildren.insert(sequenceEnd).second) {
          invalidParents.insert(parentHandle);
        } else {
          parentClaims.children.push_back(sequenceEnd);
        }
        claims.push_back(std::move(parentClaims));
      }
    }

    std::unordered_map<std::uint32_t, std::uint32_t> childOwners;
    childOwners.reserve(claims.size());
    for (const PolylineClaims &parentClaims : claims) {
      for (const std::uint32_t child : parentClaims.children) {
        const auto [owner, inserted] =
            childOwners.emplace(child, parentClaims.parent);
        if (!inserted && owner->second != parentClaims.parent) {
          invalidParents.insert(owner->second);
          invalidParents.insert(parentClaims.parent);
        }
      }
    }

    if (invalidParents.empty())
      return true;

    for (const PolylineClaims &parentClaims : claims) {
      if (invalidParents.find(parentClaims.parent) == invalidParents.end()) {
        continue;
      }
      objHandle parent;
      parent.handle = parentClaims.parent;
      recordEntityFailure(parent, parentClaims.type,
                          DwgEntityFailurePhase::Aggregate,
                          parentClaims.block->handle);
      try {
        m_invalidPolylineOwners.insert(parentClaims.parent);
      } catch (...) {
        // Quarantining the source frames below is sufficient to stop
        // a later recovery sweep from publishing this group.
      }
      (void)discardSource(parentClaims.parent);
      for (const std::uint32_t child : parentClaims.children)
        (void)discardSource(child);
    }
    if (invalidParents.size() >
        std::numeric_limits<std::size_t>::max() - m_entityParseFailures) {
      m_entityParseFailures = std::numeric_limits<std::size_t>::max();
    } else {
      m_entityParseFailures += invalidParents.size();
    }
    return false;
  } catch (...) {
    return false;
  }
}

bool dwgReader::requiresLegacyCompoundHandling(
    dwgBuffer *dbuf, const objHandle &object,
    DwgFrameClassification *classification) {
  DwgFrameClassification observed;
  if (!classifyDwgSourceFrame(dbuf, object, observed))
    return true;
  if (classification != nullptr)
    *classification = observed;
  if (observed.route != DwgFrameClassification::Route::Entity)
    return false;
  switch (observed.resolvedType) {
  case dwgType::POLYLINE_2D:
  case dwgType::POLYLINE_3D:
  case dwgType::POLYLINE_PFACE:
  case dwgType::POLYLINE_MESH:
    return false;
  case dwgType::VERTEX_2D:
  case dwgType::VERTEX_3D:
  case dwgType::VERTEX_MESH:
  case dwgType::VERTEX_PFACE:
  case dwgType::VERTEX_PFACE_FACE:
    return version < DRW::AC1018;
  case dwgType::SEQEND:
    return version < DRW::AC1018 && !isMappedInsertSequenceEnd(dbuf, object);
  default:
    return false;
  }
}

bool dwgReader::readDwgEntities(DRW_Interface &intfa, dwgBuffer *dbuf,
                                DwgIntegrityAddressSpace offsetSpace) {
  DRW_DBG("\nobject map total size= ");
  DRW_DBG(ObjectMap.size());
  // Bounded typed-body failures are warnings, not section failures. A bad
  // object frame is different: it is a structural desynchronization risk
  // and must fail the ENTITIES phase without publishing that entity.
  size_t failures = 0;
  bool structuralFailure = false;
  const bool previousSweepPolicy = rejectOwnedEntityInSweep;
  rejectOwnedEntityInSweep = version > DRW::AC1015;
  while (!ObjectMap.empty()) {
    auto itB = ObjectMap.begin();
    if (m_quarantinedEntityHandles.find(itB->first) !=
        m_quarantinedEntityHandles.end()) {
      if (!discardDwgSourceFrame(ObjectMap, itB)) {
        structuralFailure = true;
        break;
      }
      continue;
    }
    bool frameFailure = false;
    bool read = false;
    DwgFrameClassification classification;
    if (requiresLegacyCompoundHandling(dbuf, itB->second, &classification)) {
      DwgSourceFrameLease lease;
      if (!takeDwgSourceFrame(ObjectMap, itB, lease)) {
        structuralFailure = true;
        break;
      }
      read =
          readDwgEntity(dbuf, lease.object, intfa, &frameFailure, offsetSpace);
      if (!read) {
        (void)markDwgFrameOutcome(lease.source,
                                  DRW_DwgFrameDisposition::Failed);
      }
    } else {
      DwgFrameMapLease lease;
      if (!detachDwgSourceFrame(ObjectMap, itB, lease)) {
        structuralFailure = true;
        break;
      }
      lease.classification.emplace(std::move(classification));
      read =
          readMappedDwgEntity(dbuf, lease, intfa, &frameFailure, offsetSpace);
    }
    if (!read) {
      ++failures;
    }
    structuralFailure = structuralFailure || frameFailure ||
                        parsedEntityHandleMismatch || parsedEntityOwnerMismatch;
  }
  rejectOwnedEntityInSweep = previousSweepPolicy;
  if (failures > 0) {
    DRW_DBG("readDwgEntities: ");
    DRW_DBG(failures);
    DRW_DBG(" entities failed to parse (warnings, not section failure)\n");
    m_entityParseFailures += failures;
  }

  if (!validateDeferredCompoundState())
    structuralFailure = true;

  // A mapped compound can remain internally consistent while still lacking
  // a parent or a declared child. At the end of the entity sweep that is a
  // structural failure, not a recoverable deferred state.
  if (hasPendingCompoundState()) {
    structuralFailure = true;
  }

  abandonDeferredCompoundState();

  return !structuralFailure;
}

bool dwgReader::validateDeferredCompoundState() {
  return validateStagedCompoundState();
}

void dwgReader::abandonDeferredCompoundState() {
  (void)abandonStagedCompoundState();
}

/**
 * Reads a dwg drawing entity (dwg object entity) given its offset in the file
 */
bool dwgReader::readDwgEntity(dwgBuffer *dbuf, objHandle &obj,
                              DRW_Interface &intfa, bool *frameFailure,
                              DwgIntegrityAddressSpace offsetSpace) {
  DwgImmediateEntityOutput output(*this, intfa);
  return readDwgEntityWithOutput(dbuf, obj, intfa, output, frameFailure,
                                 offsetSpace);
}

bool dwgReader::readDwgEntityWithOutput(dwgBuffer *dbuf, objHandle &obj,
                                        DRW_Interface &intfa,
                                        DwgEntityOutput &output,
                                        bool *frameFailure,
                                        DwgIntegrityAddressSpace offsetSpace) {
  bool ret = true;
  m_currentEntityFailurePhase = DwgEntityFailurePhase::None;
  expectedParsedEntityHandle = obj.handle;
  parsedEntityHandleMismatch = false;
  parsedEntityOwnerMismatch = false;
  if (frameFailure)
    *frameFailure = false;

  if (dbuf == nullptr) {
    recordObjectFrameFailure(obj, offsetSpace);
    recordEntityFailure(obj, -1, DwgEntityFailurePhase::Frame);
    if (frameFailure)
      *frameFailure = true;
    return false;
  }

  nextEntLink = prevEntLink = 0; // set to 0 to skip unimplemented entities
  nextEntLinkImplicit = false;
  try {
    DwgObjectFrame frame;
    if (!frame.readAt(*dbuf, version, obj.loc)) {
      recordObjectFrameFailure(obj, offsetSpace);
      recordEntityFailure(obj, -1, DwgEntityFailurePhase::Frame);
      if (frameFailure)
        *frameFailure = true;
      DRW_DBG(" Warning: readDwgEntity, invalid object frame\n");
      return false;
    }
    const std::uint32_t bs = frame.bodyBitSize();
    auto &tmpByteStr = frame.body();
    const std::size_t size = tmpByteStr.size();
    dwgBuffer buff(tmpByteStr.data(), size, &decoder);
    std::int16_t oType = buff.getObjType(version);
    if (!buff.isGood()) {
      recordObjectFrameFailure(obj, offsetSpace);
      recordEntityFailure(obj, -1, DwgEntityFailurePhase::Frame);
      if (frameFailure)
        *frameFailure = true;
      DRW_DBG(" Warning: readDwgEntity, missing object type\n");
      return false;
    }
    const std::int16_t encodedType = oType;
    buff.resetPosition();
    auto makeRawEntity = [&](int rawType, const DRW_Class *cls = nullptr,
                             bool hasDataStorage = false,
                             std::uint32_t parentHandle = DRW::NoHandle,
                             const DRW_Entity *commonEntity = nullptr,
                             bool typedPeer = true) {
      DRW_UnsupportedObject raw;
      raw.m_version = version;
      raw.m_objectType = rawType;
      raw.m_handle = obj.handle;
      // A BLOCK_RECORD walk can establish reachability without proving the
      // entity's common owner link. Keep those facts separate below.
      raw.m_parentHandle = DRW::NoHandle;
      if (commonEntity != nullptr) {
        raw.setCommonLinkEvidence(drwDwgCommonLinkEvidenceForLinks(
            commonEntity->hasDwgCommonLinkTail(), commonEntity->parentHandle,
            commonEntity->reactorHandles, commonEntity->dwgReactorCount(),
            commonEntity->xDictHandle));
        if (raw.m_commonLinkEvidence != DRW_DwgCommonLinkEvidence::Unknown) {
          raw.m_parentHandle = commonEntity->parentHandle;
          raw.m_reactorHandles = commonEntity->reactorHandles;
          raw.m_xDictHandle = commonEntity->xDictHandle;
          raw.m_numReactors = commonEntity->dwgReactorCount();
          raw.m_xDictFlag = commonEntity->dwgXDictionaryFlag();
        }
      }
      raw.m_blockOwnerHandle = rawBlockEntityOwner != DRW::NoHandle
                                   ? rawBlockEntityOwner
                                   : parentHandle;
      raw.m_bodyBitSize = bs;
      raw.m_objectOffset = obj.loc;
      raw.m_objectSize = static_cast<std::uint32_t>(size);
      raw.m_isEntity = true;
      raw.m_isCustomClass = cls != nullptr;
      raw.m_hasDataStorage = hasDataStorage;
      if (cls != nullptr) {
        raw.m_hasClassDefinition = true;
        raw.m_classProxyFlag = static_cast<std::uint16_t>(cls->proxyFlag);
        raw.m_classAppName = cls->appName;
        raw.m_classWasProxy = cls->wasaProxyFlag != 0;
        raw.m_classEntityFlagRaw = cls->entityFlagRaw;
        raw.m_classDwgVersion = cls->dwgVersion;
        raw.m_classMaintenanceVersion = cls->maintenanceVersion;
        raw.m_classUnknown1 = cls->unknown1;
        raw.m_classUnknown2 = cls->unknown2;
        raw.m_recordName = cls->recName;
        raw.m_className = cls->className;
      }
      raw.m_rawBytes = tmpByteStr;
      if (m_activeEntityFrameCapture != nullptr &&
          m_activeEntityFrameCapture->publication.m_handle == obj.handle) {
        DRW_DwgFramePublication &publication =
            m_activeEntityFrameCapture->publication;
        publication.setCommonLinkEvidence(raw.m_commonLinkEvidence);
        publication.m_parentHandle = raw.m_parentHandle;
        publication.m_reactorHandles = raw.m_reactorHandles;
        publication.m_xDictHandle = raw.m_xDictHandle;
        publication.m_numReactors = raw.m_numReactors;
        publication.m_xDictFlag = raw.m_xDictFlag;
        m_activeEntityFrameCapture->rawViewIssued = true;
        m_activeEntityFrameCapture->rawViewHasTypedPeer =
            m_activeEntityFrameCapture->rawViewHasTypedPeer || typedPeer;
      }
      return raw;
    };
    auto deferObject = [&](const objHandle &candidate) {
      if (m_activeEntityFrameLease != nullptr) {
        DwgFrameMapLease &activeLease = *m_activeEntityFrameLease;
        if (!activeLease.isDetached() ||
            activeLease.object.handle != candidate.handle ||
            activeLease.source.handle != candidate.handle) {
          ret = false;
          return reportDwgFrameTransitionFailure(activeLease.source,
                                                 activeLease.object.loc, true);
        }
        if (!deferDetachedDwgSourceFrame(activeLease, objObjectMap)) {
          DRW_DBG("duplicate deferred object handle: ");
          DRW_DBGH(candidate.handle);
          DRW_DBG("\n");
          ret = false;
          return false;
        }
        if (m_activeMappedEntityOutcome != nullptr) {
          *m_activeMappedEntityOutcome = DwgMappedEntityOutcome::DeferredObject;
        }
        return true;
      }
      DwgSourceFrameLease lease;
      lease.object = candidate;
      lease.source = sourceFrameId(candidate);
      lease.hasCoverage =
          m_dwgFrameCoverageStatus != DRW_DwgFrameCoverageStatus::NotAvailable;
      if (!deferDwgSourceFrame(lease, objObjectMap)) {
        DRW_DBG("duplicate deferred object handle: ");
        DRW_DBGH(candidate.handle);
        DRW_DBG("\n");
        ret = false;
        return false;
      }
      return true;
    };
    // WIPEOUT and DBCOLOR use fixed DWG types above 499. Other values in
    // that range are file-local CLASSES ordinals and must be resolved before
    // dispatch.
    const DRW_Class *resolvedClass = nullptr;
    bool unresolvedCustomClass = false;
    const bool fixedEntityShell =
        version >= DRW::AC1021 &&
        DRW_UnsupportedObject::isFixedEntityShellType(oType);
    const bool fixedObjectShell =
        version >= DRW::AC1021 &&
        DRW_UnsupportedObject::isFixedObjectShellType(oType);
    if (oType > dwgObjType::PROXY_OBJECT && !dwgObjType::isFixedObject(oType) &&
        !fixedEntityShell && !fixedObjectShell && oType != dwgType::WIPEOUT) {
      auto it = classesmap.find(oType);
      if (it == classesmap.end()) { // preserve unknown custom objects
        unresolvedCustomClass = true;
        if (expectedBlockEntityOwner == DRW::NoHandle &&
            !(rejectOwnedEntityInSweep && version > DRW::AC1015)) {
          // Without an entity owner, preserve it as an opaque OBJECTS
          // record; a known block owner is handled by the raw entity
          // path below.
          DRW_DBG("Class ");
          DRW_DBG(oType);
          DRW_DBG("not found, defer handle: ");
          DRW_DBG(obj.handle);
          DRW_DBG("\n");
          obj.type = oType;
          return deferObject(obj);
        }
      } else {
        DRW_Class *cl = it->second;
        resolvedClass = cl;
        if (cl->dwgType != 0)
          oType = cl->dwgType;
      }
    }

    // Fixed OBJECTS records are not entity candidates. Defer them before the
    // entity owner probe and before dispatch, including fixed codes above the
    // custom-class range such as BLOCKREPRESENTATION.
    if (dwgObjType::isFixedObject(oType) || fixedObjectShell) {
      obj.type = oType;
      if (!deferObject(obj))
        return false;
      return true;
    }

    // CLASSES item_class_id 0x1F3 identifies an OBJECTS record. Defer it
    // before the entity owner probe: object owners commonly point to the
    // named-object dictionary and must not be mistaken for stray entities.
    if (resolvedClass != nullptr && resolvedClass->entityFlag == 0) {
      obj.type = oType;
      if (!deferObject(obj))
        return false;
      return true;
    }

    // Typed entities validate their owner in entryParse(). Opaque/custom
    // entities have no typed parser, so probe the common entity header and
    // handle stream on an independent cursor. This keeps a valid raw body
    // lossless without publishing it under the wrong BLOCK_RECORD or outside
    // its owner block.
    if (expectedBlockEntityOwner != DRW::NoHandle || ownerlessSpaceWalk ||
        (rejectOwnedEntityInSweep && version > DRW::AC1015)) {
      ProxyHostEntity ownerProbe;
      dwgBuffer ownerBuffer = buff.forkIndependent();
      const bool compoundChild =
          oType == dwgType::ATTRIB || oType == dwgType::SEQEND ||
          oType == dwgType::VERTEX_2D || oType == dwgType::VERTEX_3D ||
          oType == dwgType::VERTEX_MESH || oType == dwgType::VERTEX_PFACE ||
          oType == dwgType::VERTEX_PFACE_FACE;
      if (!compoundChild) {
        const bool parsedCommon =
            ownerProbe.parseDwg(version, &ownerBuffer, bs);
        // R2004 has no independent handle-stream offset after an opaque
        // entity body. Later versions expose the boundary through the
        // object-size/string-stream metadata and can be checked exactly.
        const bool parsedHandles =
            parsedCommon && version > DRW::AC1018 &&
            ownerProbe.parseDwgEntHandle(version, &ownerBuffer) &&
            ownerBuffer.isGood();
        const DwgEntityFailurePhase ownerProbeFailurePhase =
            !parsedCommon ? DwgEntityFailurePhase::TypedBody
                          : (version > DRW::AC1018 && !parsedHandles
                                 ? DwgEntityFailurePhase::CommonHandles
                                 : DwgEntityFailurePhase::Identity);
        if (ownerlessSpaceWalk) {
          const bool ownerMismatch = ownerProbe.hasOwnerHandle();
          const bool identityMismatch =
              expectedParsedEntityHandle != DRW::NoHandle &&
              ownerProbe.handle != expectedParsedEntityHandle;
          if (identityMismatch)
            parsedEntityHandleMismatch = true;
          if (identityMismatch || ownerMismatch) {
            parsedEntityOwnerMismatch = ownerMismatch;
            m_currentEntityFailurePhase = ownerProbeFailurePhase;
            recordObjectFrameFailure(obj, offsetSpace);
            recordEntityFailure(obj, oType, m_currentEntityFailurePhase);
            if (frameFailure)
              *frameFailure = true;
            return false;
          }
        } else if (expectedBlockEntityOwner != DRW::NoHandle) {
          const bool ownerMismatch =
              !ownerProbe.hasOwnerHandle() ||
              (version > DRW::AC1018 &&
               (!parsedHandles ||
                ownerProbe.parentHandle != expectedBlockEntityOwner));
          const bool identityMismatch =
              expectedParsedEntityHandle != DRW::NoHandle &&
              ownerProbe.handle != expectedParsedEntityHandle;
          if (!parsedCommon || identityMismatch || ownerMismatch) {
            parsedEntityOwnerMismatch = true;
            m_currentEntityFailurePhase = ownerProbeFailurePhase;
            recordObjectFrameFailure(obj, offsetSpace);
            recordEntityFailure(obj, oType, m_currentEntityFailurePhase);
            if (frameFailure)
              *frameFailure = true;
            return false;
          }
        } else if (parsedCommon && ownerProbe.hasOwnerHandle()) {
          if (rejectOwnedEntityInSweep && ownerProbe.hasOwnerHandle()) {
            // A non-null owner is only safe to publish from the
            // corresponding BLOCK_RECORD walk. The ownership list is
            // authoritative for R2004+; a stale or missing block
            // record must not turn an owned entity into a top-level
            // callback during the recovery sweep.
            parsedEntityOwnerMismatch = true;
            m_currentEntityFailurePhase = DwgEntityFailurePhase::Identity;
            recordEntityFailure(obj, oType, m_currentEntityFailurePhase);
            return false;
          }
        }
      }
    }

    DwgEntityFramePublicationCapture framePublication;
    framePublication.publication.m_version = version;
    framePublication.publication.m_handle = obj.handle;
    framePublication.publication.m_sourceOffset = obj.loc;
    framePublication.publication.m_sourceMapOrdinal = obj.sourceOrdinal;
    framePublication.publication.m_sourceOffsetSpace = obj.sourceOffsetSpace;
    framePublication.publication.m_hasSourceLocation = true;
    framePublication.publication.m_encodedType = encodedType;
    framePublication.publication.m_resolvedType = oType;
    framePublication.publication.m_isEntity = true;
    framePublication.publication.m_isCustomClass = resolvedClass != nullptr;
    if (resolvedClass != nullptr) {
      framePublication.publication.m_recordName = resolvedClass->recName;
      framePublication.publication.m_className = resolvedClass->className;
    }
    m_activeEntityFrameCapture = &framePublication;
    bool framePublicationPublished = false;
    bool compoundFrameHandled = false;
    class ProxyOutputSink final : public DRW_ProxyGraphicSink {
    public:
      explicit ProxyOutputSink(DwgEntityOutput &output) noexcept
          : m_output(output) {}

      bool addArc(const DRW_Arc &value) override {
        try {
          m_output.appendValue(value, &DRW_Interface::addArc);
          return true;
        } catch (...) {
          return false;
        }
      }
      bool addCircle(const DRW_Circle &value) override {
        try {
          m_output.appendValue(value, &DRW_Interface::addCircle);
          return true;
        } catch (...) {
          return false;
        }
      }
      bool addEllipse(const DRW_Ellipse &value) override {
        try {
          m_output.appendValue(value, &DRW_Interface::addEllipse);
          return true;
        } catch (...) {
          return false;
        }
      }
      bool addLWPolyline(const DRW_LWPolyline &value) override {
        try {
          m_output.appendValue(value, &DRW_Interface::addLWPolyline);
          return true;
        } catch (...) {
          return false;
        }
      }
      bool addMesh(const DRW_Mesh &value) override {
        try {
          m_output.appendValue(value, &DRW_Interface::addMesh);
          return true;
        } catch (...) {
          return false;
        }
      }
      bool addPolyline(const DRW_Polyline &value) override {
        try {
          m_output.appendValue(value, &DRW_Interface::addPolyline);
          return true;
        } catch (...) {
          return false;
        }
      }
      bool addText(const DRW_Text &value) override {
        try {
          m_output.appendValue(value, &DRW_Interface::addText);
          return true;
        } catch (...) {
          return false;
        }
      }

    private:
      DwgEntityOutput &m_output;
    };
    const auto decodeProxyGraphics =
        [this, &output](const DRW_Entity &host,
                        std::size_t carrierAndReceiptEventCount) {
          if (host.proxyGraphics.empty())
            return true;

          const DRW_ProxyGraphicDecodeResult preflight =
              DRW_ProxyGraphicDecoder::inspect(host.proxyGraphics);
          switch (preflight.stopReason) {
          case DRW_ProxyGraphicStopReason::Complete:
          case DRW_ProxyGraphicStopReason::ShortHeader:
          case DRW_ProxyGraphicStopReason::InvalidChunkSize:
          case DRW_ProxyGraphicStopReason::TruncatedChunk:
            break;
          default:
            return false;
          }

          if (m_activeBlockTransaction != nullptr) {
            const std::size_t requiredEventCount =
                preflight.recognizedPrimitiveChunkCount +
                carrierAndReceiptEventCount;
            // The block walker has already reserved the standard
            // semantic-event plus receipt budget for this source.
            if (requiredEventCount > 3u &&
                !m_activeBlockTransaction->reserveAdmission(
                    0u, requiredEventCount - 3u, 0u)) {
              return false;
            }
          }

          ProxyOutputSink sink(output);
          const DRW_ProxyGraphicDecodeResult decoded =
              DRW_ProxyGraphicDecoder::decode(host.proxyGraphics, version, sink,
                                              host, m_layerNameOrder,
                                              m_ltypeNameOrder);
          switch (decoded.stopReason) {
          case DRW_ProxyGraphicStopReason::Complete:
          case DRW_ProxyGraphicStopReason::ShortHeader:
          case DRW_ProxyGraphicStopReason::InvalidChunkSize:
          case DRW_ProxyGraphicStopReason::TruncatedChunk:
            break;
          default:
            return false;
          }
          if (decoded.emittedPrimitiveCount >
              preflight.recognizedPrimitiveChunkCount) {
            return false;
          }
          if (decoded.emittedPrimitiveCount >
              std::numeric_limits<std::size_t>::max() -
                  m_decodedProxyPrimitives) {
            return false;
          }
          m_decodedProxyPrimitives += decoded.emittedPrimitiveCount;
          return true;
        };
    const auto materializeCurrentFramePublication =
        [&framePublication](DRW_DwgFramePublication &publication) {
          if (!framePublication.typedViewParsed &&
              !framePublication.rawViewIssued)
            return false;
          publication = framePublication.publication;
          publication.m_carrier =
              framePublication.rawViewIssued
                  ? (framePublication.rawViewHasTypedPeer
                         ? DRW_DwgFramePublication::Carrier::TypedAndRaw
                         : DRW_DwgFramePublication::Carrier::Raw)
                  : DRW_DwgFramePublication::Carrier::Typed;
          return true;
        };
    obj.type = oType;
    if (fixedEntityShell) {
      RawEntityShell shell;
      if (entryParse(shell, buff, bs, ret)) {
        DRW_UnsupportedObject raw =
            makeRawEntity(oType, nullptr, shell.hasDataStorageBinaryData(),
                          shell.parentHandle, &shell, false);
        raw.m_recordName = DRW_UnsupportedObject::fixedEntityShellName(oType);
        framePublication.publication.m_recordName = raw.m_recordName;
        output.appendValue(raw, &DRW_Interface::addUnsupportedObject);
      }
    }
    if (!fixedEntityShell)
      switch (oType) {
      case dwgType::TEXT: {
        DRW_Text e;
        if (entryParse(e, buff, bs, ret)) {
          e.style = findTableName(DRW::STYLE, e.styleH.ref);
          output.appendValue(e, &DRW_Interface::addText);
        }
        break;
      }
      case dwgType::ATTRIB: {
        auto a = std::make_shared<DRW_Attrib>();
        bool localRet = true;
        entryParse(*a, buff, bs, localRet);
        const std::uint32_t ownerH = a->parentHandle;
        if (m_activeEntityFrameLease != nullptr) {
          compoundFrameHandled = true;
          if (!localRet) {
            if (ownerH != DRW::NoHandle) {
              abandonPendingInsertState(ownerH);
              terminalizeOrphanAttribOwner(ownerH);
            }
            ret = false;
            break;
          }
          a->style = findTableName(DRW::STYLE, a->styleH.ref);
          DRW_DwgFramePublication attribPublication;
          if (!materializeCurrentFramePublication(attribPublication)) {
            ret = false;
            break;
          }
          const DwgMappedEntityOutcome outcome =
              stagePendingAttribute(std::move(a), attribPublication, intfa);
          ret = outcome != DwgMappedEntityOutcome::Rejected;
          if (!ret)
            parsedEntityOwnerMismatch = true;
          if (m_activeMappedEntityOutcome != nullptr)
            *m_activeMappedEntityOutcome = outcome;
          break;
        }
        // INSERT-owned attributes require a detached HANDLE-map frame.
        // Direct reader calls have no source ownership to stage, so they
        // cannot establish a second compound state machine.
        ret = false;
        break;
      }
      case dwgType::ATTDEF: {
        auto a = std::make_shared<DRW_Attdef>();
        bool localRet = true;
        entryParse(*a, buff, bs, localRet);
        if (localRet) {
          a->style = findTableName(DRW::STYLE, a->styleH.ref);
          // ATTDEF belongs to its BLOCK definition, not to an INSERT's
          // trailing ATTRIB sequence. Routing it through the latter
          // turns valid block text into an orphan and drops it at EOF.
          output.appendValue(*a, &DRW_Interface::addAttDef);
        } else {
          DRW_DBG("[attdef parse failed, handle ");
          DRW_DBG(obj.handle);
          DRW_DBG("]\n");
        }
        ret = localRet;
        break;
      }
      case dwgType::SEQEND: {
        DRW_SeqEnd sequenceEnd;
        if (m_activeEntityFrameLease != nullptr) {
          compoundFrameHandled = true;
          if (m_consumedSeqEndHandles.find(obj.handle) !=
                  m_consumedSeqEndHandles.end() ||
              !entryParse(sequenceEnd, buff, bs, ret)) {
            if (sequenceEnd.parentHandle != DRW::NoHandle)
              abandonPendingInsertState(sequenceEnd.parentHandle);
            ret = false;
            break;
          }
          DRW_DwgFramePublication sequencePublication;
          if (!materializeCurrentFramePublication(sequencePublication)) {
            ret = false;
            break;
          }
          const DwgMappedEntityOutcome outcome = stagePendingSeqEnd(
              obj.handle, sequenceEnd.parentHandle, sequencePublication, intfa);
          ret = outcome != DwgMappedEntityOutcome::Rejected;
          if (!ret)
            parsedEntityOwnerMismatch = true;
          if (!ret) {
            try {
              m_invalidSeqEndHandles.insert(obj.handle);
            } catch (...) {
              // The frame is already rejected; duplicate suppression
              // is best effort when tracking it runs out of memory.
            }
          }
          if (m_activeMappedEntityOutcome != nullptr)
            *m_activeMappedEntityOutcome = outcome;
          break;
        }
        // A SEQEND has no standalone entity meaning. The owning INSERT
        // or POLYLINE must stage it from a bounded source frame.
        ret = false;
        break;
      }
      case dwgType::INSERT:
      case dwgType::MINSERT: {
        DRW_Insert e;
        if (m_activeEntityFrameLease != nullptr) {
          compoundFrameHandled = true;
          if (!entryParse(e, buff, bs, ret)) {
            terminalizeOrphanAttribOwner(obj.handle);
            ret = false;
            break;
          }
          e.name = findTableName(DRW::BLOCK_RECORD, e.blockRecH.ref);
          DRW_DwgFramePublication insertPublication;
          if (!materializeCurrentFramePublication(insertPublication)) {
            terminalizeOrphanAttribOwner(e.handle);
            ret = false;
            break;
          }
          const DwgMappedEntityOutcome outcome =
              version >= DRW::AC1018
                  ? stageMappedInsertAggregate(std::move(e), insertPublication,
                                               dbuf, intfa, offsetSpace)
                  : stageLegacyInsertAggregate(std::move(e), insertPublication,
                                               dbuf, intfa, offsetSpace);
          ret = outcome != DwgMappedEntityOutcome::Rejected;
          if (!ret)
            parsedEntityOwnerMismatch = true;
          if (m_activeMappedEntityOutcome != nullptr)
            *m_activeMappedEntityOutcome = outcome;
          if (!ret)
            terminalizeOrphanAttribOwner(obj.handle);
          break;
        }
        // INSERT-family frames are admitted only through the mapped
        // aggregate. Parse within the frame boundary, but do not publish
        // or retain source-less ownership state.
        (void)entryParse(e, buff, bs, ret);
        ret = false;
        break;
      }
      case dwgType::VERTEX_2D:
      case dwgType::VERTEX_3D:
      case dwgType::VERTEX_MESH:
      case dwgType::VERTEX_PFACE:
      case dwgType::VERTEX_PFACE_FACE: {
        DRW_Vertex vertex;
        if (m_activeEntityFrameLease == nullptr || version < DRW::AC1018) {
          ret = false;
          break;
        }
        compoundFrameHandled = true;
        if (!vertex.parseDwg(version, &buff, bs, 0.0) || !buff.isGood()) {
          ret = false;
          break;
        }
        if (vertex.handle != expectedParsedEntityHandle) {
          parsedEntityHandleMismatch = true;
          ret = false;
          break;
        }
        // VERTEX ownership is its containing POLYLINE, rather than the
        // surrounding BLOCK_RECORD.  It therefore cannot use entryParse's
        // block-owner check, but still needs the same typed frame capture
        // before the staged aggregate may claim its source frame.
        parseAttribs(&vertex);
        framePublication.publication.setCommonLinkEvidence(
            drwDwgCommonLinkEvidenceForLinks(
                vertex.hasDwgCommonLinkTail(), vertex.parentHandle,
                vertex.reactorHandles, vertex.dwgReactorCount(),
                vertex.xDictHandle));
        framePublication.publication.m_parentHandle = vertex.parentHandle;
        framePublication.publication.m_reactorHandles = vertex.reactorHandles;
        framePublication.publication.m_xDictHandle = vertex.xDictHandle;
        framePublication.publication.m_numReactors = vertex.dwgReactorCount();
        framePublication.publication.m_xDictFlag = vertex.dwgXDictionaryFlag();
        framePublication.typedViewParsed = true;
        DRW_DwgFramePublication vertexPublication;
        if (!materializeCurrentFramePublication(vertexPublication)) {
          ret = false;
          break;
        }
        const DwgMappedEntityOutcome outcome = stagePendingPolylineVertex(
            std::move(vertex), vertexPublication, intfa);
        ret = outcome != DwgMappedEntityOutcome::Rejected;
        if (!ret)
          parsedEntityOwnerMismatch = true;
        if (m_activeMappedEntityOutcome != nullptr)
          *m_activeMappedEntityOutcome = outcome;
        break;
      }
      case dwgType::POLYLINE_2D:
      case dwgType::POLYLINE_3D:
      case dwgType::POLYLINE_PFACE:
      case dwgType::POLYLINE_MESH: {
        DRW_Polyline e;
        if (m_activeEntityFrameLease == nullptr) {
          // A POLYLINE owns VERTEX and SEQEND source frames. Without a
          // detached parent lease there is no transactional authority
          // to claim that aggregate, so direct parser entry is refused.
          ret = false;
          break;
        }
        compoundFrameHandled = true;
        if (!entryParse(e, buff, bs, ret)) {
          ret = false;
          break;
        }
        DRW_DwgFramePublication polylinePublication;
        if (!materializeCurrentFramePublication(polylinePublication)) {
          ret = false;
          break;
        }
        const DwgMappedEntityOutcome outcome =
            version >= DRW::AC1018
                ? stageMappedPolylineAggregate(std::move(e),
                                               polylinePublication, dbuf, intfa,
                                               offsetSpace)
                : stageLegacyPolylineChain(std::move(e), polylinePublication,
                                           dbuf, intfa, offsetSpace);
        ret = outcome != DwgMappedEntityOutcome::Rejected;
        if (!ret)
          parsedEntityOwnerMismatch = true;
        if (m_activeMappedEntityOutcome != nullptr)
          *m_activeMappedEntityOutcome = outcome;
        break;
      }
      case dwgType::ARC: {
        DRW_Arc e;
        if (entryParse(e, buff, bs, ret)) {
          emitWithExtrusion(e, output, &DRW_Interface::addArc);
        }
        break;
      }
      case dwgType::CIRCLE: {
        DRW_Circle e;
        if (entryParse(e, buff, bs, ret)) {
          emitWithExtrusion(e, output, &DRW_Interface::addCircle);
        }
        break;
      }
      case dwgType::LINE: {
        DRW_Line e;
        if (entryParse(e, buff, bs, ret)) {
          output.appendValue(e, &DRW_Interface::addLine);
        }
        break;
      }
      case dwgType::THREEDLINE: {
        DRW_3DLine e;
        if (entryParse(e, buff, bs, ret)) {
          output.appendValue(e, &DRW_Interface::add3DLine);
        }
        break;
      }
      case dwgType::DIM_ORDINATE: {
        DRW_DimOrdinate e;
        if (entryParse(e, buff, bs, ret)) {
          e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
          output.appendValue(e, &DRW_Interface::addDimOrdinate);
        }
        break;
      }
      case dwgType::DIM_LINEAR: {
        DRW_DimLinear e;
        if (entryParse(e, buff, bs, ret)) {
          e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
          output.appendValue(e, &DRW_Interface::addDimLinear);
        }
        break;
      }
      case dwgType::DIM_ALIGNED: {
        DRW_DimAligned e;
        if (entryParse(e, buff, bs, ret)) {
          e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
          output.appendValue(e, &DRW_Interface::addDimAlign);
        }
        break;
      }
      case dwgType::DIM_ANGULAR3P: {
        DRW_DimAngular3p e;
        if (entryParse(e, buff, bs, ret)) {
          e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
          output.appendValue(e, &DRW_Interface::addDimAngular3P);
        }
        break;
      }
      case dwgType::DIM_ANGULAR: {
        DRW_DimAngular e;
        if (entryParse(e, buff, bs, ret)) {
          e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
          output.appendValue(e, &DRW_Interface::addDimAngular);
        }
        break;
      }
      case dwgType::DIM_RADIAL: {
        DRW_DimRadial e;
        if (entryParse(e, buff, bs, ret)) {
          e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
          output.appendValue(e, &DRW_Interface::addDimRadial);
        }
        break;
      }
      case dwgType::DIM_DIAMETRIC: {
        DRW_DimDiametric e;
        if (entryParse(e, buff, bs, ret)) {
          e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
          output.appendValue(e, &DRW_Interface::addDimDiametric);
        }
        break;
      }
      case dwgType::POINT: {
        DRW_Point e;
        if (entryParse(e, buff, bs, ret)) {
          output.appendValue(e, &DRW_Interface::addPoint);
        }
        break;
      }
      case dwgType::FACE3D: {
        DRW_3Dface e;
        if (entryParse(e, buff, bs, ret)) {
          output.appendValue(e, &DRW_Interface::add3dFace);
        }
        break;
      }
      case dwgType::SOLID: {
        DRW_Solid e;
        if (entryParse(e, buff, bs, ret)) {
          emitWithExtrusion(e, output, &DRW_Interface::addSolid);
        }
        break;
      }
      case dwgType::TRACE: {
        DRW_Trace e;
        if (entryParse(e, buff, bs, ret)) {
          emitWithExtrusion(e, output, &DRW_Interface::addTrace);
        }
        break;
      }
      case dwgType::SHAPE: {
        DRW_Shape e;
        if (entryParse(e, buff, bs, ret)) {
          e.m_objectSize = static_cast<std::uint32_t>(size);
          e.m_rawBytes = tmpByteStr;
          e.m_styleName = findTableName(DRW::STYLE, e.m_shapeFileHandle);
          output.appendValue(e, &DRW_Interface::addShape);
          output.appendValue(makeRawEntity(oType, nullptr,
                                           e.hasDataStorageBinaryData(),
                                           DRW::NoHandle, &e),
                             &DRW_Interface::addUnsupportedObject);
        }
        break;
      }
      case dwgType::VIEWPORT: {
        DRW_Viewport e;
        bool hasDataStorage = false;
        if (entryParse(e, buff, bs, ret)) {
          output.appendValue(e, &DRW_Interface::addViewport);
          hasDataStorage = e.hasDataStorageBinaryData();
          // Preserve the validated frame for same-version replay.
          output.appendValue(
              makeRawEntity(oType, nullptr, hasDataStorage, DRW::NoHandle, &e),
              &DRW_Interface::addUnsupportedObject);
        }
        break;
      }
      case dwgType::ELLIPSE: {
        DRW_Ellipse e;
        if (entryParse(e, buff, bs, ret)) {
          emitWithExtrusion(e, output, &DRW_Interface::addEllipse);
        }
        break;
      }
      case dwgType::SPLINE: {
        DRW_Spline e;
        if (entryParse(e, buff, bs, ret)) {
          output.appendValue(e, &DRW_Interface::addSpline);
        }
        break;
      }
      case dwgType::REGION: {
        DRW_ModelerGeometry e(DRW::REGION);
        if (entryParse(e, buff, bs, ret)) {
          linkDataStorage(e);
          e.m_objectSize = static_cast<std::uint32_t>(size);
          e.m_rawBytes = tmpByteStr;
          output.appendValue(e, &DRW_Interface::addModelerGeometry);
          output.appendValue(makeRawEntity(oType, nullptr,
                                           e.hasDataStorageBinaryData(),
                                           DRW::NoHandle, &e),
                             &DRW_Interface::addUnsupportedObject);
        }
        break;
      }
      case dwgType::SOLID3D: {
        DRW_ModelerGeometry e(DRW::E3DSOLID);
        if (entryParse(e, buff, bs, ret)) {
          linkDataStorage(e);
          e.m_objectSize = static_cast<std::uint32_t>(size);
          e.m_rawBytes = tmpByteStr;
          output.appendValue(e, &DRW_Interface::addModelerGeometry);
          output.appendValue(makeRawEntity(oType, nullptr,
                                           e.hasDataStorageBinaryData(),
                                           DRW::NoHandle, &e),
                             &DRW_Interface::addUnsupportedObject);
        }
        break;
      }
      case dwgType::BODY: {
        DRW_ModelerGeometry e(DRW::BODY);
        if (entryParse(e, buff, bs, ret)) {
          linkDataStorage(e);
          e.m_objectSize = static_cast<std::uint32_t>(size);
          e.m_rawBytes = tmpByteStr;
          output.appendValue(e, &DRW_Interface::addModelerGeometry);
          output.appendValue(makeRawEntity(oType, nullptr,
                                           e.hasDataStorageBinaryData(),
                                           DRW::NoHandle, &e),
                             &DRW_Interface::addUnsupportedObject);
        }
        break;
      }
      case dwgType::RAY: {
        DRW_Ray e;
        if (entryParse(e, buff, bs, ret)) {
          output.appendValue(e, &DRW_Interface::addRay);
        }
        break;
      }
      case dwgType::XLINE: {
        DRW_Xline e;
        if (entryParse(e, buff, bs, ret)) {
          output.appendValue(e, &DRW_Interface::addXline);
        }
        break;
      }
      case dwgType::MTEXT: {
        DRW_MText e;
        if (entryParse(e, buff, bs, ret)) {
          e.style = findTableName(DRW::STYLE, e.styleH.ref);
          output.appendValue(e, &DRW_Interface::addMText);
        }
        break;
      }
      case dwgType::LEADER: {
        DRW_Leader e;
        if (entryParse(e, buff, bs, ret)) {
          e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
          output.appendValue(e, &DRW_Interface::addLeader);
        }
        break;
      }
      case dwgType::TOLERANCE: {
        DRW_Tolerance e;
        if (entryParse(e, buff, bs, ret)) {
          output.appendValue(e, &DRW_Interface::addTolerance);
        }
        break;
      }
      case dwgType::MLINE: {
        DRW_MLine e;
        if (entryParse(e, buff, bs, ret)) {
          if (e.styleHandle != 0) {
            auto it = mlineStyleNameMap.find(e.styleHandle);
            if (it != mlineStyleNameMap.end() && e.styleName.empty()) {
              e.styleName = it->second;
            }
          }
          output.appendValue(e, &DRW_Interface::addMLine);
        }
        break;
      }
      case dwgType::OLE2FRAME: {
        DRW_Ole2Frame e;
        if (entryParse(e, buff, bs, ret)) {
          e.m_objectSize = static_cast<std::uint32_t>(size);
          e.m_rawBytes = tmpByteStr;
          output.appendValue(e, &DRW_Interface::addOle2Frame);
          output.appendValue(makeRawEntity(oType, nullptr,
                                           e.hasDataStorageBinaryData(),
                                           DRW::NoHandle, &e),
                             &DRW_Interface::addUnsupportedObject);
        }
        break;
      }
      case dwgType::OLEFRAME: {
        DRW_OleFrame e;
        if (entryParse(e, buff, bs, ret)) {
          e.m_objectSize = static_cast<std::uint32_t>(size);
          e.m_rawBytes = tmpByteStr;
          output.appendValue(e, &DRW_Interface::addOleFrame);
          output.appendValue(makeRawEntity(oType, nullptr,
                                           e.hasDataStorageBinaryData(),
                                           DRW::NoHandle, &e),
                             &DRW_Interface::addUnsupportedObject);
        }
        break;
      }
      case dwgType::LWPOLYLINE: {
        DRW_LWPolyline e;
        if (entryParse(e, buff, bs, ret)) {
          emitWithExtrusion(e, output, &DRW_Interface::addLWPolyline);
        }
        break;
      }
      case dwgType::HATCH: {
        DRW_Hatch e;
        if (entryParse(e, buff, bs, ret)) {
          output.appendValue(e, &DRW_Interface::addHatch);
        }
        break;
      }
      case dwgType::IMAGE: {
        DRW_Image e;
        if (entryParse(e, buff, bs, ret)) {
          output.appendValue(e, &DRW_Interface::addImage);
        }
        break;
      }
      case dwgType::WIPEOUT: {
        DRW_Wipeout e;
        if (entryParse(e, buff, bs, ret)) {
          output.appendValue(e, &DRW_Interface::addWipeout);
        }
        break;
      }
      case dwgType::NAVISWORKSMODEL: {
        DRW_NavisworksModel e;
        if (entryParse(e, buff, bs, ret)) {
          output.appendValue(e, &DRW_Interface::addNavisworksModel);
        }
        break;
      }
      case dwgObjType::PROXY_ENTITY: {
        DRW_ProxyEntity e;
        if (entryParse(e, buff, bs, ret)) {
          ret = decodeProxyGraphics(e, 3u);
          if (ret) {
            output.appendValue(e, &DRW_Interface::addProxyEntity);
            DRW_UnsupportedObject raw =
                makeRawEntity(oType, nullptr, false, DRW::NoHandle, &e);
            raw.m_recordName = "ACAD_PROXY_ENTITY";
            raw.m_className = "AcDbProxyEntity";
            raw.m_hasDataStorage = e.hasDataStorageBinaryData();
            output.appendValue(raw, &DRW_Interface::addUnsupportedObject);
          }
        }
        break;
      }
      case dwgObjType::DBCOLOR:
        // Fixed OBJECT type 1004 is collected for the OBJECTS pass. It
        // is above the custom-class numeric range but is not an entity.
        if (!deferObject(obj))
          ret = false;
        break;

      default:
        if (oType >= 500) {
          // Custom-class object (typically AutoCAD Mechanical AcDbAm*,
          // AcDbAssoc*, or vendor proxy entity).  Rendering proxy
          // graphics requires an ODA spec §20.4.95 decoder, which is
          // out of scope here; emit a distinct token so diagnostic
          // tools can distinguish "missing dispatch case" from
          // "intentionally-skipped custom class".
          if (resolvedClass != nullptr &&
              (resolvedClass->recName == "GEOPOSITIONMARKER" ||
               resolvedClass->recName == "POSITIONMARKER" ||
               resolvedClass->className == "AcDbGeoPositionMarker")) {
            DRW_GeoPositionMarker e;
            if (entryParse(e, buff, bs, ret)) {
              output.appendValue(e, &DRW_Interface::addGeoPositionMarker);
              output.appendValue(makeRawEntity(oType, resolvedClass,
                                               e.hasDataStorageBinaryData(),
                                               DRW::NoHandle, &e),
                                 &DRW_Interface::addUnsupportedObject);
            }
            break;
          }
          auto cit = classesmap.find(oType);
          if (cit != classesmap.end() && cit->second &&
              (cit->second->recName == "ARC_DIMENSION" ||
               cit->second->className == "AcDbArcDimension")) {
            DRW_DimArc e;
            if (entryParse(e, buff, bs, ret)) {
              e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
              output.appendValue(e, &DRW_Interface::addDimArc);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              (cit->second->recName == "LARGE_RADIAL_DIMENSION" ||
               cit->second->className == "AcDbRadialDimensionLarge")) {
            // Jogged radius dimension (ODA §20.4.20). Delivered via the
            // existing addDimRadial callback (DRW_DimLargeRadial is-a
            // DRW_DimRadial); the jog point/angle ride along on the object.
            DRW_DimLargeRadial e;
            if (entryParse(e, buff, bs, ret)) {
              e.style = findTableName(DRW::DIMSTYLE, e.dimStyleH.ref);
              output.appendValue(e, &DRW_Interface::addDimRadial);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              cit->second->recName == "WIPEOUT") {
            DRW_Wipeout e;
            if (entryParse(e, buff, bs, ret)) {
              output.appendValue(e, &DRW_Interface::addWipeout);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              cit->second->recName == "POINTCLOUD") {
            DRW_PointCloud e;
            if (entryParse(e, buff, bs, ret)) {
              output.appendValue(e, &DRW_Interface::addPointCloud);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              cit->second->recName == "POINTCLOUDEX") {
            DRW_PointCloudEx e;
            if (entryParse(e, buff, bs, ret)) {
              output.appendValue(e, &DRW_Interface::addPointCloudEx);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              (cit->second->recName == "NAVISWORKSMODEL" ||
               cit->second->className == "AcDbNavisworksModel")) {
            DRW_NavisworksModel e;
            if (entryParse(e, buff, bs, ret)) {
              output.appendValue(e, &DRW_Interface::addNavisworksModel);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              cit->second->recName == "PLANESURFACE") {
            DRW_PlaneSurface e;
            if (entryParse(e, buff, bs, ret)) {
              linkDataStorage(e);
              output.appendValue(e, &DRW_Interface::addSurface);
              output.appendValue(makeRawEntity(oType, cit->second,
                                               e.hasDataStorageBinaryData(),
                                               DRW::NoHandle, &e),
                                 &DRW_Interface::addUnsupportedObject);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              cit->second->recName == "EXTRUDEDSURFACE") {
            DRW_ExtrudedSurface e;
            if (entryParse(e, buff, bs, ret)) {
              linkDataStorage(e);
              output.appendValue(e, &DRW_Interface::addSurface);
              output.appendValue(makeRawEntity(oType, cit->second,
                                               e.hasDataStorageBinaryData(),
                                               DRW::NoHandle, &e),
                                 &DRW_Interface::addUnsupportedObject);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              cit->second->recName == "REVOLVEDSURFACE") {
            DRW_RevolvedSurface e;
            if (entryParse(e, buff, bs, ret)) {
              linkDataStorage(e);
              output.appendValue(e, &DRW_Interface::addSurface);
              output.appendValue(makeRawEntity(oType, cit->second,
                                               e.hasDataStorageBinaryData(),
                                               DRW::NoHandle, &e),
                                 &DRW_Interface::addUnsupportedObject);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              cit->second->recName == "SWEPTSURFACE") {
            DRW_SweptSurface e;
            if (entryParse(e, buff, bs, ret)) {
              linkDataStorage(e);
              output.appendValue(e, &DRW_Interface::addSurface);
              output.appendValue(makeRawEntity(oType, cit->second,
                                               e.hasDataStorageBinaryData(),
                                               DRW::NoHandle, &e),
                                 &DRW_Interface::addUnsupportedObject);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              cit->second->recName == "LOFTEDSURFACE") {
            DRW_LoftedSurface e;
            if (entryParse(e, buff, bs, ret)) {
              linkDataStorage(e);
              output.appendValue(e, &DRW_Interface::addSurface);
              output.appendValue(makeRawEntity(oType, cit->second,
                                               e.hasDataStorageBinaryData(),
                                               DRW::NoHandle, &e),
                                 &DRW_Interface::addUnsupportedObject);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              cit->second->recName == "NURBSURFACE") {
            DRW_NurbsSurface e;
            if (entryParse(e, buff, bs, ret)) {
              linkDataStorage(e);
              output.appendValue(e, &DRW_Interface::addSurface);
              output.appendValue(makeRawEntity(oType, cit->second,
                                               e.hasDataStorageBinaryData(),
                                               DRW::NoHandle, &e),
                                 &DRW_Interface::addUnsupportedObject);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              cit->second->recName == "MULTILEADER") {
            // MULTILEADER (AcDbMLeader, ODA spec §20.4.48).
            // DRW_MLeader::parseDwg fully decodes the entity: the
            // embedded MLeaderAnnotContext (roots, leader lines, text/
            // block content), the entity-level fields, and the handle
            // stream.  The DXF read path (dxfRW::processMultiLeader)
            // decodes the same nested CONTEXT_DATA{} block via
            // DRW_MLeader::parseDxfContextCode (drw_entities.cpp).
            DRW_MLeader e;
            if (entryParse(e, buff, bs, ret)) {
              output.appendValue(e, &DRW_Interface::addMLeader);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              (cit->second->recName == "MPOLYGON" ||
               cit->second->className == "AcDbMPolygon")) {
            // AcDbMPolygon (hatch-derived filled polygon). addMPolygon
            // defaults to addHatch, so it renders as a filled hatch.
            DRW_MPolygon e;
            if (entryParse(e, buff, bs, ret)) {
              output.appendValue(e, &DRW_Interface::addMPolygon);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              (cit->second->recName == "RTEXT" ||
               cit->second->className == "RText" ||
               cit->second->className == "AcDbRText")) {
            // RTEXT (AutoCAD Express Tools reactive text, ODA type 1159).
            // Mapped onto DRW_Text and delivered via addText — the
            // literal text if present, else the raw DIESEL/xref string.
            DRW_RText e;
            if (entryParse(e, buff, bs, ret)) {
              e.style = findTableName(DRW::STYLE, e.styleH.ref);
              output.appendValue(e, &DRW_Interface::addText);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              (cit->second->recName == "ARCALIGNEDTEXT" ||
               cit->second->recName == "ARC_ALIGNED_TEXT" ||
               cit->second->className == "AcDbArcAlignedText")) {
            // ARCALIGNEDTEXT (Express Tools arc-aligned text, ODA type
            // 1163).  Mapped onto DRW_Text as a 2D approximation placed
            // at the arc mid-point (see DRW_ArcAlignedText).  The style
            // is a name string in the DWG body, so it is NOT resolved
            // from a handle here.
            DRW_ArcAlignedText e;
            if (entryParse(e, buff, bs, ret)) {
              output.appendValue(e, &DRW_Interface::addText);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              (cit->second->recName == "CAMERA" ||
               cit->second->className == "AcDbCamera")) {
            DRW_Camera e;
            if (entryParse(e, buff, bs, ret)) {
              output.appendValue(e, &DRW_Interface::addCamera);
              output.appendValue(
                  makeRawEntity(oType, cit->second, false, DRW::NoHandle, &e),
                  &DRW_Interface::addUnsupportedObject);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              (cit->second->recName == "GEOPOSITIONMARKER" ||
               cit->second->recName == "POSITIONMARKER" ||
               cit->second->className == "AcDbGeoPositionMarker")) {
            DRW_GeoPositionMarker e;
            if (entryParse(e, buff, bs, ret)) {
              output.appendValue(e, &DRW_Interface::addGeoPositionMarker);
              output.appendValue(makeRawEntity(oType, cit->second,
                                               e.hasDataStorageBinaryData(),
                                               DRW::NoHandle, &e),
                                 &DRW_Interface::addUnsupportedObject);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second &&
              (cit->second->recName == "ACAD_TABLE" ||
               cit->second->className == "AcDbTable")) {
            DRW_Table e;
            if (entryParse(e, buff, bs, ret)) {
              e.name = findTableName(DRW::BLOCK_RECORD, e.blockRecH.ref);
              output.appendValue(e, &DRW_Interface::addTable);
            }
            break;
          }
          if (cit != classesmap.end() && cit->second) {
            const std::string &rn = cit->second->recName;
            const std::string &cn = cit->second->className;
            if (rn == "HELIX" || cn == "AcDbHelix") {
              DRW_Helix e;
              if (entryParse(e, buff, bs, ret)) {
                output.appendValue(e, &DRW_Interface::addHelix);
              }
              break;
            }
            if (rn == "MESH" || cn == "AcDbSubDMesh") {
              DRW_Mesh e;
              if (entryParse(e, buff, bs, ret)) {
                output.appendValue(e, &DRW_Interface::addMesh);
                // LibreCAD renders MESH as a 2D fallback and therefore does
                // not retain the typed payload. Keep the validated source
                // frame as a raw peer so OBJECTS references such as
                // SORTENTSTABLE can still target the entity on replay.
                output.appendValue(
                    makeRawEntity(oType, cit->second, false, DRW::NoHandle, &e),
                    &DRW_Interface::addUnsupportedObject);
              }
              break;
            }
            if (rn == "LIGHT" || cn == "AcDbLight") {
              DRW_Light e;
              if (entryParse(e, buff, bs, ret)) {
                output.appendValue(e, &DRW_Interface::addLight);
                output.appendValue(
                    makeRawEntity(oType, cit->second, false, DRW::NoHandle, &e),
                    &DRW_Interface::addUnsupportedObject);
              }
              break;
            }
            if (rn == "SECTIONOBJECT" || cn == "AcDbSection") {
              // SECTIONOBJECT (AcDbSection) live-section plane — typed
              // decode restores the section geometry + metadata + the
              // section_settings reference for dwgTs parity. Keep the
              // raw shelf only when the frame passes entryParse.
              DRW_SectionObject e;
              if (entryParse(e, buff, bs, ret)) {
                output.appendValue(e, &DRW_Interface::addSectionObject);
                output.appendValue(
                    makeRawEntity(oType, cit->second, false, DRW::NoHandle, &e),
                    &DRW_Interface::addUnsupportedObject);
              }
              break;
            }
            // NOTE: the AcDbSurface family (SURFACE / EXTRUDED / REVOLVED /
            // LOFTED / SWEPT / PLANE / NURB) is dispatched by the typed
            // DRW_Surface arms above (intfa.addSurface); a bare AcDbSurface
            // with no concrete subtype falls through to the generic
            // custom-class handler (raw round-trip + proxy-graphics decode).
            if (rn == "PDFUNDERLAY" || rn == "DGNUNDERLAY" ||
                rn == "DWFUNDERLAY" || cn == "AcDbPdfReference" ||
                cn == "AcDbDgnReference" || cn == "AcDbDwfReference") {
              DRW_Underlay e;
              if (rn == "DGNUNDERLAY" || cn == "AcDbDgnReference")
                e.kind = DRW_Underlay::DGN;
              else if (rn == "DWFUNDERLAY" || cn == "AcDbDwfReference")
                e.kind = DRW_Underlay::DWF;
              // else default PDF
              if (entryParse(e, buff, bs, ret)) {
                output.appendValue(e, &DRW_Interface::addUnderlay);
              }
              break;
            }
          }
          if (cit != classesmap.end() && cit->second &&
              cit->second->entityFlag == 0) {
            if (deferObject(obj)) {
              DRW_DBG("[entity-pass-defer-custom-object ");
              DRW_DBG(oType);
              DRW_DBG(" ");
              DRW_DBG(cit->second->recName.c_str());
              DRW_DBG("]\n");
            }
            break;
          }
          const char *className = (cit != classesmap.end() && cit->second)
                                      ? cit->second->recName.c_str()
                                      : "(unknown)";
          const DRW_Class *customClass =
              (cit != classesmap.end() && cit->second) ? cit->second : nullptr;
          // The post-block sweep is only a recovery pass for records
          // that were not claimed by a BLOCK_RECORD. A known entity
          // class without a validated block walk is therefore
          // malformed, even when its common header happens to parse.
          // Model/paper-space walks and direct block reads have already
          // set the owner context and are intentionally unaffected.
          if (rejectOwnedEntityInSweep && version > DRW::AC1015 &&
              customClass != nullptr && !ownerlessSpaceWalk) {
            parsedEntityOwnerMismatch = true;
            ret = false;
            break;
          }
          // R2007+ exposes the entity handle stream at objSize. Do not
          // raw-publish an opaque custom entity, or decode its proxy
          // graphics, until the common entity header and detached tail
          // both parse on an isolated cursor. R2000/R2004 opaque custom
          // bodies have no generic safe handle-stream boundary.
          if (version > DRW::AC1018 && customClass != nullptr) {
            RawEntityShell shell;
            dwgBuffer validationBuffer = buff.forkIndependent();
            ret = shell.parseDwg(version, &validationBuffer, bs) &&
                  validationBuffer.isGood();
            if (ret && shell.handle != obj.handle) {
              parsedEntityHandleMismatch = true;
              ret = false;
            }
            if (!ret)
              break;

            DRW_UnsupportedObject raw = makeRawEntity(
                oType, customClass, shell.hasDataStorageBinaryData(),
                shell.parentHandle, &shell, false);
            // Recover proxy graphics only after structural framing is
            // valid; a truncated tail must not produce child callbacks.
            ProxyHostEntity host;
            dwgBuffer proxyBuffer = buff.forkIndependent();
            if (host.parseDwg(version, &proxyBuffer, bs) &&
                proxyBuffer.isGood()) {
              host.parentHandle = shell.parentHandle;
              if (!decodeProxyGraphics(host, 2u)) {
                ret = false;
                break;
              }
            }
            output.appendValue(raw, &DRW_Interface::addUnsupportedObject);
            DRW_DBG("[custom-class-skipped ");
            DRW_DBG(oType);
            DRW_DBG(" ");
            DRW_DBG(className);
            DRW_DBG("]\n");
            ++m_skippedCustomClasses[className];
            break;
          }
          DRW_UnsupportedObject raw;
          // The raw carrier still needs the common identity fields
          // checked.  This is the only handle available before an
          // unknown AC1018 body reaches its unbounded tail.
          ProxyHostEntity identityHost;
          dwgBuffer identityBuffer = buff.forkIndependent();
          if (!identityHost.parseDwg(version, &identityBuffer, bs) ||
              !identityBuffer.isGood() || identityHost.handle != obj.handle) {
            parsedEntityHandleMismatch = true;
            recordObjectFrameFailure(obj, offsetSpace);
            if (frameFailure)
              *frameFailure = true;
            ret = false;
            break;
          }
          const std::uint32_t rawOwner =
              expectedBlockEntityOwner != DRW::NoHandle
                  ? expectedBlockEntityOwner
                  : identityHost.parentHandle;
          raw = makeRawEntity(oType, customClass, false, rawOwner, nullptr,
                              false);
          if (cit != classesmap.end() && cit->second) {
            raw.m_recordName = cit->second->recName;
            raw.m_className = cit->second->className;
          }
          // Recover cached PROXY GRAPHICS before raw-netting: this class is
          // unmodelled, but it may carry a self-contained primitive stream
          // (STDPART2D, AEC_WALL/WINDOW/DOOR, …) that any reader can render.
          // makeRawEntity never parses, so proxyGraphics is empty here; run
          // the class-agnostic common prologue on a throwaway host purely to
          // lift the graphData bytes (buff is unconsumed at this fall-through
          // — every typed arm above breaks), then decode them into render
          // primitives.  The raw object is STILL emitted below for lossless
          // round-trip; decoding only adds extra renderable geometry.
          {
            ProxyHostEntity host;
            dwgBuffer proxyBuffer = buff.forkIndependent();
            if (host.parseDwg(version, &proxyBuffer, bs) &&
                proxyBuffer.isGood()) {
              host.parentHandle = rawOwner;
              if (!decodeProxyGraphics(host, 2u)) {
                ret = false;
                break;
              }
            }
          }
          output.appendValue(raw, &DRW_Interface::addUnsupportedObject);
          DRW_DBG("[custom-class-skipped ");
          DRW_DBG(oType);
          DRW_DBG(" ");
          DRW_DBG(className);
          DRW_DBG("]\n");
          ++m_skippedCustomClasses[className];
        } else {
          if (!deferObject(obj))
            break;
          // Fixed oType (<500) not handled by the entity-pass switch
          // but queued in objObjectMap for the OBJECTS pass; the
          // OBJECTS switch dispatches case 42 DICTIONARY, 73 MLINESTYLE,
          // 82 LAYOUT, 102 IMAGEDEF etc.  Older code logged this as
          // "unhandled" which was misleading.
          DRW_DBG("[entity-pass-defer ");
          DRW_DBG(oType);
          DRW_DBG("]\n");
        }
        break;
      }
    if (!ret && !compoundFrameHandled) {
      // A frame only establishes byte bounds. A known typed record whose
      // body parser fails is malformed for that record and must not escape
      // through the raw callback as if it were a validated opaque shell.
      // Explicit fixed/custom shell routes publish raw data only after their
      // own complete parser succeeds.
      DRW_DBG("Warning: Entity type ");
      DRW_DBG(oType);
      DRW_DBG("has failed, handle: ");
      DRW_DBG(obj.handle);
      DRW_DBG("\n");
      markDwgFrameOutcome(sourceFrameId(obj), DRW_DwgFrameDisposition::Failed);
    }

    if (ret && !framePublicationPublished && !compoundFrameHandled &&
        (framePublication.typedViewParsed || framePublication.rawViewIssued)) {
      framePublication.publication.m_carrier =
          framePublication.rawViewIssued
              ? (framePublication.rawViewHasTypedPeer
                     ? DRW_DwgFramePublication::Carrier::TypedAndRaw
                     : DRW_DwgFramePublication::Carrier::Raw)
              : DRW_DwgFramePublication::Carrier::Typed;
      if (!output.appendFramePublication(*this, framePublication.publication))
        ret = false;
    }
    if (!ret) {
      DwgEntityFailurePhase phase = m_currentEntityFailurePhase;
      if (parsedEntityHandleMismatch || parsedEntityOwnerMismatch)
        phase = DwgEntityFailurePhase::Identity;
      else if (phase == DwgEntityFailurePhase::None)
        phase = compoundFrameHandled ? DwgEntityFailurePhase::Aggregate
                                     : DwgEntityFailurePhase::TypedBody;
      recordEntityFailure(obj, oType, phase);
    }
    m_activeEntityFrameCapture = nullptr;
    return ret;
  } catch (...) {
    m_activeEntityFrameCapture = nullptr;
    nextEntLink = prevEntLink = 0;
    nextEntLinkImplicit = false;
    if (m_dwgFrameCoverageStatus != DRW_DwgFrameCoverageStatus::NotAvailable) {
      (void)markDwgFrameOutcome(sourceFrameId(obj),
                                DRW_DwgFrameDisposition::Failed,
                                DRW_DwgFrameCoverageReason::CallbackException);
    }
    if (frameFailure)
      *frameFailure = true;
    recordEntityFailure(obj,
                        obj.type > std::numeric_limits<std::int16_t>::max()
                            ? -1
                            : static_cast<std::int16_t>(obj.type),
                        m_currentEntityFailurePhase ==
                                DwgEntityFailurePhase::None
                            ? DwgEntityFailurePhase::TypedBody
                            : m_currentEntityFailurePhase);
    return false;
  }
}

bool dwgReader::readDwgObjects(DRW_Interface &intfa, dwgBuffer *dbuf,
                               DwgIntegrityAddressSpace offsetSpace) {
  std::uint32_t i = 0;
  bool structuralFailure = false;
  DRW_DBG("\nentities map total size= ");
  DRW_DBG(ObjectMap.size());
  DRW_DBG("\nobjects map total size= ");
  DRW_DBG(objObjectMap.size());
  std::vector<DRW_UnsupportedObject> deferredRawObjects;
  deferredRawObjects.swap(m_deferredRawObjects);
  // Per-object parseDwg failures are warnings, not section failures —
  // each object is read from its own ObjectMap location, so one bad
  // record cannot corrupt the next. Mirrors readDwgEntities resilience.
  size_t failures = 0;
  while (!objObjectMap.empty()) {
    auto itB = objObjectMap.begin();
    if (m_quarantinedEntityHandles.find(itB->first) !=
        m_quarantinedEntityHandles.end()) {
      if (!discardDwgSourceFrame(objObjectMap, itB)) {
        structuralFailure = true;
        break;
      }
      continue;
    }
    DwgFrameMapLease lease;
    if (!detachDwgSourceFrame(objObjectMap, itB, lease)) {
      structuralFailure = true;
      break;
    }
    bool frameFailure = false;
    bool read = true;
    if (lease.classification.has_value()) {
      DwgFrameClassification observed;
      read = classifyDwgSourceFrame(dbuf, lease.object, observed) &&
             classificationsMatch(*lease.classification, observed);
      if (!read) {
        recordObjectFrameFailure(lease.object, offsetSpace);
        frameFailure = true;
      }
    }
    if (read &&
        !readDwgObject(dbuf, lease.object, intfa, &frameFailure, offsetSpace)) {
      read = false;
    }
    if (!read) {
      ++failures;
      if (lease.hasCoverage &&
          !markDwgFrameOutcome(lease.source, DRW_DwgFrameDisposition::Failed)) {
        structuralFailure = true;
      }
    }
    if (lease.hasCoverage && lease.classification.has_value()) {
      const auto sourceIt = m_dwgSourceFrameIndexes.find(lease.source.handle);
      if (sourceIt == m_dwgSourceFrameIndexes.end() ||
          sourceIt->second >= m_dwgSourceFrameLedger.size()) {
        structuralFailure = true;
      } else {
        DRW_DwgFrameDisposition disposition =
            m_dwgSourceFrameLedger[sourceIt->second].m_disposition;
        DwgFramePhaseSnapshot::Destination destination;
        bool terminalDisposition = true;
        switch (disposition) {
        case DRW_DwgFrameDisposition::Published:
          destination = DwgFramePhaseSnapshot::Destination::Published;
          break;
        case DRW_DwgFrameDisposition::Failed:
          destination = DwgFramePhaseSnapshot::Destination::Failed;
          break;
        case DRW_DwgFrameDisposition::Quarantined:
          destination = DwgFramePhaseSnapshot::Destination::Quarantined;
          break;
        case DRW_DwgFrameDisposition::Unresolved:
          destination = DwgFramePhaseSnapshot::Destination::Unresolved;
          break;
        default:
          terminalDisposition = markDwgFrameOutcome(
              lease.source, DRW_DwgFrameDisposition::Failed);
          structuralFailure = structuralFailure || !terminalDisposition;
          if (terminalDisposition) {
            disposition = DRW_DwgFrameDisposition::Failed;
            destination = DwgFramePhaseSnapshot::Destination::Failed;
          }
          break;
        }
        if (terminalDisposition) {
          recordDwgFramePhaseSnapshot(lease, destination, disposition);
        }
      }
    }
    if (!discardDetachedDwgSourceFrame(lease)) {
      structuralFailure = true;
    }
    structuralFailure = structuralFailure || frameFailure;
  }
  if (failures > 0) {
    DRW_DBG("readDwgObjects: ");
    DRW_DBG(failures);
    DRW_DBG(" objects failed to parse (warnings, not section failure)\n");
    m_objectParseFailures += failures;
  }
  if (DRW_DBGGL == DRW_dbg::Level::Debug) {
    for (auto it = remainingMap.begin(); it != remainingMap.end(); ++it) {
      DRW_DBG("\nnum.# ");
      DRW_DBG(i++);
      DRW_DBG(" Remaining object Handle, loc, type= ");
      DRW_DBG(it->first);
      DRW_DBG(" ");
      DRW_DBG(it->second.loc);
      DRW_DBG(" ");
      DRW_DBG(it->second.type);
    }
    DRW_DBG("\n");
  }
  finalizeDataStorageLinks();
  if (!structuralFailure) {
    structuralFailure = !publishDeferredRawObjects(intfa, deferredRawObjects);
  }
  // A bounded object body that a typed parser cannot decode is retained as
  // a warning, but a missing/truncated object frame invalidates the OBJECTS
  // section. The frame reader is the only authority for this distinction.
  return !structuralFailure;
}

bool dwgReader::publishDeferredRawObjects(
    DRW_Interface &intfa, std::vector<DRW_UnsupportedObject> &objects) {
  for (const DRW_UnsupportedObject &raw : objects) {
    const bool hasFrameCoverage =
        m_dwgFrameCoverageStatus != DRW_DwgFrameCoverageStatus::NotAvailable;
    DwgSourceFrameId source;
    if (hasFrameCoverage) {
      source = sourceFrameIdForHandle(raw.m_handle);
      if (m_dwgSourceFrameIndexes.find(raw.m_handle) ==
          m_dwgSourceFrameIndexes.end()) {
        return reportDwgFrameTransitionFailure(source);
      }
    }
    try {
      intfa.addUnsupportedObject(raw);
    } catch (...) {
      if (hasFrameCoverage) {
        (void)markDwgFrameOutcome(
            source, DRW_DwgFrameDisposition::Failed,
            DRW_DwgFrameCoverageReason::CallbackException);
      }
      return false;
    }
    if (hasFrameCoverage) {
      DRW_DwgFramePublication publication;
      publication.m_version = raw.m_version;
      publication.m_handle = raw.m_handle;
      publication.m_sourceOffset = source.offset;
      publication.m_sourceMapOrdinal = source.ordinal;
      publication.m_sourceOffsetSpace = source.offsetSpace;
      publication.m_hasSourceLocation = true;
      publication.m_encodedType = raw.m_objectType;
      publication.m_resolvedType = raw.m_objectType;
      publication.m_isEntity = raw.m_isEntity;
      publication.m_isCustomClass = raw.m_isCustomClass;
      publication.m_recordName = raw.m_recordName;
      publication.m_className = raw.m_className;
      publication.setCommonLinkEvidence(raw.m_commonLinkEvidence);
      publication.m_parentHandle = raw.m_parentHandle;
      publication.m_reactorHandles = raw.m_reactorHandles;
      publication.m_xDictHandle = raw.m_xDictHandle;
      publication.m_numReactors = raw.m_numReactors;
      publication.m_xDictFlag = raw.m_xDictFlag;
      publication.m_carrier = DRW_DwgFramePublication::Carrier::Raw;
      if (!publishDwgFramePublication(intfa, std::move(publication)))
        return false;
    }
  }
  objects.clear();
  return true;
}

/**
 * Reads a dwg drawing object (dwg object object) given its offset in the file
 */
bool dwgReader::readDwgObject(dwgBuffer *dbuf, objHandle &obj,
                              DRW_Interface &intfa, bool *frameFailure,
                              DwgIntegrityAddressSpace offsetSpace) {
  bool ret = true;
  const auto failStructural = [frameFailure]() {
    if (frameFailure)
      *frameFailure = true;
    return false;
  };
  if (frameFailure)
    *frameFailure = false;

  if (dbuf == nullptr) {
    recordObjectFrameFailure(obj, offsetSpace);
    return failStructural();
  }

  try {
    DwgObjectFrame frame;
    if (!frame.readAt(*dbuf, version, obj.loc)) {
      recordObjectFrameFailure(obj, offsetSpace);
      if (frameFailure)
        *frameFailure = true;
      DRW_DBG(" Warning: readDwgObject, invalid object frame\n");
      return false;
    }
    DwgFrameClassification classification;
    if (!classifyDwgSourceFrame(dbuf, obj, classification)) {
      recordObjectFrameFailure(obj, offsetSpace);
      DRW_DBG(" Warning: readDwgObject, missing object type\n");
      return failStructural();
    }
    if (classification.route == DwgFrameClassification::Route::BlockDelimiter) {
      recordObjectFrameFailure(obj, offsetSpace);
      DRW_DBG(" Warning: readDwgObject, misplaced block delimiter\n");
      return failStructural();
    }
    if (classification.route != DwgFrameClassification::Route::Object) {
      recordObjectFrameFailure(obj, offsetSpace);
      DRW_DBG(" Warning: readDwgObject, object frame routed as entity\n");
      return failStructural();
    }

    const std::uint32_t bs = frame.bodyBitSize();
    auto &tmpByteStr = frame.body();
    const std::size_t size = tmpByteStr.size();
    dwgBuffer buff(tmpByteStr.data(), size, &decoder);
    const std::int16_t encodedType = classification.encodedType;
    const std::int16_t oType = classification.resolvedType;
    const DRW_Class *resolvedClass = classification.resolvedClass;
    const bool fixedObjectShell = classification.fixedObjectShell;
    const bool rawCustomObjectShell =
        isValidatedRawCustomObjectShell(resolvedClass);
    const bool centerLineActionBody =
        isCenterLineActionBodyClass(resolvedClass);
    std::optional<DRW_DwgDictionaryMembership> dictionaryMembership;
    std::optional<DRW_DwgGroupMembership> groupMembership;
    std::optional<DRW_DwgSortEntsMembership> sortEntsMembership;
    std::optional<DRW_DwgFieldListMembership> fieldListMembership;
    std::optional<DRW_DwgFieldPayloadReceipt> fieldPayloadReceipt;
    std::optional<DRW_Field> fieldOutput;
    std::optional<DRW_FieldList> fieldListOutput;
    std::optional<DRW_UnsupportedObject> fieldRawOutput;
    std::optional<DRW_DwgDictionaryWithDefaultMembership>
        dictionaryWithDefaultMembership;
    std::optional<DRW_DwgTypedReference> typedReference;
    obj.type = static_cast<std::uint32_t>(oType);
    // Validate the shared OBJECTS prologue before dispatch. Every typed table
    // object consumes this same common handle before its class-specific body;
    // checking it on an independent cursor prevents a map entry from
    // publishing a valid-looking object under a different handle.
    dwgBuffer commonBuffer = buff.forkIndependent();
    dwgBuffer commonStringBuffer = buff.forkIndependent();
    DRW_Dictionary commonObject;
    const bool commonParsed = commonObject.DRW_TableEntry::parseDwg(
        version, &commonBuffer,
        version > DRW::AC1018 ? &commonStringBuffer : nullptr, bs);
    if (!commonParsed || !commonBuffer.isGood() ||
        (version > DRW::AC1018 && !commonStringBuffer.isGood()) ||
        commonObject.handle != obj.handle) {
      recordObjectFrameFailure(obj, offsetSpace);
      DRW_DBG(" Warning: readDwgObject, invalid common prologue or handle\n");
      return failStructural();
    }
    if (version > DRW::AC1018 &&
        !commonObject.DRW_TableEntry::parseDwgCommonHandleData(version,
                                                               &commonBuffer)) {
      recordObjectFrameFailure(obj, offsetSpace);
      DRW_DBG(" Warning: readDwgObject, invalid common handle tail\n");
      return failStructural();
    }
    DRW_DwgFramePublication publication;
    publication.m_version = version;
    publication.m_handle = obj.handle;
    publication.m_sourceOffset = obj.loc;
    publication.m_sourceMapOrdinal = obj.sourceOrdinal;
    publication.m_sourceOffsetSpace = obj.sourceOffsetSpace;
    publication.m_hasSourceLocation = true;
    publication.m_encodedType = encodedType;
    publication.m_resolvedType = oType;
    publication.m_isCustomClass = resolvedClass != nullptr;
    if (resolvedClass != nullptr) {
      publication.m_recordName = resolvedClass->recName;
      publication.m_className = resolvedClass->className;
    }
    publication.setCommonLinkEvidence(drwDwgCommonLinkEvidenceForLinks(
        commonObject.hasDwgCommonLinkTail(), commonObject.parentHandle,
        commonObject.reactorHandles, commonObject.reactorCount(),
        commonObject.xDictHandle));
    publication.m_parentHandle = commonObject.parentHandle;
    publication.m_reactorHandles = commonObject.reactorHandles;
    publication.m_xDictHandle = commonObject.xDictHandle;
    publication.m_numReactors = commonObject.reactorCount();
    publication.m_xDictFlag = commonObject.extensionDictionaryFlag();
    const auto failReceiptPreflight = [this, &obj]() {
      if (m_dwgFrameCoverageStatus !=
          DRW_DwgFrameCoverageStatus::NotAvailable) {
        (void)markDwgFrameOutcome(sourceFrameId(obj),
                                  DRW_DwgFrameDisposition::Failed,
                                  DRW_DwgFrameCoverageReason::ReceiptFailure);
      }
      return false;
    };
    bool rawViewIssued = false;
    bool rawViewHasTypedPeer = false;
    auto makeRawObject = [&](int rawType, const DRW_Class *cls = nullptr,
                             bool typedPeer = true) {
      rawViewIssued = true;
      rawViewHasTypedPeer = rawViewHasTypedPeer || typedPeer;
      DRW_UnsupportedObject raw;
      raw.m_version = version;
      raw.m_objectType = rawType;
      raw.m_handle = obj.handle;
      raw.m_parentHandle = DRW::NoHandle;
      raw.setCommonLinkEvidence(drwDwgCommonLinkEvidenceForLinks(
          commonObject.hasDwgCommonLinkTail(), commonObject.parentHandle,
          commonObject.reactorHandles, commonObject.reactorCount(),
          commonObject.xDictHandle));
      if (raw.m_commonLinkEvidence != DRW_DwgCommonLinkEvidence::Unknown) {
        raw.m_parentHandle = commonObject.parentHandle;
        raw.m_reactorHandles = commonObject.reactorHandles;
        raw.m_xDictHandle = commonObject.xDictHandle;
        raw.m_numReactors = commonObject.reactorCount();
        raw.m_xDictFlag = commonObject.extensionDictionaryFlag();
      }
      raw.m_bodyBitSize = bs;
      raw.m_objectOffset = obj.loc;
      raw.m_objectSize = static_cast<std::uint32_t>(size);
      raw.m_isEntity = false;
      raw.m_isCustomClass = cls != nullptr;
      if (cls != nullptr) {
        raw.m_hasClassDefinition = true;
        raw.m_classProxyFlag = static_cast<std::uint16_t>(cls->proxyFlag);
        raw.m_classAppName = cls->appName;
        raw.m_classWasProxy = cls->wasaProxyFlag != 0;
        raw.m_classEntityFlagRaw = cls->entityFlagRaw;
        raw.m_classDwgVersion = cls->dwgVersion;
        raw.m_classMaintenanceVersion = cls->maintenanceVersion;
        raw.m_classUnknown1 = cls->unknown1;
        raw.m_classUnknown2 = cls->unknown2;
        raw.m_recordName = cls->recName;
        raw.m_className = cls->className;
      }
      raw.m_hasDataStorage = commonObject.hasDataStorageBinaryData();
      raw.m_rawBytes = tmpByteStr;
      return raw;
    };
    // OBJECTS are parsed only after the APPID and layer tables are complete.
    // Resolve EED's deferred references before any typed callback observes the
    // record, matching the table-record and entity publication contracts.
    auto parseTableEntry = [this, &buff, bs](auto &entry) {
      const bool parsed = entry.parseDwg(version, &buff, bs) && buff.isGood();
      if (parsed)
        parseAttribs(&entry);
      return parsed;
    };

    if (fixedObjectShell) {
      RawObjectShell shell;
      ret = shell.parseDwg(version, &buff, bs) && buff.isGood();
      if (ret) {
        DRW_UnsupportedObject raw = makeRawObject(oType, nullptr, false);
        raw.m_recordName = DRW_UnsupportedObject::fixedObjectShellName(oType);
        intfa.addUnsupportedObject(raw);
      }
    } else if (rawCustomObjectShell) {
      RawObjectShell shell;
      ret = shell.parseDwg(version, &buff, bs) && buff.isGood();
      if (ret)
        intfa.addUnsupportedObject(makeRawObject(oType, resolvedClass, false));
    } else
      switch (oType) {
      case dwgObjType::DICTIONARY: {
        DRW_Dictionary e;
        ret = parseTableEntry(e);
        if (ret) {
          if (e.hasCompleteDwgEntries()) {
            DRW_DwgDictionaryMembership receipt;
            receipt.m_version = version;
            receipt.m_dictionaryHandle = obj.handle;
            receipt.m_sourceOffset = obj.loc;
            receipt.m_sourceMapOrdinal = obj.sourceOrdinal;
            receipt.m_sourceOffsetSpace = obj.sourceOffsetSpace;
            receipt.m_hasSourceLocation = true;
            receipt.m_complete = true;
            receipt.m_entries.reserve(e.m_entries.size());
            for (const DRW_Dictionary::Entry &entry : e.m_entries) {
              receipt.m_entries.push_back({entry.m_name, entry.m_handle});
            }
            dictionaryMembership = std::move(receipt);
          }
          intfa.addDictionary(e);
          intfa.addUnsupportedObject(makeRawObject(oType));
        }
        break;
      }
      case dwgObjType::GROUP: {
        DRW_Group e;
        ret = parseTableEntry(e);
        if (ret) {
          if (!e.hasCompleteDwgEntityHandles()) {
            ret = failReceiptPreflight();
            break;
          }
          DRW_DwgGroupMembership receipt;
          receipt.m_version = version;
          receipt.m_groupHandle = obj.handle;
          receipt.m_sourceOffset = obj.loc;
          receipt.m_sourceMapOrdinal = obj.sourceOrdinal;
          receipt.m_sourceOffsetSpace = obj.sourceOffsetSpace;
          receipt.m_hasSourceLocation = true;
          receipt.m_complete = true;
          receipt.m_entries.reserve(e.m_entityHandles.size());
          for (std::size_t index = 0; index < e.m_entityHandles.size();
               ++index) {
            receipt.m_entries.push_back(
                {e.m_entityHandles[index], static_cast<std::uint32_t>(index)});
          }
          groupMembership = std::move(receipt);
          normalizeDwgFramePublication(publication);
          if (m_dwgFrameCoverageStatus !=
                  DRW_DwgFrameCoverageStatus::NotAvailable &&
              !validateDwgFramePublicationStaticArtifacts(
                  publication,
                  {nullptr, nullptr, nullptr, &*groupMembership})) {
            ret = failReceiptPreflight();
            break;
          }
          intfa.addGroup(e);
          intfa.addUnsupportedObject(makeRawObject(oType));
        }
        break;
      }
      case dwgObjType::MLINESTYLE: {
        DRW_MLineStyle e;
        ret = parseTableEntry(e);
        if (ret) {
          mlineStyleNameMap[obj.handle] = e.name;
          intfa.addMLineStyle(e);
          intfa.addUnsupportedObject(makeRawObject(oType));
        }
        break;
      }
      case dwgObjType::XRECORD: {
        DRW_XRecord e;
        ret = parseTableEntry(e);
        if (ret) {
          intfa.addXRecord(e);
          intfa.addUnsupportedObject(makeRawObject(oType));
        }
        break;
      }
      case dwgObjType::ACDBPLACEHOLDER: {
        DRW_AcDbPlaceholder e;
        ret = parseTableEntry(e);
        if (ret) {
          intfa.addAcDbPlaceholder(e);
          intfa.addUnsupportedObject(makeRawObject(oType));
        }
        break;
      }
      case dwgObjType::VBA_PROJECT: {
        DRW_VbaProject e;
        ret = parseTableEntry(e);
        if (ret) {
          DRW_UnsupportedObject raw = makeRawObject(oType);
          raw.m_recordName = "VBA_PROJECT";
          raw.m_className = "AcDbVbaProject";
          intfa.addVbaProject(e);
          intfa.addUnsupportedObject(raw);
        }
        break;
      }
      case dwgObjType::PROXY_OBJECT: {
        DRW_ProxyObject e;
        // A failed proxy-object body is not a lossless raw carrier: its
        // metadata parser has already established that the declared
        // object layout is truncated or invalid.
        ret = parseTableEntry(e);
        if (ret) {
          DRW_UnsupportedObject raw = makeRawObject(oType);
          raw.m_recordName = "ACAD_PROXY_OBJECT";
          raw.m_className = "AcDbProxyObject";
          intfa.addProxyObject(e);
          intfa.addUnsupportedObject(raw);
        }
        break;
      }
      case dwgObjType::LAYOUT: {
        DRW_Layout e;
        ret = parseTableEntry(e);
        if (ret) {
          intfa.addLayout(e);
          intfa.addUnsupportedObject(makeRawObject(oType));
        }
        break;
      }
      case dwgObjType::IMAGEDEF: {
        DRW_ImageDef e;
        ret = parseTableEntry(e);
        if (ret) {
          intfa.linkImage(&e);
          intfa.addUnsupportedObject(makeRawObject(oType));
        }
        break;
      }
      case dwgObjType::DBCOLOR: {
        DRW_DbColor e;
        ret = parseTableEntry(e);
        if (ret) {
          std::string formatted =
              e.bookName.empty() ? e.name : (e.bookName + "$" + e.name);
          dbColorMap[obj.handle] = {e.rgb, formatted};
          intfa.addDbColor(e);
          intfa.addUnsupportedObject(makeRawObject(oType));
        }
        break;
      }
      case dwgObjType::VPORT_ENTITY_HEADER: {
        DRW_ViewportEntityHeader e;
        ret = parseTableEntry(e);
        if (ret) {
          intfa.addViewportEntityHeader(e);
          intfa.addUnsupportedObject(makeRawObject(oType));
        }
        break;
      }
      case dwgObjType::BLOCKREPRESENTATION: {
        DRW_BlockRepresentationData e;
        ret = parseTableEntry(e);
        if (ret) {
          intfa.addBlockRepresentationData(e);
          intfa.addUnsupportedObject(makeRawObject(oType));
        }
        break;
      }
      case dwgObjType::UNKNOWN_9:
      case dwgObjType::UNKNOWN_36:
      case dwgObjType::UNKNOWN_37:
      case dwgObjType::UNKNOWN_3A:
      case dwgObjType::UNKNOWN_3B:
      case dwgObjType::DUMMY:
      case dwgObjType::LONG_TRANSACTION: {
        // These fixed OBJECTS types are intentionally opaque in the
        // cross-reader oracles. The validated frame is the contract:
        // retain its exact bytes without guessing a version-specific
        // body layout or leaving it in the deferred/skipped maps.
        const char *recordName = nullptr;
        switch (oType) {
        case dwgObjType::UNKNOWN_9:
          recordName = "UNKNOWN_9";
          break;
        case dwgObjType::UNKNOWN_36:
          recordName = "UNKNOWN_36";
          break;
        case dwgObjType::UNKNOWN_37:
          recordName = "UNKNOWN_37";
          break;
        case dwgObjType::UNKNOWN_3A:
          recordName = "UNKNOWN_3A";
          break;
        case dwgObjType::UNKNOWN_3B:
          recordName = "UNKNOWN_3B";
          break;
        case dwgObjType::DUMMY:
          recordName = "DUMMY";
          break;
        case dwgObjType::LONG_TRANSACTION:
          recordName = "LONG_TRANSACTION";
          break;
        default:
          break;
        }
        DRW_UnsupportedObject raw = makeRawObject(oType, nullptr, false);
        raw.m_recordName = recordName;
        ret = buff.isGood();
        if (ret)
          intfa.addUnsupportedObject(raw);
        break;
      }
      default:
        // Custom-class objects (oType >= 500) — look up by classesmap
        // recName.  MLEADERSTYLE lives here (ODA spec §20.4.87) and
        // mirrors the WIPEOUT-from-entity dispatch pattern added in
        // commit a05908400 / 8e6730e5b.
        if (oType >= 500) {
          auto cit = classesmap.find(oType);
          if (cit != classesmap.end() && cit->second) {
            const std::string &rn = cit->second->recName;
            if (rn == "DBCOLOR" || rn == "ACDBCOLOR" ||
                cit->second->className == "AcDbColor") {
              DRW_DbColor e;
              ret = parseTableEntry(e);
              if (ret) {
                std::string formatted =
                    e.bookName.empty() ? e.name : (e.bookName + "$" + e.name);
                dbColorMap[obj.handle] = {e.rgb, formatted};
                intfa.addDbColor(e);
                intfa.addUnsupportedObject(
                    makeRawObject(oType, cit->second, false));
              }
              break;
            }
            if (rn == "VXCONTROL" || rn == "VX_CONTROL" ||
                cit->second->className == "AcDbVxControl") {
              DRW_VxControl e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addVxControl(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "VXTABLERECORD" || rn == "VX_TABLE_RECORD" ||
                cit->second->className == "AcDbVxTableRecord") {
              DRW_VxTableRecord e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addVxTableRecord(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "TVDEVICEPROPERTIES" ||
                cit->second->className == "AcDbTvDeviceProperties") {
              DRW_TvDeviceProperties e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addTvDeviceProperties(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "CSACDOCUMENTOPTIONS") {
              DRW_CsacDocumentOptions e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addCsacDocumentOptions(e);
                intfa.addUnsupportedObject(
                    makeRawObject(oType, cit->second, false));
              }
              break;
            }
            if (rn == "CONTEXTDATAMANAGER" ||
                cit->second->className == "AcDbContextDataManager") {
              DRW_ContextDataManager e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addContextDataManager(e);
                intfa.addUnsupportedObject(
                    makeRawObject(oType, cit->second, false));
              }
              break;
            }
            if (rn == "SUNSTUDY" || rn == "ACDBSUNSTUDY" ||
                cit->second->className == "AcDbSunStudy") {
              DRW_SunStudy e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addSunStudy(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "MOTIONPATH" || rn == "ACDBMOTIONPATH" ||
                cit->second->className == "AcDbMotionPath") {
              DRW_MotionPath e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addMotionPath(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "CURVEPATH" || rn == "ACDBCURVEPATH" ||
                cit->second->className == "AcDbCurvePath") {
              DRW_CurvePath e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addCurvePath(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "POINTPATH" || rn == "ACDBPOINTPATH" ||
                cit->second->className == "AcDbPointPath") {
              DRW_PointPath e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addPointPath(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "OBJECT_PTR" || rn == "OBJECTPTR" ||
                rn == "ACDBOBJECTPTR" ||
                cit->second->className == "AcDbObjectPtr") {
              DRW_ObjectPtr e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addObjectPtr(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "PARTIAL_VIEWING_INDEX" || rn == "PARTIALVIEWINGINDEX" ||
                rn == "ACDBPARTIALVIEWINGINDEX" ||
                cit->second->className == "AcDbPartialViewingIndex") {
              DRW_PartialViewingIndex e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addPartialViewingIndex(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "RENDERSETTINGS" || rn == "ACDBRENDERSETTINGS" ||
                cit->second->className == "AcDbRenderSettings" ||
                rn == "RAPIDRTRENDERSETTINGS" ||
                rn == "ACDBRAPIDRTRENDERSETTINGS" ||
                cit->second->className == "AcDbRapidRTRenderSettings" ||
                rn == "MENTALRAYRENDERSETTINGS" ||
                rn == "ACDBMENTALRAYRENDERSETTINGS" ||
                cit->second->className == "AcDbMentalRayRenderSettings" ||
                rn == "RENDERENTRY" ||
                cit->second->className == "AcDbRenderEntry" ||
                rn == "RENDERENVIRONMENT" ||
                cit->second->className == "AcDbRenderEnvironment" ||
                rn == "RENDERGLOBAL" ||
                cit->second->className == "AcDbRenderGlobal") {
              DRW_RenderSettings settings;
              if (rn == "RENDERSETTINGS" || rn == "ACDBRENDERSETTINGS" ||
                  cit->second->className == "AcDbRenderSettings")
                settings.m_kind = DRW_RenderSettings::Settings;
              else if (rn == "RAPIDRTRENDERSETTINGS" ||
                       rn == "ACDBRAPIDRTRENDERSETTINGS" ||
                       cit->second->className == "AcDbRapidRTRenderSettings")
                settings.m_kind = DRW_RenderSettings::RapidRT;
              else if (rn == "MENTALRAYRENDERSETTINGS" ||
                       rn == "ACDBMENTALRAYRENDERSETTINGS" ||
                       cit->second->className == "AcDbMentalRayRenderSettings")
                settings.m_kind = DRW_RenderSettings::MentalRay;
              else if (rn == "RENDERENTRY" ||
                       cit->second->className == "AcDbRenderEntry")
                settings.m_kind = DRW_RenderSettings::Entry;
              else if (rn == "RENDERENVIRONMENT" ||
                       cit->second->className == "AcDbRenderEnvironment")
                settings.m_kind = DRW_RenderSettings::Environment;
              else
                settings.m_kind = DRW_RenderSettings::Global;
              ret = parseTableEntry(settings);
              if (ret) {
                intfa.addRenderSettings(settings);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "SOLIDBACKGROUND" || rn == "SOLID_BACKGROUND" ||
                cit->second->className == "AcDbSolidBackground" ||
                rn == "GRADIENTBACKGROUND" || rn == "GRADIENT_BACKGROUND" ||
                cit->second->className == "AcDbGradientBackground" ||
                rn == "GROUNDPLANEBACKGROUND" ||
                rn == "GROUND_PLANE_BACKGROUND" ||
                cit->second->className == "AcDbGroundPlaneBackground" ||
                rn == "IMAGEBACKGROUND" || rn == "IMAGE_BACKGROUND" ||
                cit->second->className == "AcDbImageBackground" ||
                rn == "IBLBACKGROUND" || rn == "IBL_BACKGROUND" ||
                rn == "RAPIDRTRENDERENVIRONMENT" ||
                cit->second->className == "AcDbIBLBackground" ||
                rn == "SKYLIGHTBACKGROUND" || rn == "SKYLIGHT_BACKGROUND" ||
                cit->second->className == "AcDbSkyBackground") {
              DRW_Background e;
              if (rn == "GRADIENTBACKGROUND" || rn == "GRADIENT_BACKGROUND" ||
                  cit->second->className == "AcDbGradientBackground")
                e.m_kind = DRW_Background::Gradient;
              else if (rn == "GROUNDPLANEBACKGROUND" ||
                       rn == "GROUND_PLANE_BACKGROUND" ||
                       cit->second->className == "AcDbGroundPlaneBackground")
                e.m_kind = DRW_Background::GroundPlane;
              else if (rn == "IMAGEBACKGROUND" || rn == "IMAGE_BACKGROUND" ||
                       cit->second->className == "AcDbImageBackground")
                e.m_kind = DRW_Background::Image;
              else if (rn == "IBLBACKGROUND" || rn == "IBL_BACKGROUND" ||
                       rn == "RAPIDRTRENDERENVIRONMENT" ||
                       cit->second->className == "AcDbIBLBackground")
                e.m_kind = DRW_Background::Ibl;
              else if (rn == "SKYLIGHTBACKGROUND" ||
                       rn == "SKYLIGHT_BACKGROUND" ||
                       cit->second->className == "AcDbSkyBackground")
                e.m_kind = DRW_Background::Skylight;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addBackground(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "SECTION_MANAGER" || rn == "SECTIONMANAGER" ||
                cit->second->className == "AcDbSectionManager") {
              DRW_Section e;
              e.m_kind = DRW_Section::Manager;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addSection(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "SECTION_SETTINGS" || rn == "SECTIONSETTINGS" ||
                cit->second->className == "AcDbSectionSettings") {
              DRW_Section e;
              e.m_kind = DRW_Section::Settings;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addSection(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "DICTIONARYVAR" ||
                cit->second->className == "AcDbDictionaryVar") {
              DRW_DictionaryVar e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addDictionaryVar(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "ACDBDICTIONARYWDFLT" || rn == "DICTIONARYWDFLT" ||
                cit->second->className == "AcDbDictionaryWithDefault") {
              DRW_DictionaryWithDefault e;
              ret = parseTableEntry(e);
              if (ret) {
                if (!e.hasCompleteDwgEntries() ||
                    !e.isDwgPayloadValid(version)) {
                  ret = failReceiptPreflight();
                  break;
                }
                DRW_DwgTypedReference receipt;
                receipt.m_version = version;
                receipt.m_sourceHandle = obj.handle;
                receipt.m_sourceOffset = obj.loc;
                receipt.m_sourceMapOrdinal = obj.sourceOrdinal;
                receipt.m_sourceOffsetSpace = obj.sourceOffsetSpace;
                receipt.m_hasSourceLocation = true;
                receipt.m_complete = true;
                receipt.m_encodedType = encodedType;
                receipt.m_resolvedType = oType;
                const auto classOrdinal = m_dwgClassNumberOrdinals.find(
                    static_cast<std::uint32_t>(encodedType));
                if (classOrdinal != m_dwgClassNumberOrdinals.end()) {
                  receipt.m_classStreamOrdinal = classOrdinal->second;
                }
                receipt.m_field = DRW_DwgTypedReferenceField::DictionaryDefault;
                receipt.m_referenceCode = DRW::DwgHardPointer;
                receipt.m_targetHandle = e.m_defaultEntryHandle;
                typedReference = std::move(receipt);
                normalizeDwgFramePublication(publication);
                DRW_DwgDictionaryWithDefaultMembership membership;
                membership.m_version = publication.m_version;
                membership.m_dictionaryHandle = publication.m_handle;
                membership.m_sourceOffset = publication.m_sourceOffset;
                membership.m_sourceMapOrdinal = publication.m_sourceMapOrdinal;
                membership.m_sourceOffsetSpace =
                    publication.m_sourceOffsetSpace;
                membership.m_hasSourceLocation =
                    publication.m_hasSourceLocation;
                membership.m_complete = true;
                membership.m_encodedType = publication.m_encodedType;
                membership.m_resolvedType = publication.m_resolvedType;
                membership.m_recordName = publication.m_recordName;
                membership.m_className = publication.m_className;
                membership.m_classStreamOrdinal =
                    publication.m_classStreamOrdinal;
                membership.m_cloning = e.cloning;
                membership.m_hardOwner = e.hardOwner;
                membership.m_defaultEntryHandle = e.m_defaultEntryHandle;
                membership.m_entries.reserve(e.m_entries.size());
                for (const DRW_Dictionary::Entry &entry : e.m_entries) {
                  membership.m_entries.push_back(
                      {entry.m_name, entry.m_handle});
                }
                dictionaryWithDefaultMembership = std::move(membership);
                if (m_dwgFrameCoverageStatus !=
                        DRW_DwgFrameCoverageStatus::NotAvailable &&
                    !validateDwgFramePublicationStaticArtifacts(
                        publication,
                        {nullptr, &*typedReference, nullptr, nullptr, nullptr,
                         nullptr, &*dictionaryWithDefaultMembership})) {
                  ret = failReceiptPreflight();
                  break;
                }
                intfa.addDictionaryWithDefault(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "XRECORD" || cit->second->className == "AcDbXrecord") {
              DRW_XRecord e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addXRecord(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "FIELD" || cit->second->className == "AcDbField") {
              DRW_Field e;
              ret = parseTableEntry(e);
              if (ret) {
                if (e.hasCompleteDwgPayload()) {
                  normalizeDwgFramePublication(publication);
                  DRW_DwgFieldPayloadReceipt receipt;
                  receipt.m_version = version;
                  receipt.m_fieldHandle = obj.handle;
                  receipt.m_sourceOffset = obj.loc;
                  receipt.m_sourceMapOrdinal = obj.sourceOrdinal;
                  receipt.m_sourceOffsetSpace = obj.sourceOffsetSpace;
                  receipt.m_hasSourceLocation = true;
                  receipt.m_complete = true;
                  receipt.m_encodedType = encodedType;
                  receipt.m_resolvedType = oType;
                  receipt.m_recordName = publication.m_recordName;
                  receipt.m_className = publication.m_className;
                  receipt.m_classStreamOrdinal =
                      publication.m_classStreamOrdinal;
                  receipt.m_field = e;
                  fieldPayloadReceipt = std::move(receipt);
                  if (m_dwgFrameCoverageStatus !=
                          DRW_DwgFrameCoverageStatus::NotAvailable &&
                      !validateDwgFramePublicationStaticArtifacts(
                          publication,
                          {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                           nullptr, &*fieldPayloadReceipt})) {
                    ret = failReceiptPreflight();
                    break;
                  }
                  fieldOutput = std::move(e);
                  fieldRawOutput = makeRawObject(oType, cit->second);
                } else {
                  fieldRawOutput = makeRawObject(oType, cit->second, false);
                }
              }
              break;
            }
            if (rn == "FIELDLIST" ||
                cit->second->className == "AcDbFieldList") {
              DRW_FieldList e;
              ret = parseTableEntry(e);
              if (ret) {
                if (!e.hasCompleteDwgEntries()) {
                  ret = failReceiptPreflight();
                  break;
                }
                normalizeDwgFramePublication(publication);
                DRW_DwgFieldListMembership receipt;
                receipt.m_version = version;
                receipt.m_listHandle = obj.handle;
                receipt.m_sourceOffset = obj.loc;
                receipt.m_sourceMapOrdinal = obj.sourceOrdinal;
                receipt.m_sourceOffsetSpace = obj.sourceOffsetSpace;
                receipt.m_hasSourceLocation = true;
                receipt.m_complete = true;
                receipt.m_encodedType = encodedType;
                receipt.m_resolvedType = oType;
                receipt.m_recordName = publication.m_recordName;
                receipt.m_className = publication.m_className;
                receipt.m_classStreamOrdinal = publication.m_classStreamOrdinal;
                receipt.m_entries.reserve(e.m_fieldHandles.size());
                for (std::size_t index = 0; index < e.m_fieldHandles.size();
                     ++index) {
                  receipt.m_entries.push_back(
                      {e.m_fieldHandles[index],
                       static_cast<std::uint32_t>(index)});
                }
                fieldListMembership = std::move(receipt);
                if (m_dwgFrameCoverageStatus !=
                        DRW_DwgFrameCoverageStatus::NotAvailable &&
                    !validateDwgFramePublicationStaticArtifacts(
                        publication, {nullptr, nullptr, nullptr, nullptr,
                                      nullptr, &*fieldListMembership})) {
                  ret = failReceiptPreflight();
                  break;
                }
                fieldListOutput = std::move(e);
                fieldRawOutput = makeRawObject(oType, cit->second);
              }
              break;
            }
            if (rn == "DATATABLE" ||
                cit->second->className == "AcDbDataTable") {
              DRW_DataTable e;
              ret = parseTableEntry(e);
              // DATATABLE's body is a DEBUG_CLASS ("(varies)") whose
              // cell walk may drift; parseDwg graceful-degrades to the
              // decoded prefix, and the raw shelf preserves the exact
              // bytes so the round-trip stays faithful regardless.
              if (ret) {
                intfa.addDataTable(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "RASTERVARIABLES" ||
                cit->second->className == "AcDbRasterVariables") {
              DRW_RasterVariables e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addRasterVariables(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "WIPEOUTVARIABLES" ||
                cit->second->className == "AcDbWipeoutVariables") {
              DRW_WipeoutVariables e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addWipeoutVariables(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "SORTENTSTABLE" ||
                cit->second->className == "AcDbSortentsTable") {
              DRW_SortEntsTable e;
              ret = parseTableEntry(e);
              if (ret) {
                if (!e.hasCompleteDwgEntries()) {
                  ret = failReceiptPreflight();
                  break;
                }
                normalizeDwgFramePublication(publication);
                DRW_DwgSortEntsMembership receipt;
                receipt.m_version = version;
                receipt.m_tableHandle = obj.handle;
                receipt.m_sourceOffset = obj.loc;
                receipt.m_sourceMapOrdinal = obj.sourceOrdinal;
                receipt.m_sourceOffsetSpace = obj.sourceOffsetSpace;
                receipt.m_hasSourceLocation = true;
                receipt.m_complete = true;
                receipt.m_encodedType = encodedType;
                receipt.m_resolvedType = oType;
                receipt.m_recordName = publication.m_recordName;
                receipt.m_className = publication.m_className;
                receipt.m_classStreamOrdinal = publication.m_classStreamOrdinal;
                receipt.m_blockOwnerHandle = e.m_blockOwnerHandle;
                receipt.m_entries.reserve(e.m_entityHandles.size());
                for (std::size_t index = 0; index < e.m_entityHandles.size();
                     ++index) {
                  const std::uint32_t sort = e.m_sortHandles[index];
                  receipt.m_entries.push_back(
                      {e.m_entityHandles[index], sort,
                       static_cast<std::uint32_t>(index),
                       sort == DRW::NoHandle});
                }
                sortEntsMembership = std::move(receipt);
                if (m_dwgFrameCoverageStatus !=
                        DRW_DwgFrameCoverageStatus::NotAvailable &&
                    !validateDwgFramePublicationStaticArtifacts(
                        publication, {nullptr, nullptr, nullptr, nullptr,
                                      &*sortEntsMembership})) {
                  ret = failReceiptPreflight();
                  break;
                }
                intfa.addSortEntsTable(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "MATERIAL" || cit->second->className == "AcDbMaterial") {
              DRW_Material e;
              ret = parseTableEntry(e);
              // MATERIAL's parser is truncated (name + description
              // only); raw replay captures the full byte image so
              // the round-trip stays faithful regardless. (Phase 2b.1)
              if (ret) {
                intfa.addMaterial(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "TABLESTYLE" ||
                cit->second->className == "AcDbTableStyle") {
              DRW_TableStyle e;
              ret = parseTableEntry(e);
              // Raw replay preserves the full byte image; native
              // table writer (when active) claims the handle and
              // suppresses double-emit. (Phase 2b.2)
              if (ret) {
                intfa.addTableStyle(e);
                // AC1018 TABLESTYLE bodies are not fully typed by
                // this parser, but the validated frame remains
                // available through the raw representation.
                DRW_UnsupportedObject raw = makeRawObject(oType, cit->second);
                raw.m_typedPayloadValidated = true;
                intfa.addUnsupportedObject(raw);
              }
              break;
            }
            if (rn == "TABLECONTENT" ||
                cit->second->className == "AcDbTableContent") {
              DRW_TableContentObject e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addTableContent(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              } else if (version <= DRW::AC1018 && buff.isGood()) {
                // R2000/R2004 use a body layout that is retained
                // raw until a typed decoder is available.
                intfa.addUnsupportedObject(
                    makeRawObject(oType, cit->second, false));
                ret = true;
              }
              break;
            }
            if (rn == "CELLSTYLEMAP" ||
                cit->second->className == "AcDbCellStyleMap") {
              DRW_CellStyleMap e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addCellStyleMap(e);
                // Preserve the complete validated object frame
                // alongside the typed metadata.
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "DIMASSOC" || cit->second->className == "AcDbDimAssoc") {
              DRW_DimensionAssociation e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addDimensionAssociation(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              } else if (version <= DRW::AC1018 && buff.isGood()) {
                // DIMASSOC first appears in legacy files, while
                // this typed body starts at R2007. Keep the older
                // bounded frame raw until a legacy decoder exists.
                intfa.addUnsupportedObject(
                    makeRawObject(oType, cit->second, false));
                ret = true;
              }
              break;
            }
            if (rn == "ACAD_EVALUATION_GRAPH" ||
                cit->second->className == "AcDbEvalGraph") {
              DRW_EvaluationGraph e;
              ret = parseTableEntry(e);
              // parseDwg now decodes the typed body at every version
              // (R2000/R2004 handles inline, R2007+ separate stream).
              // Raw replay preserves the full byte image alongside
              // the validated typed record.
              if (ret) {
                intfa.addEvaluationGraph(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "SUN" || cit->second->className == "AcDbSun") {
              DRW_Sun e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addSun(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn.rfind("ACDBASSOC", 0) == 0 ||
                rn == "ACDBPERSSUBENTMANAGER" || centerLineActionBody) {
              // Every ACDBASSOC* associativity class (plus the bare
              // PERSSUBENTMANAGER) routes to the shell parser:
              // ACTION/NETWORK/DEPENDENCY/GEOMDEPENDENCY/PERSSUBENT
              // and the action-param variants decode structured
              // fields; all other subclasses (surface/array action
              // bodies, generic action params, value/variable deps,
              // 2d-constraint groups) run the shared prefix
              // (suffix-inferred in parseDwg) and are preserved
              // byte-for-byte by the raw shelf below.
              DRW_AssociativeObject e(rn);
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addAssociativeObject(e);
                // A validated object frame is still losslessly
                // readable when a class-specific suffix is outside
                // this typed decoder's coverage.
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              } else if (version <= DRW::AC1018 && buff.isGood()) {
                // Legacy associativity subclasses may have opaque
                // suffixes; preserve only a structurally readable
                // frame, never a failed modern parse.
                intfa.addUnsupportedObject(
                    makeRawObject(oType, cit->second, false));
                ret = true;
              }
              break;
            }
            if (rn.rfind("ACSH_", 0) == 0) {
              // Every ACSH_* solid-history class routes to the shell
              // parser and is delivered through addAcShHistoryObject.
              // Structured field decode covers HISTORY, SWEEP/EXTRUSION,
              // BOX/WEDGE/SPHERE/CYLINDER/CONE, and (dwgTs parity)
              // BOOLEAN/CHAMFER/FILLET/TORUS/REVOLVE/LOFT. BREP and the
              // remaining classes run the shared prefix (or nothing) and
              // are preserved byte-for-byte by the raw shelf below.
              DRW_AcShHistoryObject e(rn);
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addAcShHistoryObject(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (DRW_DynamicBlockObject::isDynamicBlockRecName(rn)) {
              // The dynamic-block object family (BLOCK*PARAMETER /
              // BLOCK*ACTION / BLOCK*GRIP / BLOCKGRIPLOCATIONCOMPONENT /
              // DYNAMICBLOCK* + singletons) — the largest custom-class
              // family.  Every recName routes to the shell parser: the
              // shared AcDbEvalExpr (+ AcDbBlockElement/BlockParameter)
              // prefix decodes typed, BLOCKVISIBILITYPARAMETER /
              // the verified BLOCK*ACTION subclasses decode fully,
              // and the rest are preserved byte-for-byte by the raw
              // shelf below.  parseDwg
              // graceful-degrades so a drift never drops the object.
              DRW_DynamicBlockObject e(rn);
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addDynamicBlockObject(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "ACDBDETAILVIEWSTYLE" || rn == "DETAILVIEWSTYLE" ||
                cit->second->className == "AcDbDetailViewStyle") {
              DRW_DetailViewStyle e;
              ret = parseTableEntry(e);
              // Raw replay preserves the full byte image (version
              // guard blocks cross-version replay). (Phase 2b.3)
              if (ret) {
                intfa.addDetailViewStyle(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "ACDBSECTIONVIEWSTYLE" || rn == "SECTIONVIEWSTYLE" ||
                cit->second->className == "AcDbSectionViewStyle") {
              DRW_SectionViewStyle e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addSectionViewStyle(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "BREAKDATA" ||
                cit->second->className == "AcDbBreakData") {
              DRW_BreakData e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addBreakData(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "BREAKPOINTREF" ||
                cit->second->className == "AcDbBreakPointRef") {
              DRW_BreakPointRef e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addBreakPointRef(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "GEODATA" || cit->second->className == "AcDbGeoData") {
              DRW_GeoData e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addGeoData(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "IMAGEDEF_REACTOR" ||
                cit->second->className == "AcDbRasterImageDefReactor") {
              DRW_ImageDefinitionReactor e;
              ret = parseTableEntry(e);
              // Preserving the reactor object keeps each raster
              // IMAGE entity's reactor handle non-dangling. (Phase 2b.4)
              if (ret) {
                intfa.addImageDefinitionReactor(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "SPATIAL_FILTER" ||
                cit->second->className == "AcDbSpatialFilter") {
              DRW_SpatialFilter e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addSpatialFilter(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            // INDEX (AcDbIndex) — ODA dwg.spec: TIMEBLL body followed
            // by the common object handle stream.
            if (rn == "INDEX" || cit->second->className == "AcDbIndex") {
              DRW_Index e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addIndex(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            // IDBUFFER (AcDbIdBuffer) — ODA §20.4.79. List of object
            // handles, used by selection filters (LAYER_INDEX entries
            // point to one of these for the per-layer entity set).
            if (rn == "IDBUFFER" || cit->second->className == "AcDbIdBuffer") {
              DRW_IDBuffer e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addIDBuffer(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            // LAYER_INDEX (AcDbLayerIndex) — ODA §20.4.83. Per-layer
            // entity index, used for partial-load drawings.
            if (rn == "LAYER_INDEX" ||
                cit->second->className == "AcDbLayerIndex") {
              DRW_LayerIndex e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addLayerIndex(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            // SPATIAL_INDEX (AcDbSpatialIndex) — ODA §20.4.95.
            // Spatial entity index; only timestamps are parsed
            // (body beyond is opaque per ODA spec).
            if (rn == "SPATIAL_INDEX" ||
                cit->second->className == "AcDbSpatialIndex") {
              DRW_SpatialIndex e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addSpatialIndex(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "TABLEGEOMETRY" ||
                cit->second->className == "AcDbTableGeometry") {
              DRW_TableGeometry e;
              ret = parseTableEntry(e);
              // Raw replay preserves the full byte image. (Phase 2b.4)
              if (ret) {
                intfa.addTableGeometry(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "MLEADERSTYLE") {
              DRW_MLeaderStyle e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addMLeaderStyle(&e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            // recName is the DXF CLASSES section record name (code 1),
            // not the C++ className (code 2 = "AcDbColor").  Match the
            // DXF spelling "DBCOLOR" — same convention as MLEADERSTYLE
            // above.  Populate dbColorMap so entity-side resolution in
            // entryParse (dwgreader.h) can patch color24 + colorName
            // onto entities referencing this DBCOLOR via the ENC flag
            // 0x40 handle.
            if (rn == "DBCOLOR" || cit->second->className == "AcDbColor") {
              DRW_DbColor e;
              ret = parseTableEntry(e);
              if (ret) {
                std::string formatted =
                    e.bookName.empty() ? e.name : (e.bookName + "$" + e.name);
                dbColorMap[obj.handle] = {e.rgb, formatted};
                intfa.addDbColor(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            // PLOTSETTINGS — plot configuration object (paper size,
            // margins, plotter name, etc.). DXF dispatches via
            // libdxfrw.cpp; the DWG path used to drop these into
            // remainingMap. libreDWG dwg.spec:5627 confirms
            // `DWG_OBJECT (PLOTSETTINGS)`; objects.in:321 marks the
            // dxfname as "PLOTSETTINGS".  RS_FilterDXFRW already
            // implements addPlotSettings (margins → m_graphic).
            if (rn == "PLOTSETTINGS" ||
                cit->second->className == "AcDbPlotSettings") {
              DRW_PlotSettings e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addPlotSettings(&e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            // OBJECTCONTEXTDATA (annotative per-object context) -
            // metadata-only shell. Text/MTEXT, dimension-family, leader,
            // block-reference and FCF contexts are typed for corpus
            // coverage, but raw DWG bytes are still emitted for lossless
            // replay. MLeader keeps its version-specific body raw.
            {
              DRW_ObjectContextData::Kind contextKind =
                  DRW_ObjectContextData::Kind::Unknown;
              if (objectContextKindFromClassNames(rn, cit->second->className,
                                                  contextKind)) {
                DRW_ObjectContextData e(
                    rn.empty() ? cit->second->className : rn, contextKind);
                ret = parseTableEntry(e);
                if (ret) {
                  intfa.addObjectContextData(e);
                  // Context-data suffixes vary by owning entity and
                  // DWG version. A structurally valid frame can be
                  // preserved even when this decoder cannot type
                  // its version-specific body.
                  intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
                }
                break;
              }
            }
            // SCALE (AcDbScale) — annotation-scale entry, ODA §20.4.93.
            // Lives under ACAD_SCALELIST in the named-object dictionary.
            // libreDWG dwg2.spec:1195 (DWG_OBJECT (SCALE)).  recName
            // "SCALE" or className "AcDbScale".  RS_FilterDXFRW currently
            // discards (no annotation-scale-aware viewport) but the
            // parser foundation lands so future per-scale resolution
            // can build on a populated handle map.
            if (rn == "SCALE" || cit->second->className == "AcDbScale") {
              DRW_Scale e;
              ret = parseTableEntry(e);
              if (ret) {
                scaleMap[obj.handle] = e;
                intfa.addScale(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            // VISUALSTYLE — AcDbVisualStyle (ODA spec §20.4.95).
            // Typed fields are decoded for metadata consumers; the
            // raw shelf remains the lossless replay representation.
            // LibreCAD has no 3D consumer. recName "ACDB_VISUALSTYLE_CLASS"
            // per spec; className fallback for files using the
            // C++ class spelling.
            if (rn == "ACDB_VISUALSTYLE_CLASS" ||
                cit->second->className == "AcDbVisualStyle") {
              DRW_VisualStyle e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addVisualStyle(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            // UNDERLAYDEFINITION — AcDb{Pdf,Dgn,Dwf}Definition.
            // Three flavors share one parser; routed by recName.
            // Object lives in OBJECTS section AFTER entities are
            // parsed; LibreCAD filter caches by handle for matching.
            {
              const std::string &cn = cit->second->className;
              if (rn == "PDFDEFINITION" || rn == "DGNDEFINITION" ||
                  rn == "DWFDEFINITION" || cn == "AcDbPdfDefinition" ||
                  cn == "AcDbDgnDefinition" || cn == "AcDbDwfDefinition") {
                DRW_UnderlayDefinition e;
                if (rn == "DGNDEFINITION" || cn == "AcDbDgnDefinition")
                  e.kind = DRW_UnderlayDefinition::DGN;
                else if (rn == "DWFDEFINITION" || cn == "AcDbDwfDefinition")
                  e.kind = DRW_UnderlayDefinition::DWF;
                ret = parseTableEntry(e);
                if (ret) {
                  intfa.linkUnderlay(&e);
                  intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
                }
                break;
              }
            }
            if (rn == "POINTCLOUDDEFINITION" ||
                rn == "POINTCLOUDDEFINITIONEX" ||
                rn == "POINTCLOUDDEFREACTOR" ||
                rn == "POINTCLOUDDEFREACTOREX" ||
                cit->second->className == "AcDbPointCloudDef" ||
                cit->second->className == "AcDbPointCloudDefEx" ||
                cit->second->className == "AcDbPointCloudDefReactor" ||
                cit->second->className == "AcDbPointCloudDefReactorEx") {
              DRW_PointCloudDef e;
              if (rn == "POINTCLOUDDEFINITIONEX" ||
                  cit->second->className == "AcDbPointCloudDefEx") {
                e.m_kind = DRW_PointCloudDef::DefinitionEx;
              } else if (rn == "POINTCLOUDDEFREACTOREX" ||
                         cit->second->className ==
                             "AcDbPointCloudDefReactorEx") {
                e.m_kind = DRW_PointCloudDef::ReactorEx;
              } else if (rn == "POINTCLOUDDEFREACTOR" ||
                         cit->second->className == "AcDbPointCloudDefReactor") {
                e.m_kind = DRW_PointCloudDef::Reactor;
              }
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addPointCloudDef(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "NAVISWORKSMODELDEF" ||
                cit->second->className == "AcDbNavisworksModelDef") {
              DRW_NavisworksModelDef e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addNavisworksModelDef(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "POINTCLOUDCOLORMAP" ||
                cit->second->className == "AcDbPointCloudColorMap") {
              DRW_PointCloudColorMap e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addPointCloudColorMap(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "LIGHTLIST" || rn == "ACDBLIGHTLIST" ||
                cit->second->className == "AcDbLightList") {
              DRW_LightList e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addLightList(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "LAYERFILTER" ||
                cit->second->className == "AcDbLayerFilter") {
              DRW_LayerFilter e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addLayerFilter(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "DATALINK" || cit->second->className == "AcDbDataLink") {
              DRW_DataLink e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addDataLink(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
            if (rn == "GEOMAPIMAGE" || rn == "ACDBGEOMAPIMAGE" ||
                cit->second->className == "AcDbGeomapImage") {
              DRW_GeoMapImage e;
              ret = parseTableEntry(e);
              if (ret) {
                intfa.addGeoMapImage(e);
                intfa.addUnsupportedObject(makeRawObject(oType, cit->second));
              }
              break;
            }
          }
        }
        // not supported object or entity add to remaining map for debug
        {
          // R2007+ object frames expose the detached common-handle
          // stream. An unrecognized custom class is safe to preserve
          // only after that stream validates; otherwise a truncated
          // owner/reactor/xdictionary tail would become a public raw
          // callback. R2000/R2004 opaque bodies have no generic safe
          // boundary, so retain their existing frame-bounded fallback.
          if (version > DRW::AC1018 && resolvedClass != nullptr) {
            RawObjectShell shell;
            ret = shell.parseDwg(version, &buff, bs) && buff.isGood();
            if (ret) {
              const std::string &recordName = resolvedClass->recName;
              const std::string &className = resolvedClass->className;
              const std::string &objectName =
                  recordName.empty() ? className : recordName;
              ++m_skippedCustomClasses[className.empty() ? recordName.c_str()
                                                         : className];
              ++m_skippedUnsupportedObjects[objectName];
              intfa.addUnsupportedObject(
                  makeRawObject(oType, resolvedClass, false));
              remainingMap[obj.handle] = obj;
              DRW_DBG("[custom-object-skipped ");
              DRW_DBG(objectName.c_str());
              DRW_DBG("]\n");
            }
            break;
          }
          std::string objectName;
          std::string recordName;
          std::string className;
          if (oType >= 500) {
            auto cit = classesmap.find(oType);
            if (cit != classesmap.end() && cit->second) {
              recordName = cit->second->recName;
              className = cit->second->className;
              objectName = recordName.empty() ? className : recordName;
              const char *clsName =
                  className.empty() ? recordName.c_str() : className.c_str();
              ++m_skippedCustomClasses[clsName];
            }
          }
          if (objectName.empty())
            objectName = "type-" + std::to_string(oType);
          ++m_skippedUnsupportedObjects[objectName];
          DRW_UnsupportedObject raw = makeRawObject(
              oType,
              (oType >= 500 && classesmap.find(oType) != classesmap.end())
                  ? classesmap.find(oType)->second
                  : nullptr,
              false);
          raw.m_recordName = recordName;
          raw.m_className = className;
          intfa.addUnsupportedObject(raw);
          DRW_DBG("[unsupported-object-skipped ");
          DRW_DBG(objectName.c_str());
          DRW_DBG("]\n");
        }
        remainingMap[obj.handle] = obj;
        break;
      }
    if (!ret) {
      // As with entities, a failed typed OBJECTS parser is not a raw
      // preservation success. Valid opaque fixed/custom shell routes
      // publish from their successful dispatch arms above.
      DRW_DBG("Warning: Object type ");
      DRW_DBG(oType);
      DRW_DBG("has failed, handle: ");
      DRW_DBG(obj.handle);
      DRW_DBG("\n");
    }
    if (ret) {
      publication.m_carrier =
          rawViewIssued ? (rawViewHasTypedPeer
                               ? DRW_DwgFramePublication::Carrier::TypedAndRaw
                               : DRW_DwgFramePublication::Carrier::Raw)
                        : DRW_DwgFramePublication::Carrier::Typed;
      if (!publishDwgFramePublication(
              intfa, publication,
              {dictionaryMembership ? &*dictionaryMembership : nullptr,
               typedReference ? &*typedReference : nullptr, nullptr,
               groupMembership ? &*groupMembership : nullptr,
               sortEntsMembership ? &*sortEntsMembership : nullptr,
               fieldListMembership ? &*fieldListMembership : nullptr,
               dictionaryWithDefaultMembership
                   ? &*dictionaryWithDefaultMembership
                   : nullptr,
               fieldPayloadReceipt ? &*fieldPayloadReceipt : nullptr},
              {fieldOutput ? &*fieldOutput : nullptr,
               fieldListOutput ? &*fieldListOutput : nullptr,
               fieldRawOutput ? &*fieldRawOutput : nullptr}))
        ret = false;
    }
    return ret;
  } catch (...) {
    // A decoder or consumer failure must not escape the per-object
    // recovery loop. A HANDLE-map source can distinguish the latter
    // from an ordinary unsuccessful body parse in the final report.
    if (m_dwgFrameCoverageStatus != DRW_DwgFrameCoverageStatus::NotAvailable) {
      (void)markDwgFrameOutcome(sourceFrameId(obj),
                                DRW_DwgFrameDisposition::Failed,
                                DRW_DwgFrameCoverageReason::CallbackException);
    }
    return failStructural();
  }
}

bool DRW_ObjControl::parseDwg(DRW::Version version, dwgBuffer *buf,
                              std::uint32_t bs) {
  const auto fail = [this, buf]() {
    if (buf != nullptr)
      buf->invalidate();
    reset();
    return false;
  };
  if (buf == nullptr)
    return fail();
  reset();

  int unkData = 0;
  bool ret = DRW_TableEntry::parseDwg(version, buf, nullptr, bs);
  DRW_DBG("\n***************************** parsing object control entry "
          "*********************************************\n");
  if (!ret)
    return fail();
  dwgBuffer hBuff = *buf;
  dwgBuffer *hBuf = buf;
  if (version > DRW::AC1021) {
    const std::uint64_t totalBits =
        static_cast<std::uint64_t>(buf->size()) * 8u;
    if (totalBits < bs || totalBits - bs > UINT32_MAX)
      return fail();
    const std::uint32_t objectBits = static_cast<std::uint32_t>(totalBits - bs);
    if (!hBuff.setPosition(objectBits >> 3))
      return fail();
    hBuff.setBitPos(static_cast<std::uint8_t>(objectBits & 7u));
    hBuf = &hBuff;
  }
  const std::uint64_t totalBits = static_cast<std::uint64_t>(buf->size()) * 8u;
  const std::uint64_t bodyEndBit = objSize != 0 ? objSize : totalBits;
  if (bodyEndBit > totalBits)
    return fail();
  // last parsed is: XDic Missing Flag 2004+
  std::int32_t rawNumEntries = 0;
  const bool countRead =
      controlEntryCountUsesBitShort(oType)
          ? readControlBitShort(*buf, bodyEndBit, rawNumEntries)
          : readControlBitLong(*buf, bodyEndBit, rawNumEntries);
  if (!countRead || rawNumEntries < 0)
    return fail();
  const std::uint32_t numEntries = static_cast<std::uint32_t>(rawNumEntries);
  const std::uint32_t phantomEntryCount =
      controlHasPhantomEntries(oType) ? 2U : 0U;
  if (numEntries > dwgSafety::MaxOwnedObjectCount ||
      numEntries >
          std::numeric_limits<std::uint32_t>::max() - phantomEntryCount)
    return fail();
  DRW_DBG(" num entries: ");
  DRW_DBG(numEntries);
  DRW_DBG("\n");
  DRW_DBG("Remaining bytes: ");
  DRW_DBG(buf->numRemainingBytes());
  DRW_DBG("\n");

  //    if (oType == 68 && version== DRW::AC1015){//V2000 dimstyle seems have
  //    one unknown byte hard handle counter??
  if (oType == 68 && version > DRW::AC1014) { // dimstyle seems have one unknown
                                              // byte hard handle counter??
    std::uint8_t parsedUnknown = 0;
    if (!readControlRawChar(*buf, bodyEndBit, parsedUnknown))
      return fail();
    unkData = parsedUnknown;
    DRW_DBG(" unknown v2000 byte: ");
    DRW_DBG(unkData);
    DRW_DBG("\n");
  }
  if (version > DRW::AC1018) { // from v2007+ have a bit for strings follows
                               // (ObjControl do not have)
    bool stringBit = false;
    if (!readControlBit(*buf, bodyEndBit, stringBit))
      return fail();
    DRW_DBG(" string bit for  v2007+: ");
    DRW_DBG(stringBit);
    DRW_DBG("\n");
  }
  if (!buf->isGood())
    return fail();

  dwgHandle objectH;
  if (!readDwgHandleChecked(*hBuf, 0, false, objectH))
    return fail();
  DRW_DBG(" NULL Handle: ");
  DRW_DBGHL(objectH.code, objectH.size, objectH.ref);
  DRW_DBG("\n");
  DRW_DBG("Remaining bytes: ");
  DRW_DBG(buf->numRemainingBytes());
  DRW_DBG("\n");

  //    if (oType == 56 && version== DRW::AC1015){//linetype in 2004 seems not
  //    have XDicObjH or NULL handle
  if (xDictFlag !=
      1) { // linetype in 2004 seems not have XDicObjH or NULL handle
    dwgHandle XDicObjH;
    if (!readDwgHandleChecked(*hBuf, 0, false, XDicObjH))
      return fail();
    xDictHandle = XDicObjH.ref;
    DRW_DBG(" XDicObj control Handle: ");
    DRW_DBGHL(XDicObjH.code, XDicObjH.size, XDicObjH.ref);
    DRW_DBG("\n");
    DRW_DBG("Remaining bytes: ");
    DRW_DBG(buf->numRemainingBytes());
    DRW_DBG("\n");
  }
  // add 2 for modelspace, paperspace blocks & bylayer, byblock linetypes
  const std::uint64_t declaredHandleCount =
      static_cast<std::uint64_t>(numEntries) + phantomEntryCount +
      static_cast<std::uint64_t>(unkData);
  if (declaredHandleCount >
      static_cast<std::uint64_t>(std::max(0, hBuf->numRemainingBytes()))) {
    return fail();
  }

  const std::uint64_t childHandleCount =
      static_cast<std::uint64_t>(numEntries) + phantomEntryCount;
  std::list<std::uint32_t> parsedHandles;
  std::unordered_set<std::uint32_t> seenHandles;

  for (std::uint64_t i = 0; i < childHandleCount; i++) {
    if (!readDwgHandleChecked(*hBuf, handle, true, objectH))
      return fail();
    if (objectH.ref != 0) { // in vports R14  I found some NULL handles
      if (!seenHandles.insert(objectH.ref).second)
        return fail();
      parsedHandles.push_back(objectH.ref);
    }
    DRW_DBG(" objectH Handle: ");
    DRW_DBGHL(objectH.code, objectH.size, objectH.ref);
    DRW_DBG("\n");
    DRW_DBG("Remaining bytes: ");
    DRW_DBG(buf->numRemainingBytes());
    DRW_DBG("\n");
  }

  for (int i = 0; i < unkData; i++) {
    if (!readDwgHandleChecked(*hBuf, handle, true, objectH))
      return fail();
    DRW_DBG(" unknown Handle: ");
    DRW_DBGHL(objectH.code, objectH.size, objectH.ref);
    DRW_DBG("\n");
    DRW_DBG("Remaining bytes: ");
    DRW_DBG(buf->numRemainingBytes());
    DRW_DBG("\n");
  }
  handlesList = std::move(parsedHandles);
  return buf->isGood() && hBuf->isGood() ? true : fail();
}
