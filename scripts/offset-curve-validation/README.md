# Offset Curve Validation Tools

Optional developer tooling for the auxiliary X-H1 through X-H7 roadmap. The
application and its builds do not depend on Node.js or on these receipt files.

## Implemented

X-H1 provides immutable format descriptors and receipt parsing/readback. Run:

```sh
node --test scripts/offset-curve-validation/test-preflight-receipt.js
```

Node.js 18 or later is required. Hardened filesystem readback requires POSIX
no-follow flags and Unix permission/link metadata. An unavailable capability is
a refusal, not a weaker successful validation. Unit fixtures use the host's
canonical temporary directory; none are product-validation receipts.

The private receipt schema is version 2 with stable revision `offset-2961`.
Earlier V509-V513 private revisions are negative fixtures only. Receipts explicitly
identify `purpose: validation` or `purpose: synthetic`; receipt-set validation
defaults to the validation purpose. Synthetic callers must supply the explicit
expected purpose and cannot relabel a nested result. A single generic inspection
does not make a synthetic receipt usable as validation evidence.

`schema-descriptors.js` exports data only. Independent checkers may share these
descriptors but must implement parsing and checking separately; they cannot call
the production validator. Parsing rejects duplicate keys, noncanonical bytes,
invalid encodings and nesting beyond 64 containers. Receipt strings reject
unpaired Unicode surrogates before filesystem conversion.

Readback binds actual report/stdout/stderr bytes, file and directory identity,
permissions, single-link regular files, exact membership and ordered pair
consistency. Revalidation detects content and inode substitution. The filesystem
model does not claim to eliminate all races on a concurrently hostile filesystem.
Ancestor guards retain device/inode/type/mode identity without treating unrelated
sibling activity as receipt mutation. Owned receipt directories and files keep
full metadata snapshots. Directory enumeration stops at the first extra member
instead of allocating an unbounded entry list. Final identity checks cover changes
after enumeration, after an individual file was read, and while the other receipt
was validated. Protected roots are compared through physical paths, resolving an
existing prefix even when the final component does not exist. PASS inventory
records require matching identity digests as well as matching counts.

## X-H2 Input And Process Capture

Run the focused capture tests, or include the H1 suite as a regression check:

```sh
node --test scripts/offset-curve-validation/test-input-capture.js scripts/offset-curve-validation/test-preflight-capture.js
node --test scripts/offset-curve-validation/test-*.js
```

`input-capture.js` owns a separate version-1 input-manifest format. Its
`captureInputs(config)` API requires explicit `purpose`, `candidatePath`,
`runnerPath`, `caseInventoryPath`, and the arrays `sourcePaths`, `inputPaths`,
`fixturePaths`, `configPaths`. Empty input/fixture/config arrays are explicit;
the source array must be nonempty. All paths must be absolute, normalized and
free of symlink components. Files must be regular and single-link. Sources,
candidate, runner, inventory and configuration files must be nonempty; actual
empty raw input files are allowed. Missing files are errors, never dummy records.
Caller-supplied candidate tuples or `sourceManifestSha256` are refused.

The canonical JSON+LF case inventory has ordered keys `schema_version` (1),
`purpose`, `cases`. Each case has ordered keys `id`, `source_path`; IDs are
nonempty, sorted and unique, and definitions must name a declared source or
fixture file. This is the exact declared inventory, not proof that a future
worker executed those cases. H3/H7 still own worker wiring and execution reports.

The returned immutable manifest includes the candidate tuple computed from actual
bytes (SHA-256, byte count and Git SHA-1 blob hash), sorted file paths and roles,
actual hashes/bytes for every declared file, the runner, four capture/schema
modules, `process.execPath`, `process.version` and the resolved actual executable.
`sourceManifestSha256` hashes this manifest and binds H1 schema-2 receipts without
changing H1. It is an explicit file closure, not automatic dependency discovery;
in particular, loaded dynamic libraries and transitive runner imports are not
discovered. Callers must declare source/fixture/config/input dependencies.

Limits are 128 distinct files, 8 MiB per ordinary file, 128 MiB for the executable,
256 MiB total, 1 MiB for the inventory and 4096 cases. Immutable base64 byte and
full metadata snapshots remain in the returned `inputs`; `revalidateInputs`
requires the original in-process capture and rereads bytes and metadata. Snapshot
metadata includes device/inode, type/mode, link count, size, mtime and ctime, not
atime (ordinary reads can update atime). Ancestor guards pin device/inode/type/mode
without treating unrelated sibling activity as input mutation. Captures with the
same file bytes and declared tuple have the same manifest hash; filesystem
metadata is retained for revalidation, not included in that hash.

`preflight-capture.js` exports async `capturePreflight(config, dependencies?)`.
In addition to the input fields above it requires `runnerArgs`, `cwd`, an explicit
string-valued `environment`, UUIDv4 `receiptId`, absent `receiptDirectory`, and
absent `runRoot` with existing real parents. Optional `maxStreamBytes` lowers the
64 MiB per-stream cap; `timeoutMs` lowers the five-minute direct-child kill
deadline. Capture waits for the child's `close` event, not merely `exit`. This is
not process-tree containment: descendants retaining inherited pipes can delay
closure; H3 still owns worker lifecycles. No shell is used.
The generated tuple/inventory/root-observation context is one bounded canonical
JSON+LF payload on stdin (fd 0), followed by EOF. The limit is 32 KiB. Runners must
read this channel; no per-run root/parent context is added to argv or environment.
The obsolete `LC_OFFSET_CAPTURE_CONTEXT` environment key is refused. Actual argv
and the entire actual launch environment are retained and hashed without removing
or normalizing per-run values. Callers can therefore capture two distinct roots
with identical pinned inputs/runtime/argv/cwd/environment and join them through
H1 without weakening its equality checks. A real-child pair regression exercises
the join and revalidation. Node, loader and OpenSSL injection environment keys
are refused, not inherited. On
macOS callers must also explicitly include the current runtime's
`__CF_USER_TEXT_ENCODING` value: the platform injects/replaces it even for a
custom environment, so omitting it would make the environment digest misleading.
The complete sorted parent environment is compared but never changed or restored.

The returned `stdin` snapshot retains the exact generated payload bytes, byte
count, SHA-256 and `write_completed` transport result, derived from the pinned
tuple and original root-parent observations. Failure diagnostics retain the same
snapshot. Write completion means delivery to the pipe, not independent proof of
child consumption; a failed write may have delivered only a prefix. Stdin errors
are refusals, retain stdout/stderr/exit diagnostics, and still run boundary checks.
No stdin hash is inserted into the unchanged H1 schema; future persisted evidence
owners must retain/check this returned snapshot alongside the input manifest.

The runner must print one H1 canonical preflight result with matching purpose,
actual candidate/manifest, exact inventory count/ID hash and original parent/root
proof. `fixtures/capture-child.js` is only a synthetic positive/negative process
control; it refuses validation purpose and does not execute the named H7 suites.
Injected `spawn`/`fs` dependencies are permitted only for synthetic controls, not
validation evidence. No capture here grants formal evidence or feature completion.

Inputs, runtime, cwd, original receipt parent, parent environment and the SAME
bigint original run-root-parent identity are checked before spawn, after child
completion (also spawn/stream/exit/signal/timeout failures), before publication,
after publication and after H1 readback. Run-root absence is checked at these
observation points. `write_attempts: 0` means no root presence was observed; this
is not a watcher and cannot detect a transient create/delete between observations.
No filesystem race elimination or dynamic-library closure is claimed.

Publication creates one new directory (0700) and exactly report/stdout/stderr
files (0400), using no-follow/exclusive opens, checked writes, file fsync,
directory fsync and H1 byte/identity/membership readback. Unsupported directory
fsync is recorded in returned/error `durabilityLimitations`; file flush failures
and other directory flush errors refuse publication. The manifest and snapshots
are returned separately, not added as a fourth receipt entry. Persisting and
independently checking them belongs to H5/H7.

Failures throw `CaptureError` with immutable `details.child`, `stdin`, `stdout`, `stderr`
base64/byte/hash snapshots, observed stream counts and all failed boundary checks.
Overflow retains each stream's bounded prefix and records truncation via observed
counts/failure; it never advertises full-stream hashes for omitted bytes. No
canonical success/result is fabricated for malformed or unsuccessful children,
so those failures have diagnostics but no H1 receipt. Partial publication trees
and independently created roots are never removed; retries need a new namespace.

## X-H3 Formal Worker Runner

`formal-registry.js` defines the version-1 canonical configuration, registry and
worker-result contracts. `formal-runner.js` consumes the two H2 receipts and
recaptures their actual input manifest before creating an absent run root. The
configuration and registry must be pinned H2 configuration files; every worker
module must be a declared source/fixture and own exactly its inventory cases.
Registry order is dispatch order. The exported schema key arrays specify exact
field order; the synthetic fixture/tests show a complete executable example.

```sh
node scripts/offset-curve-validation/run-formal-gates.js --run --config /absolute/config.json --run-dir /absolute/new-run --preflight-receipt /absolute/receipt-1 --preflight-receipt /absolute/receipt-2
node --test scripts/offset-curve-validation/test-*.js
```

Workers receive bounded canonical context on stdin and run as direct Node
children without a shell. Their explicit PASS result must contain the exact
ordered cases, per-case events and output inventory. Missing, undefined, unknown
or FAIL status is a refusal. Output files must be declared flat names, single-link
regular files with mode 0400; actual bounded bytes/hashes are read back with
no-follow opens and identity/membership checks. Actual child argv, environment,
streams, exit and stdin delivery are retained. Inputs, receipt parents and root
identities are rechecked around dispatch, including failed dispatch.

Configuration purpose is established through trusted filesystem I/O before any
injected adapter or hook can be accessed. Validation runs forbid injection;
synthetic runs accept only explicit own data-property hooks. The shipped child
fixture refuses validation purpose and provides no product-validation evidence.
H3 returns in-memory worker events and output snapshots, not a persisted joined
event ledger or independent evidence manifest. Direct-child timeout/stream bounds
are not process-tree containment, and observation-point checks do not eliminate
concurrent filesystem races. Failed runs retain diagnostics and partial trees.

## X-H4 Joined Ledger

`formal-ledger.js` captures its own actual configuration, H2 manifest, two H1
receipts and output snapshots after H3 completes. `buildJoinedLedger(h3, context)`
checks complete H3 worker/event/stream/output joins against that context and
derives an immutable genesis anchor plus one chained row per executed case.
`ledger-descriptors.js` exports only inert version-1 format data for later
independent checking; the production builder is not an independent oracle.

```js
const ledger = require('./formal-ledger');
const context = ledger.captureLedgerContext({configPath, receiptPaths});
const document = ledger.buildJoinedLedger(h3Result, context);
const saved = ledger.publishJoinedLedger(document, newLedgerPath);
ledger.revalidateJoinedLedger(saved);
```

The H4 source and descriptor must be declared actual H2 source inputs before
capturing receipts. Publication creates an absent directory outside H3's exact
worker-output root, with a single canonical mode-0400 ledger file. Exclusive
no-follow writes, file/directory flushes and original-identity readback protect
publication; failures preserve partial artifacts. Replay requires a freshly built
expected document, not the persisted ledger's own claimed hashes. Revalidation
checks original ledger, source, receipt and output bytes/metadata through final
boundaries. Unsupported directory fsync is recorded explicitly. These are
observation-point checks, not elimination of hostile filesystem races.

H4 is an explicit API, not automatic CLI persistence. H5 still owns the evidence
manifest and review dispatch; H7 owns independent checking. A failed H3 prefix
remains diagnostics and cannot become a complete PASS ledger.

## X-H5 Evidence And Automated Review

`p0f-evidence.js` binds an original H4 readback and a fresh actual H4 join to the
source/runtime/input manifest, both receipts, retained H3 diagnostics, preflight
transport snapshots, worker outputs and an explicitly pinned checker configuration.
Its format constants live in inert `evidence-descriptors.js`.

```js
const evidence = require('./p0f-evidence');
const context = evidence.captureEvidenceContext(request);
const ready = evidence.publishEvidenceManifest(context);
const reviewed = await evidence.runEvidenceChecker(ready);
evidence.revalidateEvidenceManifest(reviewed);
```

The exact request/config key order is exported by the descriptors. Original H4
issuer objects are mandatory; cloned payloads cannot grant publication. Declare
the actual H5 sources, checker source and checker configuration in H2's input
closure before capturing receipts. `runP0fEvidence(request)` combines the stages.
Publication creates an absent private directory outside H3/H4 membership roots,
with canonical mode-0400 manifest, diagnostics and transport files. Review retains
actual checker argv/environment/runtime/stdin/streams/exit and a bound envelope.
Original bytes, metadata, parents and membership are checked before and after
dispatch, including child failures and late output/readback substitution.

Validation forbids injected hooks and synthetic fixture checkers. The shipped
checker fixture refuses validation purpose. Automated checker execution is not
human approval or independent H7 qualification. Caller-supplied preflight stdin
snapshots cannot independently prove origin, delivery or child consumption; missing
snapshots are explicitly derived expectations, never fabricated observations.
These limits, direct-child containment limits and filesystem observation limits
are retained in evidence/review results. Failed artifacts are preserved.

## Remaining

`refusal-inventory.js` preserves all 71 original scenario identities and explicitly
separates executable obligations from pending H4/H5 integration. Run
`test-refusal-inventory.js` to exercise the current inventory against the real
transaction/CLI. Each executable identity has its own test; pending declarations
are reported as pending obligations, never counted as successful or skipped tests.
The current core has 93 obligations: 82 executable and 11 pending. Complete H6
qualification still requires those remaining cases and the H7 independent runs.
Typed stdout-shape and substituted-readback diagnoses distinguish these failures
from report parsing and on-disk receipt mutation. Repeated paths are refused by
the same request guard at both API and CLI ingress.

Remaining H6 integration and X-H7 still own completed orchestration, independent evidence
validation, adversarial case inventories
and deterministic reports. In particular, the schema layer does not establish
that a candidate/manifest digest names the current product source. Callers must
derive the expected tuple from actual inputs and validate those inputs at their
execution boundaries. A well-formed receipt or synthetic PASS is not a source
edit permission, independent review, or feature-completion claim.
