#!/usr/bin/env bash
# LibreCAD one-command GitHub release script.
#
# Usage:
#   scripts/release_github.sh <version> [stable|rc[N]] [options]
#
# Examples:
#   scripts/release_github.sh 2.2.1                # stable, tag v2.2.1
#   scripts/release_github.sh 2.2.2 rc             # tag 2.2.2_rc1
#   scripts/release_github.sh 2.2.2 rc3            # tag 2.2.2_rc3
#   scripts/release_github.sh 2.2.1 stable --dry-run
#
# Options:
#   --remote <name>     Git remote (default: origin).
#   --draft             Create the GitHub release as draft.
#   --no-push           Skip pushing branch and tag (implies --no-release).
#   --no-release        Skip 'gh release create' (CI ReleaseAssets job
#                       creates the release on its own).
#   --allow-dirty       Allow uncommitted/staged changes in the working tree
#                       (they will be folded into the release commit).
#   --dry-run           Print everything; don't modify anything.
#   --yes               Skip the final confirmation prompt.
#   -h, --help          Show this help.
#
# What it does:
#   1. Updates LC_VERSION in CMakeLists.txt and librecad/src/src.pro.
#   2. Commits the version bump (if any files changed) on the current branch.
#   3. Creates a signed (or annotated) tag and pushes the current branch
#      and the tag atomically. The tag push triggers
#      .github/workflows/build-all.yml which builds all platforms at the
#      tag and uploads named artifacts to the GitHub release.

set -euo pipefail

err()  { printf 'Error: %s\n'   "$*" >&2; }
warn() { printf 'Warning: %s\n' "$*" >&2; }
info() { printf '==> %s\n'      "$*"; }

print_usage() {
    sed -n '2,31p' "$0" | sed 's/^# \{0,1\}//'
    exit "${1:-0}"
}

DRY_RUN=false
run() {
    printf '+ %s\n' "$*"
    if [[ "$DRY_RUN" == true ]]; then return 0; fi
    "$@"
}

sed_inplace() {
    local expr="$1" file="$2"
    [[ -f "$file" ]] || return 0
    if sed --version >/dev/null 2>&1; then
        run sed -i -E -e "$expr" "$file"
    else
        run sed -i '' -E -e "$expr" "$file"
    fi
}

CREATED_TAG=""
PUSHED=false
on_exit() {
    local rc=$?
    if [[ $rc -ne 0 && -n "$CREATED_TAG" && "$DRY_RUN" != true ]]; then
        warn "Failed after creating tag $CREATED_TAG."
        if [[ "$PUSHED" == true ]]; then
            warn "Tag was pushed; remove from remote first:  git push --delete ${REMOTE} ${CREATED_TAG}"
        fi
        warn "Then remove local tag:  git tag -d ${CREATED_TAG}"
    fi
    exit $rc
}
trap on_exit EXIT

# ---- argument parsing -----------------------------------------------------

VERSION=""
RELEASE_TYPE="stable"
RC_NUMBER=""
REMOTE="origin"
DRAFT=false
DO_PUSH=true
DO_RELEASE=true
ALLOW_DIRTY=false
ASSUME_YES=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)     print_usage 0 ;;
        --dry-run)     DRY_RUN=true ;;
        --draft)       DRAFT=true ;;
        --no-push)     DO_PUSH=false ;;
        --no-release)  DO_RELEASE=false ;;
        --allow-dirty) ALLOW_DIRTY=true ;;
        --yes|-y)      ASSUME_YES=true ;;
        --remote)      REMOTE="${2:?--remote needs a value}"; shift ;;
        --remote=*)    REMOTE="${1#*=}" ;;
        -*)
            err "Unknown option: $1"; print_usage 1 ;;
        *)
            if [[ -z "$VERSION" ]]; then
                VERSION="$1"
            elif [[ "$RELEASE_TYPE" == "stable" ]]; then
                if [[ "$1" == "stable" ]]; then
                    :
                elif [[ "$1" =~ ^rc([0-9]*)$ ]]; then
                    RELEASE_TYPE="rc"
                    RC_NUMBER="${BASH_REMATCH[1]:-1}"
                else
                    err "Unexpected positional argument: $1"; print_usage 1
                fi
            else
                err "Unexpected positional argument: $1"; print_usage 1
            fi
            ;;
    esac
    shift
done

[[ -z "$VERSION" ]] && { err "Missing <version>"; print_usage 1; }

if ! [[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+(\.[0-9]+)?$ ]]; then
    err "Invalid version format '$VERSION'. Expected e.g. 2.2.1 or 2.2.1.4"
    exit 1
fi

# --no-push without --no-release would create a GitHub release referencing
# a tag that's only local; gh would auto-create the tag server-side at the
# default branch HEAD, so the local tag and remote tag could point at
# different commits. Force them to move together.
if [[ "$DO_PUSH" != true && "$DO_RELEASE" == true ]]; then
    info "--no-push implies --no-release; skipping 'gh release create'."
    DO_RELEASE=false
    if [[ "$DRAFT" == true ]]; then
        warn "--draft is ignored because --no-push implies --no-release."
    fi
fi

# ---- environment checks ---------------------------------------------------

for cmd in git gh; do
    command -v "$cmd" >/dev/null || { err "$cmd not found in PATH"; exit 1; }
done

if [[ "$DRY_RUN" != true && "$DO_RELEASE" == true ]] && ! gh auth status >/dev/null 2>&1; then
    err "gh not authenticated. Run 'gh auth login'."
    exit 1
fi

git rev-parse --is-inside-work-tree >/dev/null 2>&1 || {
    err "Not inside a git working tree."
    exit 1
}

REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "$REPO_ROOT"

# Verify the remote name resolves before we rely on `git ls-remote` failures
# being meaningful (otherwise "remote unreachable" is indistinguishable from
# "tag does not exist on remote").
if ! git remote get-url "$REMOTE" >/dev/null 2>&1; then
    err "Remote '${REMOTE}' is not configured. Use --remote <name> or 'git remote add'."
    exit 1
fi

if ! git diff --quiet || ! git diff --cached --quiet; then
    if [[ "$ALLOW_DIRTY" != true ]]; then
        err "Working tree has uncommitted/staged changes."
        err "Stash or commit them first, or pass --allow-dirty to fold them into the release commit."
        exit 1
    fi
    warn "Working tree has uncommitted changes; they will be included in the release commit."
    if [[ "$ASSUME_YES" != true ]]; then
        if [[ ! -t 0 ]]; then
            err "stdin is not a TTY; pass --yes to skip confirmation prompts."
            exit 1
        fi
        read -r -p "Continue anyway? [y/N] " ans
        [[ "$ans" =~ ^[Yy]$ ]] || exit 1
    fi
fi

BRANCH="$(git rev-parse --abbrev-ref HEAD)"
if [[ "$BRANCH" == "HEAD" ]]; then
    err "Detached HEAD; check out a branch first."
    exit 1
fi

# Allow GPG to prompt on TTY-less shells without dying under set -e.
GPG_TTY="$(tty 2>/dev/null || true)"
export GPG_TTY

# ---- derive tag (matches existing history: v2.2.1 / 2.2.1_rc1) ------------

if [[ "$RELEASE_TYPE" == "stable" ]]; then
    TAG="v${VERSION}"
    PRERELEASE_FLAG=()
    COMMIT_MSG="Release ${VERSION}"
    TAG_MSG="Release ${VERSION}"
else
    TAG="${VERSION}_rc${RC_NUMBER}"
    PRERELEASE_FLAG=(--prerelease)
    COMMIT_MSG="Release candidate ${VERSION} rc${RC_NUMBER}"
    TAG_MSG="$COMMIT_MSG"
fi
# Title must match what CI's softprops/action-gh-release@v2 will set (which
# is "LibreCAD <tag>"); otherwise CI overwrites the script's title later.
RELEASE_TITLE="LibreCAD ${TAG}"

info "Release type : ${RELEASE_TYPE}"
info "Version      : ${VERSION}"
info "Tag          : ${TAG}"
info "Branch       : ${BRANCH} -> ${REMOTE}"
info "Dry run      : ${DRY_RUN}"

# ---- duplicate-tag guard (local + remote + GitHub release) ---------------

if git rev-parse -q --verify "refs/tags/${TAG}" >/dev/null; then
    err "Tag ${TAG} already exists locally. Delete with: git tag -d ${TAG}"
    exit 1
fi

if [[ "$DRY_RUN" != true ]] && \
   git ls-remote --exit-code --tags "$REMOTE" "refs/tags/${TAG}" >/dev/null 2>&1; then
    err "Tag ${TAG} already exists on remote ${REMOTE}."
    err "Delete with: git push --delete ${REMOTE} ${TAG}"
    exit 1
fi

if [[ "$DO_RELEASE" == true && "$DRY_RUN" != true ]] && gh release view "$TAG" >/dev/null 2>&1; then
    err "GitHub release ${TAG} already exists. Delete with: gh release delete ${TAG}"
    exit 1
fi

# ---- final confirmation ---------------------------------------------------

if [[ "$ASSUME_YES" != true && "$DRY_RUN" != true ]]; then
    if [[ ! -t 0 ]]; then
        err "stdin is not a TTY; pass --yes to skip the confirmation prompt."
        exit 1
    fi
    read -r -p "Proceed with release ${TAG}? [y/N] " ans
    [[ "$ans" =~ ^[Yy]$ ]] || { info "Aborted by user."; exit 1; }
fi

# ---- update version files -------------------------------------------------

info "Updating version files"

# CMakeLists.txt: add_compile_definitions(LC_VERSION=2.2.x[_rc])
if [[ -f CMakeLists.txt ]]; then
    sed_inplace "s|(add_compile_definitions\\(LC_VERSION=)[^)]+\\)|\\1${VERSION}\\)|" CMakeLists.txt
fi

# librecad/src/src.pro: LC_VERSION="2.2.x"
if [[ -f librecad/src/src.pro ]]; then
    sed_inplace "s|^LC_VERSION=\"[^\"]*\"|LC_VERSION=\"${VERSION}\"|" librecad/src/src.pro
fi

# Stage exactly what we touched.
STAGED=()
[[ -f CMakeLists.txt ]] && STAGED+=("CMakeLists.txt")
[[ -f librecad/src/src.pro ]] && STAGED+=("librecad/src/src.pro")
if [[ ${#STAGED[@]} -gt 0 ]]; then
    run git add -- "${STAGED[@]}"
fi

# ---- commit (only if staged changes exist) --------------------------------

NEW_COMMIT=false
if [[ "$DRY_RUN" == true ]]; then
    info "(dry-run) would create commit if version files changed"
elif ! git diff --cached --quiet; then
    run git commit -s -m "$COMMIT_MSG"
    NEW_COMMIT=true
else
    info "Version files unchanged; tagging current HEAD without a new commit."
fi

# ---- tag ------------------------------------------------------------------
# When a signing key is configured we *require* a signed tag — releases must
# be signed and a silent fallback to an annotated tag would be wrong. When
# no key is configured we fall back to annotated.

HAVE_SIGNING_KEY=false
if git config --get user.signingkey >/dev/null 2>&1; then
    HAVE_SIGNING_KEY=true
fi

if [[ "$HAVE_SIGNING_KEY" == true ]]; then
    info "Creating signed tag (user.signingkey is configured)."
    run git tag -s "$TAG" -m "$TAG_MSG"
else
    warn "No signing key configured; creating annotated tag."
    run git tag -a "$TAG" -m "$TAG_MSG"
fi
[[ "$DRY_RUN" != true ]] && CREATED_TAG="$TAG"

# ---- push (atomic when there's a new commit) ------------------------------

if [[ "$DO_PUSH" != true ]]; then
    info "Skipping push (--no-push). To finish:"
    info "    git push --atomic ${REMOTE} ${BRANCH} ${TAG}"
else
    if [[ "$NEW_COMMIT" == true ]]; then
        run git push --atomic "$REMOTE" "${BRANCH}:${BRANCH}" "refs/tags/${TAG}"
    else
        run git push "$REMOTE" "refs/tags/${TAG}"
    fi
    [[ "$DRY_RUN" != true ]] && PUSHED=true
    info "Pushed tag ${TAG} to ${REMOTE}."
fi

# ---- GitHub release stub --------------------------------------------------

if [[ "$DO_RELEASE" != true ]]; then
    info "Skipping 'gh release create' (--no-release). CI ReleaseAssets job will publish."
else
    DRAFT_FLAG=()
    [[ "$DRAFT" == true ]] && DRAFT_FLAG+=(--draft)

    run gh release create "$TAG" \
        --title "$RELEASE_TITLE" \
        --generate-notes \
        ${PRERELEASE_FLAG[@]+"${PRERELEASE_FLAG[@]}"} \
        ${DRAFT_FLAG[@]+"${DRAFT_FLAG[@]}"}
    info "GitHub release: https://github.com/LibreCAD/LibreCAD/releases/tag/${TAG}"
fi

info "Done. Watch CI: gh run list --workflow build-all.yml --limit 5"

trap - EXIT
