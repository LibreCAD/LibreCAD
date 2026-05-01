#!/bin/bash
# Local snap build (destructive mode, no LXD/Multipass).
# CI uses canonical/action-build@v1 instead; see .github/workflows/build-all.yml.

sudo snap install snapcraft --classic

sudo snap run snapcraft --destructive-mode

sudo snap remove librecad --purge
#sudo snap install --dangerous librecad_2.2.1-rc_amd64.snap --jailmode
