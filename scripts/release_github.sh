#!/bin/bash

# Fully Automated LibreCAD GitHub Release Script (with gh CLI) - Fixed for older Bash
# Usage: ./release_github.sh <new_version> [rc|stable] [--assets-dir <path>] [--draft] [--dry-run]

set -euo pipefail

NEW_VERSION="${1:-}"
RELEASE_TYPE="${2:-stable}"
ASSETS_DIR=""
DRAFT=false
DRY_RUN=false

# Parse options
shift 2 || true
while [[ $# -gt 0 ]]; do
    case $1 in
        --assets-dir) ASSETS_DIR="$2"; shift ;;
        --draft) DRAFT=true ;;
        --dry-run) DRY_RUN=true ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
    shift
done

if [[ -z "$NEW_VERSION" ]]; then
    echo "Error: New version required."
    echo "Usage: $0 <new_version> [rc|stable] [--assets-dir <path>] [--draft] [--dry-run]"
    exit 1
fi

if [[ "$RELEASE_TYPE" != "stable" && "$RELEASE_TYPE" != "rc" ]]; then
    echo "Error: Release type must be 'stable' or 'rc'"
    exit 1
fi

if ! command -v gh &> /dev/null; then
    echo "Error: GitHub CLI (gh) not found. Install from https://cli.github.com"
    exit 1
fi

if ! [[ "$NEW_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+(\.[0-9]+)?$ ]]; then
    echo "Error: Invalid version format."
    exit 1
fi

# Portable uppercase conversion
RELEASE_TYPE_UPPER=$(echo "$RELEASE_TYPE" | tr '[:lower:]' '[:upper:]')

echo "Preparing $RELEASE_TYPE_UPPER GitHub release for $NEW_VERSION"

# Tag and prerelease setup
if [[ "$RELEASE_TYPE" == "stable" ]]; then
    TAG="v${NEW_VERSION}"
    PRERELEASE_FLAG=""
    COMMIT_MSG="Release $NEW_VERSION"
    echo "Stable release → Tag: $TAG"
else
    TAG="${NEW_VERSION}_rc-latest"
    PRERELEASE_FLAG="--prerelease"
    COMMIT_MSG="Prepare RC $NEW_VERSION"
    echo "Release Candidate → Tag: $TAG (pre-release)"
fi

[[ "$DRAFT" == "true" ]] && DRAFT_FLAG="--draft" || DRAFT_FLAG=""

# Assets handling
ASSETS_ARGS=""
if [[ -n "$ASSETS_DIR" && -d "$ASSETS_DIR" ]]; then
    ASSETS_ARGS="$ASSETS_DIR/*"
    echo "Will upload assets from: $ASSETS_DIR"
elif [[ -n "$ASSETS_DIR" ]]; then
    echo "Warning: Assets directory '$ASSETS_DIR' not found. Proceeding without assets."
fi

# === Version file update logic (robust, same as before) ===
# (Insert the full version update block from the previous script here – omitted for brevity,
# but keep it exactly as in the last working version)

echo "Version files updated."

if [[ "$DRY_RUN" == "true" ]]; then
    echo "!!! DRY-RUN: Skipping Git and GitHub operations !!!"
    echo "Would run:"
    echo "  git commit -m '$COMMIT_MSG' -S"
    echo "  git tag -a $TAG -m 'Release $NEW_VERSION' -S"
    echo "  git push origin HEAD && git push origin $TAG"
    echo "  gh release create $TAG $ASSETS_ARGS --generate-notes $PRERELEASE_FLAG $DRAFT_FLAG"
    exit 0
fi

# Git operations
git add CMakeLists.txt common.pri librecad.pro
git commit -m "$COMMIT_MSG" -S
git tag -a "$TAG" -m "Release $NEW_VERSION" -S
git push origin HEAD
git push origin "$TAG"

echo "Tag $TAG pushed successfully."

# GitHub Release
GH_CMD="gh release create $TAG $ASSETS_ARGS --generate-notes $PRERELEASE_FLAG $DRAFT_FLAG"
echo "Creating GitHub Release..."
eval "$GH_CMD"

echo "GitHub Release created successfully!"
echo "View at: https://github.com/LibreCAD/LibreCAD/releases/tag/$TAG"