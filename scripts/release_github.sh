#!/bin/bash -x
# Fully Automated LibreCAD GitHub Release Script (with gh CLI)
# Usage: ./release_github.sh <new_version> [rc|stable] [--assets-dir <path>] [--draft] [--dry-run]
#
# Features:
# - Validates inputs and environment
# - Checks for existing tag/release
# - Updates version files (CMakeLists.txt, common.pri, librecad.pro)
# - Portable and shellcheck-compliant
# - Idempotent operations
# - Detailed error handling with reasons and fix suggestions
# - Allow release even if no new commits (skip empty commit)
# - Always show commands before running
# - Dry-run mode for testing

set -euo pipefail

# Function to print usage
print_usage() {
    echo "Usage: $0 <new_version> [rc|stable] [--assets-dir <path>] [--draft] [--dry-run]"
    echo "Example: $0 2.2.1 stable --assets-dir generated --draft"
    exit 1
}

# Function to run command with echo (and execute unless dry-run)
run_cmd() {
    local cmd="$1"
    echo "Executing: $cmd"
    if [[ "$DRY_RUN" == true ]]; then
        echo "  (Skipped due to dry-run)"
        return 0
    fi
    eval "$cmd" || {
        local err=$?
        echo "Error: Command failed with exit code $err"
        echo "Suggestion: Check the command output above for details. Ensure files exist and permissions are correct."
        exit $err
    }
}

# Parse arguments
NEW_VERSION="${1:-}"
if [[ -z "$NEW_VERSION" ]]; then
    print_usage
fi
RELEASE_TYPE="${2:-stable}"
shift 2 || true

ASSETS_DIR=""
DRAFT=false
DRY_RUN=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --assets-dir)
            ASSETS_DIR="$2"
            shift
            ;;
        --draft)
            DRAFT=true
            ;;
        --dry-run)
            DRY_RUN=true
            ;;
        *)
            echo "Unknown option: $1"
            print_usage
            ;;
    esac
    shift
done

export GPG_TTY=$(tty)

# Validate version format (e.g., 2.2.1 or 2.2.1.1)
if ! [[ "$NEW_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+(\.[0-9]+)?$ ]]; then
    echo "Error: Invalid version format. Must be like 2.2.1 or 2.2.1.1"
    exit 1
fi

# Validate release type
if [[ "$RELEASE_TYPE" != "stable" && "$RELEASE_TYPE" != "rc" ]]; then
    echo "Error: Release type must be 'stable' or 'rc'"
    exit 1
fi

# Check prerequisites
if ! command -v gh &> /dev/null; then
    echo "Error: GitHub CLI (gh) not found."
    echo "Suggestion: Install from https://cli.github.com or your package manager (e.g., brew install gh)."
    exit 1
fi

if ! command -v git &> /dev/null; then
    echo "Error: git not found."
    echo "Suggestion: Install git from https://git-scm.com/downloads or your package manager."
    exit 1
fi

# Check if gh is authenticated
run_cmd "gh auth status" || {
    echo "Error: gh not authenticated."
    echo "Suggestion: Run 'gh auth login' and follow the prompts."
    exit 1
}

# Uppercase release type (portable)
RELEASE_TYPE_UPPER=$(echo "$RELEASE_TYPE" | tr '[:lower:]' '[:upper:]')
echo "Preparing $RELEASE_TYPE_UPPER GitHub release for $NEW_VERSION"

# Determine tag
if [[ "$RELEASE_TYPE" == "stable" ]]; then
    TAG="v${NEW_VERSION}"
    PRERELEASE_FLAG=""
    COMMIT_MSG="Release $NEW_VERSION"
else
    TAG="${NEW_VERSION}-rc"
    PRERELEASE_FLAG="--prerelease"
    COMMIT_MSG="Prepare RC $NEW_VERSION"
fi

# Check if tag already exists
if git tag -l | grep -q "^${TAG}$"; then
    echo "Error: Tag $TAG already exists."
    echo "Suggestion: Delete it with 'git tag -d $TAG' and 'git push --delete origin $TAG' if needed, or choose a different version."
    exit 1
fi

# Check if release already exists (using gh)
if gh release view "$TAG" &> /dev/null; then
    echo "Error: Release for $TAG already exists on GitHub."
    echo "Suggestion: Delete it via gh: 'gh release delete $TAG' or from the GitHub web interface."
    exit 1
fi

# Assets setup
ASSETS_ARGS=""
if [[ -n "$ASSETS_DIR" ]]; then
    if [[ ! -d "$ASSETS_DIR" ]]; then
        echo "Error: Assets directory '$ASSETS_DIR' not found."
        echo "Suggestion: Ensure the directory exists and contains files to upload."
        exit 1
    fi
    ASSETS_ARGS="$ASSETS_DIR/*"
    echo "Uploading assets from: $ASSETS_DIR"
fi

DRAFT_FLAG=""
if [[ "$DRAFT" == true ]]; then
    DRAFT_FLAG="--draft"
fi

# === Version file updates ===
echo "Updating version files..."

# Update CMakeLists.txt (example: set(LIBRECAD_VERSION "${NEW_VERSION}"))
if [[ -f CMakeLists.txt ]]; then
    run_cmd "sed -i.bak \"s/set(LIBRECAD_VERSION \".*\"/set(LIBRECAD_VERSION \"${NEW_VERSION}\"/\" CMakeLists.txt"
    run_cmd "rm -f CMakeLists.txt.bak"
    echo "Updated CMakeLists.txt"
else
    echo "Warning: CMakeLists.txt not found - skipping."
fi

# Update common.pri (example: DEFINES += LC_VERSION="\\\"2.2.x\\\"")
if [[ -f common.pri ]]; then
    run_cmd "sed -i.bak \"s/LC_VERSION=\\\".*\\\"/LC_VERSION=\\\"${NEW_VERSION}\\\"/\" common.pri"
    run_cmd "rm -f common.pri.bak"
    echo "Updated common.pri"
else
    echo "Warning: common.pri not found - skipping."
fi

# Update librecad.pro (example: VERSION = 2.2.x)
if [[ -f librecad.pro ]]; then
    run_cmd "sed -i.bak \"s/VERSION = .*/VERSION = ${NEW_VERSION}/\" librecad.pro"
    run_cmd "rm -f librecad.pro.bak"
    echo "Updated librecad.pro"
else
    echo "Warning: librecad.pro not found - skipping."
fi

echo "Version files updated."

# Git operations
run_cmd "git add CMakeLists.txt common.pri librecad.pro || true"  # Ignore if no changes

# Commit only if there are changes (allow no-commit releases)
if git diff --cached --quiet; then
    echo "No changes to commit - proceeding without new commit."
else
    run_cmd "git commit -m \"$COMMIT_MSG\" -s || true"  # true to continue if empty
fi

run_cmd "git tag -a \"$TAG\" -m \"Release $NEW_VERSION\" -s"
run_cmd "git push origin HEAD"
run_cmd "git push origin \"$TAG\""

echo "Tag $TAG pushed successfully."

# Create GitHub release
GH_CMD="gh release create \"$TAG\" $ASSETS_ARGS --generate-notes $PRERELEASE_FLAG $DRAFT_FLAG"
run_cmd "$GH_CMD"

echo "GitHub Release created successfully!"
echo "View at: https://github.com/LibreCAD/LibreCAD/releases/tag/$TAG"
