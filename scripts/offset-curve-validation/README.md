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

## Remaining

X-H2 through X-H7 still own actual source/runtime capture, worker dispatch,
persisted joins, independent evidence validation, adversarial case inventories
and deterministic reports. In particular, the schema layer does not establish
that a candidate/manifest digest names the current product source. Callers must
derive the expected tuple from actual inputs and validate those inputs at their
execution boundaries. A well-formed receipt or synthetic PASS is not a source
edit permission, independent review, or feature-completion claim.
