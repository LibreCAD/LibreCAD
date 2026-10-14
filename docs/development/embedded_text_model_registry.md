# Provisional embedded-text model registry

This is the field contract for the inert `LC_EmbeddedTextModel` service. It is
not an activated DXF/DWG carrier, a native-wire compatibility claim, a profile
schema freeze, a font resolver, or permission to create scene entities. A known
decoded record establishes framing and this registry only. Graph ownership,
observed native baseline, STYLE association and runtime validation remain
separate acceptance boundaries.

The implementation is in
[`lc_embeddedtextmodel.h`](../../librecad/src/lib/filters/lc_embeddedtextmodel.h)
and [`lc_embeddedtextmodel.cpp`](../../librecad/src/lib/filters/lc_embeddedtextmodel.cpp).
Native mappings refer to the actual members in
[`drw_entities.h`](../../libraries/libdxfrw/src/drw_entities.h) and
[`drw_base.h`](../../libraries/libdxfrw/src/drw_base.h), not an older filter
sidecar or a rendering projection. Some retained native fields are protected or
private; reserving their storage here does not invent public accessors or prove
that an application bridge captures them.

## Versions, framing and ownership

The current **envelope version is 1 and model schema is 2**. Schema 2 requires
complete retained handle components, raw owner/link/presence state and embedded
X-axis precedence. The earlier unreleased schema 1 omitted those fields; it is
retained as `Opaque`, including every bounded input byte, never migrated by
zero-filling or activated. Unknown envelope/schema versions and unknown fields
or kinds likewise publish no model and retain bounded raw. A schema-2 record
missing required fields is `Invalid`, not an older model with defaults.
This provisional service migration is independent of the `LibreCADText` profile
schema and does not freeze either native transport or the model protocol.

The 64-byte envelope is `LCEM`, u16 envelope, u16 schema, u32 header size,
u64 body bytes, u32 top-level field count, SHA256(body)[32], u64 reserved zero.
Each field is u16 ID, u8 kind, u8 flags zero, u64 payload bytes and payload.
IDs are ascending and unique within each record. Integers are little-endian;
signed storage uses two's-complement bits. `Real` is exact IEEE754 binary64;
`Coord` is x/y/z in that order. No floating-point normalization or approximate
equality is allowed, including NaN payloads, infinities and signed zero.

| Kind | Number | Payload |
| --- | ---: | --- |
| Bool | 1 | One byte, exactly 0 or 1 |
| U8/U16/U32/U64 | 2/3/4/5 | 1/2/4/8 bytes |
| I32/I64 | 6/7 | 4/8 exact bytes |
| Real/Coord | 8/9 | 8/24 exact bytes |
| Bytes | 10 | Exact bytes; no null state |
| Utf16 | 11 | Nullness byte, u64 code-unit count, exact LE UTF16 units |
| Record | 12 | u64 field count and fields |
| U32Array/RealArray | 13/14 | u64 element count and fixed-width elements |
| RecordArray | 15 | u64 count, then u64 byte length and record per element |
| RawTags | 16 | `LC_EmbeddedTextXData` v2 capsule, not native wire XDATA |
| RawTagArrays | 17 | u64 count, then u64 length and capsule per ordered list |
| NullableBytes | 18 | Nullness byte, u64 byte count, exact bytes |

Nullness is 0 for null and 1 for nonnull; null requires count zero. Omitted,
present-null and present-empty differ. Malformed UTF16, NUL and invalid UTF8 are
retained where their storage permits them. All fields own their payload bytes.
RawTags retains full signed code/type, ordered null slots versus `INVALID`,
binary/string bytes, IEEE bits and active DWG1003 source-version/u64 provenance.
AppData list boundaries are not flattened. QVariant semantic equality is not
raw-byte or dirty-state authority.

All listed fields are required unless marked optional. Final encode/decode share
aggregate ceilings of 64 MiB, 1,000,000 fields/array elements/raw variants, and
depth 16; framing and nested payloads count. Framing-only builders enforce local
immediate counts and bytes, not nested schema/depth/item acceptance. Oversize or
unavailable allocation cannot be claimed retained: the bridge must fail visibly
and preserve the original input externally. These are memory limits, not native
DXF/DWG wire allowances.

## Top-level records

| ID | Kind | Meaning |
| ---: | --- | --- |
| 1 | Record | CurrentSource: actual owning current source/profile/STYLE/N |
| 2 | Record, optional | ImportedSource: captured immutable historical input |
| 3 | Record | OuterAttribute: current prepared ATTRIB/ATTDEF fields |
| 4 | Record | EmbeddedMText: complete model even when target omits native body |
| 5 | Record | Legacy marker/ambiguous ownership provenance |
| 6 | Record | TargetWireBaseline: actual target typed reread, not a render view |
| 7 | RecordArray, optional | OpaqueExtras: already-retained named/versioned bytes |

## Shared native records

These IDs apply to OuterAttribute, EmbeddedMText and baseline native records.
Names below are exact `DRW_Entity` members unless an accessor is named. Retain raw
and semantic fields independently, even if they conflict. A source reference,
presence flag or stored wide handle is not a validated live graph relationship.

| ID | Kind | Native member / accessor |
| ---: | --- | --- |
| 1 | I32 | `space` enum storage |
| 2 | Bytes | `layer` |
| 3 | Bytes | `lineType` |
| 4 | U32 | `material` |
| 5 | I32 | `color` |
| 6 | I32 | `lWeight` enum storage |
| 7 | Real | `ltypeScale` |
| 8 | Bool | `visible` |
| 9 | I32 | `numProxyGraph`, independent of actual byte length |
| 10 | Bytes | `proxyGraphics` |
| 11 | I32 | `color24` |
| 12 | Bytes | `colorName` |
| 13 | I32 | `transparency` |
| 14 | I32 | `plotStyle`, including exact signed high-bit storage |
| 15 | I32 | `shadow` enum storage |
| 16 | U32 | `shadowHandle` |
| 17 | U32 | `fullVisualStyleHandle` |
| 18 | U32 | `faceVisualStyleHandle` |
| 19 | U32 | `edgeVisualStyleHandle` |
| 20 | U32Array | `reactorHandles`, exact order including retained zeros/duplicates |
| 21 | U32 | `xDictHandle`, source relation only |
| 22 | Bool | `haveExtrusion` |
| 23 | RawTags | `extData` |
| 24 | RawTagArrays | `appData`, ordered lists and values |
| 25 | Bool | `hasDataStorageRecord` |
| 26 | U64 | `dataStorageHandle` |
| 27 | Bytes | `dataStorageHandleKey` |
| 28 | Bytes | `dataStorageData` |
| 29 | U32 | `dataStorageSegmentIndex` |
| 30 | U32 | `dataStorageSchemaIndex` |
| 31 | Bool | `hasDataStoragePayloadMarker` |
| 32 | U32 | `dataStoragePayloadMarkerOffset` |
| 33 | U32 | `dataStoragePayloadMarkerLength` |
| 34 | Bytes | `dataStoragePayloadMarkerSection` |
| 35 | Bool | `hasDataStorageBinaryData()`, semantic view of `hasDsData` |
| 36 | Bool | `hasDwgAcDbColorHandle()` / `hasAcDbColorH` |
| 37 | U32 | `dwgAcDbColorHandle()` / `acDbColorHandle` |
| 38 | U32 | `dwgLayerHandle()` / `layerH.ref` only |
| 39 | U32 | `dwgLinetypeHandle()` / `lTypeH.ref` only |
| 40 | RecordArray | `pendingAppIdResolutions` |
| 41 | RecordArray | `pendingLayerRefResolutions` |
| 42 | U8 | `layerH.code` |
| 43 | U8 | `layerH.size` |
| 44 | U64 | `layerH.ref64`, independent of field 38 |
| 45 | U8 | `lTypeH.code` |
| 46 | U8 | `lTypeH.size` |
| 47 | U64 | `lTypeH.ref64`, independent of field 39 |
| 48 | U32 | `handle`, original own identity, not output ownership proof |
| 49 | U32 | `parentHandle`, original owner relationship |
| 50 | U8 | `haveNextLinks`, exact retained no-links/presence byte |
| 51 | U32 | `prevEntLink`, original sequence relationship |
| 52 | U32 | `nextEntLink`, original sequence relationship |
| 53 | Bool | `ownerHandle`, raw owner-presence state |
| 54 | U8 | `xDictFlag`, independent of field 21 |
| 55 | I32 | `numReactors`, independent of field 20 |
| 56 | U8 | `plotFlags` |
| 57 | U8 | `ltFlags` |
| 58 | U8 | `materialFlag` |
| 59 | U8 | `shadowFlag`, including unsupported high bits |
| 60 | U8 | `hasFullVisualStyle` |
| 61 | U8 | `hasFaceVisualStyle` |
| 62 | U8 | `hasEdgeVisualStyle` |
| 63 | U8 | `hasDsData`, exact retained byte independent of semantic field 35 |
| 64 | I32 | `oType`, retained int16 source type, not an unchecked class selector |

Each pending-reference record is: 1 U64 `indexInExtData`, 2 U32 `handleRef`,
3 U64 `rawHandleRef`, 4 Bool `useRawHandleRef`. `size_t` conversion is checked.
The bridge must not invent resolved names or reuse stale indexes after changing
namespaces. It must prepare coherent resolution/edit state or refuse safely.

Complete `dwgHandle` means code, size, low32 ref and full ref64, independently.
The current layer/linetype public setters assign only ref/code, so they are not
an exact reconstruction API. `ltFlags` is not merely a cache: the common writer
prefers retained flags before deriving from lineType. Other presence bytes can
retain present-null or unsupported distinctions even when a writer recomputes
them. The codec does not reconcile those differences.

### Point, line and text fields

| ID | Kind | Exact native member |
| ---: | --- | --- |
| 100 | Coord | `DRW_Point::basePoint` |
| 101 | Coord | `DRW_Line::secPoint` |
| 102 | Coord | `DRW_Point::extPoint` |
| 103 | Real | `thickness` |
| 104 | Real | `xAxisAngle`, stored radians; no conversion |
| 105 | Real | `DRW_Text::height` |
| 106 | Bytes | `text`, prepared current native bytes, not legacy `getText()` |
| 107 | Real | `angle`, stored DRW degrees; no radian round-trip |
| 108 | Real | `widthscale`, TEXT width factor or MTEXT rectangle width |
| 109 | Real | `oblique` |
| 110 | Bytes | `style`, exact native STYLE identity, not compatibility font alias |
| 111 | I32 | `textgen`, TEXT generation or MTEXT attachment semantics |
| 112 | I32 | `alignH`, TEXT alignment or MTEXT drawing direction |
| 113 | I32 | `alignV`, independent of MTEXT `linespacingStyle` |
| 114 | U8 | `styleH.code` |
| 115 | U8 | `styleH.size` |
| 116 | U32 | `styleH.ref` |
| 117 | U64 | `styleH.ref64`, independent of low32 field 116 |

### Outer-specific fields

| ID | Kind | `DRW_Attrib` member |
| ---: | --- | --- |
| 200 | Bytes | `tag` |
| 201 | U8 | `attribFlags` |
| 202 | U8 | `attVersion` |
| 203 | U8 | `m_attributeType` |
| 204 | U8 | `keepDuplicateRecords` |
| 205 | I32 | `m_fieldLength` |
| 206 | Bool | `lockPosition` |
| 207 | Bytes, optional | `DRW_Attdef::prompt`, present only for actual ATTDEF |

The bridge validates actual dynamic class/eType rather than reinterpreting a
contradictory prompt/class. Actual ATTDEF host restoration remains a distinct
gate. A `mtext` pointer's presence has model/body meaning; its address has none.
Outer fallback bytes and outer namespaces remain independent of embedded source.

### Embedded-specific fields

| ID | Kind | `DRW_MText` member |
| ---: | --- | --- |
| 200 | Real | `interlin` |
| 201 | U16 | `linespacingStyle` |
| 202 | I32 | `m_backgroundFlags` |
| 203 | Real | `m_backgroundScale` |
| 204 | I32 | `m_backgroundColor` |
| 205 | I32 | `m_backgroundTransparency` |
| 206 | Bool | `m_r2018IsNotAnnotative` |
| 207 | Bool | `m_r2018ReallyLocked` |
| 208 | U16 | `m_r2018Version` |
| 209 | Bool | `m_r2018DefaultFlag` |
| 210 | U32 | `m_r2018AppIdHandle` |
| 211 | I32 | `m_r2018Attachment` |
| 212 | Coord | `m_r2018XAxisDir` |
| 213 | Coord | `m_r2018InsertionPoint` |
| 214 | Real | `m_r2018RectWidth` |
| 215 | Real | `m_r2018RectHeight` |
| 216 | Real | `m_r2018ExtentsHeight` |
| 217 | Real | `m_r2018ExtentsWidth` |
| 218 | U16 | `m_r2018ColumnType` |
| 219 | I32 | `m_r2018ColumnCount` |
| 220 | Real | `m_r2018ColumnWidth` |
| 221 | Real | `m_r2018ColumnGutter` |
| 222 | Bool | `m_r2018ColumnAutoHeight` |
| 223 | Bool | `m_r2018ColumnFlowReversed` |
| 224 | RealArray | `m_r2018ColumnHeights`, exact order and IEEE bits |
| 225 | Bytes | `m_r2018AnnotativeData` |
| 226 | U16 | `m_r2018AnnotativeUnknown` |
| 227 | U32 | `m_r2018AnnotativeAppHandle` |
| 228 | Bool | `hasXAxisVec`, retained private X-axis/angle precedence state |

Field 228 is not safely discarded as finalized parser scratch: standalone
MTEXT calls `updateAngle()`, but nested ATTRIB/ATTDEF processing does not always
finalize it. Angle, second-point vector and this flag may disagree; preserve all
three without recomputing angle or inventing a flag. A parser-finalization repair
or native access API requires separate review and evidence.

## Current source, profiles and history

CurrentSource captures actual current owning state. Native records are prepared
current DRW copies after explicit edits; old sidecars cannot overwrite them.
Target-transcoded native text may differ from canonical source convention, so
native field 106 is not automatically CurrentSource field 5.

| ID | Kind | CurrentSource authority |
| ---: | --- | --- |
| 1 | Utf16 | `State.source()`, exact UTF16/nullness |
| 2 | U8 | `State.convention().syntax`: Plain0, MText1 |
| 3 | Bool | `decodeCaretCodes` |
| 4 | Bool | `decodeDxfEscapes` |
| 5 | NullableBytes, optional | Exact **current-convention** UTF8; explicit-length decoding equals field 1 |
| 6 | U8 | `State.origin()`: Native0, Dxf1, Dwg2 |
| 7 | U8 | Actual payload relation: Standalone0, AttributeText1, EmbeddedMText2 |
| 8 | Utf16 | Actual current original STYLE identity selected for export |
| 9 | Utf16 | Actual current `Data.style` compatibility alias |
| 10 | U8 | Prepared current marker: None0, LegacyReversed1, LogicalRtl2 |
| 11 | U8 | Current marker origin: None0, Entity1, AttributeOuter2 |
| 12 | Utf16 | Original STYLE provenance; imported identity if actually present |
| 13 | Utf16 | Current `State.compatibilityInput()` **before** one leading A-strip |
| 14 | Utf16 | Current `State.compatibilityText()` / final legacy Data.text |
| 15 | Bool | `State.textEdited()` at capture, not a later dirty-authority setter |
| 16 | Record | Materialized current entity profile, including direction-only edits |
| 17 | Record | Current STYLE association/profile, not a shared STYLE replacement |
| 18 | Record | Actual current canonical local deformation N |

Field 5 cannot use stale imported UTF8 after a source/convention edit, even if a
legacy decoded string still looks equal. Field 13 captures the owned current
prestrip buffer, independently of imported history and final compatibility text;
do not reconstruct it from field 14 or replay the imported prestrip baseline.
Payload relation and current marker policy likewise require
actual host/prepared authority, not blind reuse of historical marker metadata.
Unchanged aliases may keep the original STYLE identity; real style edits win.

Deformation is a required nested record of 1 Real xx, 2 Real xy, 3 Real yx,
4 Real yy, the actual `Data.nativeDeformation` contract. Identity is (1,0,0,1)
only when actually held by the host. Never infer N from scalar height/angle/width,
normalize it during save, or invent historical N. Invalid IEEE values remain
inert metadata; renderer/activation validation separately requires finite,
invertible deformation. The affine Data/API may land in a separate checkpoint;
until then a bridge cannot claim actual host capture by supplying guessed identity.

ImportedSource has the same scalar IDs 1–14, with field 5 optional and field 8
the original imported STYLE. They map to immutable `State::Imported` source,
convention, UTF8, origin/payload, style/compatibilityStyle, marker/origin,
compatibilityInput and compatibilityText. Field 12 retains original STYLE.
Imported16 is HistoricalProfile. Imported18 is optional captured historical N,
which current `State::Imported` does not itself store. No other current-source
fields are copied into history by inference. Historical raw remains outside the
fresh State's current replay authority on load.

### Materialized EntityProfile / StyleProfile

| ID | Kind | Meaning |
| ---: | --- | --- |
| 1 | U8 | Absent0, Known1, Opaque2, Invalid3, Unavailable4 |
| 2 | U8 | StyleDefaults0 or EntityOverride1, locked by containing record |
| 3 | Record, optional | KnownValue, present exactly when Known |
| 4 | RawTags, optional | Exact current affected raw, present for Opaque/Invalid/Unavailable |
| 5 | U8 | Representable0, DuplicateNamespace1, UnrepresentableNamespace2 |
| 6 | Utf16 | Diagnostic, nonempty for Unavailable |

Read guarded `State.currentProfile()` / STYLE `getCurrentTextProfile()`, not
low-level binding Absent as evidence of transport availability. `current().raw`
is intentionally null. Untouched opaque/invalid raw comes from owned imported
binding raw or the retained affected transport stream. Replace materializes its
current owning value; Remove materializes Absent. Neither read nor save resets
pending Replace/Remove authority. Empty and direction-only entity values are
Known, not Absent, inherited selection or an implicit legacy opt-out.

KnownValue: optional 1 Record selection, optional 2 NullableBytes language,
optional 3 U8 direction (Auto0, LTR1, RTL2, ByStyle3). Selection has 1 U8 backend
(Legacy0, OpenType1), 2 Record identity iff OpenType. Identity requires 1 Utf16
profileId, 2 NullableBytes fontSha256, 3 U32 faceIndex, 4 Utf16 metricPolicy,
5 Utf16 shapingPolicy, 6 Utf16 engineIdentity. `LC_TextProfile::validate` checks
known values only; no resource is loaded or shaping engine certified.

HistoricalProfile: 1 U8 original status, 2 U8 original transport, 3 Utf16
diagnostic, optional 4 RawTags original raw (required exactly for non-Absent).
`binding.imported().raw` is the sole isolated raw authority; State::Imported
deliberately contains no second potentially shallow profile copy. Unavailable
history retains the complete affected raw transport stream, not fabricated Absent.

### STYLE authority boundary

CurrentStyle: 1 Utf16 actual STYLE identity, 2 Record materialized current STYLE
profile, optional 3 RawTags captured original STYLE profile raw. This is only an
association/current-profile record. It is **not** a complete `LC_TextStyle`
historical snapshot or a complete `DRW_Textstyle` record.

The actual STYLE/document transport must separately retain original STYLE name
and exact name UTF8, original unavailable transport/diagnostic (which current
Replace/Remove can hide), all original raw namespaces, standard STYLE scalar and
font fields, table flags and association/xref metadata. In particular,
`LC_TextStyle.importedName()/importedNameUtf8()` and its original unavailable
provenance cannot be reconstructed/defaulted from CurrentStyle3. The current
profile is independent from those histories. STYLE height, width, oblique,
generation, lastHeight, font, bigFont, fontFamily and xref references remain actual
STYLE/table authority, not values inferred from the entity carrier.

The graph/filter bridge must prove exact retained STYLE authority and reconcile
the actual live shared STYLE association/profile against its baseline, or refuse
safely while retaining input. A missing/ambiguous STYLE is not an absent profile
with defaults. No entity carrier may silently overwrite a shared STYLE or
activate unknown/global defaults. A separate additive STYLE-history record or
isolated override policy is not approved by this registry.

## Legacy, observed baseline and opaque extras

Legacy: 1 U8 marker (None0, Canonical1011, Legacy1002), 2 Bool ambiguousRaw,
3 Utf16 ownership diagnostic. Use actual `embeddedDxfMarker()` and
`hasAmbiguousLegacyEmbeddedXData()`. Trailing XDATA is stored on the outer
attribute, including Legacy100 input, consistently with master's reader.
Marker provenance survives copies, moves and assignment. The reader does not
create independent embedded XDATA; the ambiguity helper detects only a legacy
value that a caller constructed with XDATA on its embedded MTEXT. Such data has
no DXF frame and is not emitted there; callers must not rely on it as transport.
Canonical101 trailing XDATA belongs to the outer attribute.

TargetWireBaseline: 1 U8 format (Dxf0,Dwg1), 2 U32 actual DRW version,
3 U8 target syntax, 4 Bool target caret policy, 5 Bool target escape policy,
6 Bool actually observed native body, 7 Record observed outer, optional 8 Record
observed embedded exactly when field 6 is true. A pre2018 missing body is not
synthesized from the carrier. Capture actual low-level typed target reread, not
a filter/render projection or pre-wire double hash. Graph-linked raw fields
remain in the full model; any normalization/rebinding for baseline comparison
or activation needs an explicit separate comparison view and actual graph proof.

OpaqueExtras items require 1 Utf16 agreed record name, 2 U32 version,
3 Bytes exact already-retained data. This does not claim unknown, unparsed DXF
groups are captured; missing upstream raw cannot be repaired by inventing values.

## Explicit runtime exclusions and remaining bridge gates

Only these retained native members are excluded for the stated reasons:

- `DRW_Entity.curr`: parser coordinate/tag cursor, not authored model data.
- `extAxisX/extAxisY`: calculated extrusion basis; source coordinates are retained.
- `objSize/dwgDataEndBit`: input framing size/cursor bounds, recomputed for output.
- `commonLinkTailValidated`: runtime validation evidence, never restored as trust.
- `dwgEedAppIdWriteRefs/dwgEedLayerWriteRefs/dwgEedCodePage`: explicitly writer-only
  output resolution/codepage scratch, rebuilt per output. Public unresolved
  `pending*Resolutions` are different and are retained.
- `DRW_Attrib.dxfInAttributeSubclass`: DXF parser routing cursor. Marker provenance
  itself is retained in Legacy, and MTEXT `hasXAxisVec` is retained at 228.
- Constructor class/eType is checked against the fixed native role and actual
  ATTDEF prompt presence; unsupported/mismatched class reconstruction fails.

RS updateMode, rendering geometry/layout/metrics, font registry, tessellation,
selection/undo and runtime pointers are not canonical source/profile/N. Raw
own/parent/link handles are explicitly **not** covered by a blanket transient
exclusion; retain them, then resolve actual output graph/sequence separately.

Upstream capture gaps remain: some embedded DWG rectangle/extents/spacing values
are read into ignored locals; some parse-local common flags and complete handles
are reduced before the DRW object is published; unknown native groups may have
no retained raw representation. The codec cannot recover those values from
defaults. Three retained `dwgHandle` objects can be stored completely; this is
not proof every original native handle or wire byte survives the existing parser.

A future bounded pure bridge may capture owning current source/profile/STYLE/N
DTOs and DRW native records, then reconstruct inert private objects. Unsupported
enum/range values or unrepresentable appData null slots must remain inert with
visible refusal, not unchecked enum casts or silent defaults. All allocations
and raw conversions must complete before publication. Graph acceptance separately
checks ATTRIB extension dictionary ownership, one exact key/XRECORD owner,
duplicate/stale/unknown carriers, observed native equality and actual STYLE
authority. Fresh State initializes from materialized CurrentSource only after
those checks; old Imported remains historical provenance. No save resets live
edit authority. Native DXF/DWG limits, R12, three-version/cross-format roundtrip,
independent-reader evidence and private two-pass output publication remain
unproved by this service.
