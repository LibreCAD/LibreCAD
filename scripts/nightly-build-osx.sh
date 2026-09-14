#!/bin/bash

# ********************************************************************************
# This file is part of the LibreCAD project, a 2D CAD program
#
# Copyright (C) 2011-2026 LibreCAD.org
# Copyright (C) 2011-2026 Dongxu Li (github.com/dxli)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
# USA.
# ********************************************************************************

set -euo pipefail

# scripts for nightly builds in OS/X
# 
# 1 checkout for new HEAD in git repository
# 2 build and upload generated DMG files up to 10 DMGs at sf.net
# 3 compute checksums, and upload README.md

if [[ $(date -u +%H) != 03 ]]
then
	exit 0
fi

#nightly librecad build

TOP_DIR=~/hd2
SF_URL='dongxuli@frs.sf.net:/home/frs/project/librecad/OSX/NightlyBuilds/'
#log dir
LOG_DIR=$TOP_DIR/log-nightly
[[ -d $LOG_DIR ]] || mkdir -p $LOG_DIR
LOG_ERR=$LOG_DIR/err.log
LOG_MSG=$LOG_DIR/msg.log
#local repository
LC_DIR=$TOP_DIR/LibreCAD
if [ ! -d $LC_DIR ]
then
	echo "Error: LibreCAD source repository $LC_DIR does not exist" >> $LOG_MSG
	exit 1
fi

#file keeping current HEAD commit number
LC_HEAD=$LOG_DIR/lc_HEAD_number.tmp
#git HEAD
LC_GIT_HEAD=$LC_DIR/.git/refs/heads/master
#dmg built
LC_DMG=$LC_DIR/LibreCAD.dmg
#folder holding dmg files
LC_SF_NIGHTLY=$TOP_DIR/sf-OSX-Nightly
[[ -d $LC_SF_NIGHTLY ]] || mkdir -p $LC_SF_NIGHTLY

cd "$LC_DIR"

#update git
git fetch origin
git reset --hard origin/master
echo "$(date): repository updated" >> $LOG_MSG

#detect git updates
if [ -f $LC_HEAD ]
then
	if cmp $LC_HEAD $LC_GIT_HEAD
	then
		echo "$(date): No git update, exiting" >> $LOG_ERR
		exit 0
	fi
fi

#save git HEAD
cp -v $LC_GIT_HEAD $LC_HEAD
#dmg to save to
LC_HEAD_NUM=$(cat $LC_HEAD)
DATE_STR="$(date -u '+%F')"
COMMIT_STR=${LC_HEAD_NUM:0:8}
LC_DMG_SF=LibreCAD-${DATE_STR}-${COMMIT_STR}.dmg
echo "$LC_DMG_SF"

# Remove every prior candidate so a failed build cannot upload a stale image.
rm -f "$LC_DMG" "$LC_DMG.sha256" "$LC_DMG.verification.txt" \
    "$LC_DMG.app.manifest"
#build DMG
./scripts/build-osx.sh -p="" -q="" >> "$LOG_ERR" 2>&1
if ! [ -f "$LC_DMG" ] || ! [ -f "$LC_DMG.sha256" ]
then
	echo "$(date): building failed, exiting" >> $LOG_ERR
	exit 1
fi
if ! shasum -a 256 -c "$LC_DMG.sha256" >> "$LOG_ERR" 2>&1
then
	echo "$(date): generated DMG checksum failed, exiting" >> "$LOG_ERR"
	exit 1
fi
if ! ./scripts/verify-osx-package.sh --dmg "$LC_DMG" \
    --source-app "$LC_DIR/LibreCAD.app" --mode adhoc >> "$LOG_ERR" 2>&1
then
	echo "$(date): generated DMG verification failed, exiting" >> "$LOG_ERR"
	exit 1
fi

#copy to local folder
mkdir -p "$LC_SF_NIGHTLY"
find "$LC_SF_NIGHTLY" -maxdepth 1 -type f -name '*.dmg' -print |
    LC_ALL=C sort -r | sed -n '10,$p' | while IFS= read -r old_dmg
do
	rm -f "$old_dmg" "$old_dmg.sha256"
done
PUBLISHED_DMG="$LC_SF_NIGHTLY/$LC_DMG_SF"
cp -v "$LC_DMG" "$PUBLISHED_DMG"
DMG_DIGEST=$(awk 'NR == 1 { print $1 }' "$LC_DMG.sha256")
printf '%s  %s\n' "$DMG_DIGEST" "$LC_DMG_SF" > "$PUBLISHED_DMG.sha256"

#copy to sf.net
if ! rsync -e ssh --delete -Pac "$LC_SF_NIGHTLY/" "$SF_URL"
then
	echo "$(date): failed to upload $LC_DMG_SF" >> $LOG_ERR
	exit 1
fi

#README.md
README_FILE=$TOP_DIR/README.md
sed -e "s:DATE_PLACE_HOLDER:${DATE_STR}:g" \
-e "s:COMMIT_PLACE_HOLDER:${COMMIT_STR}:g" \
"$TOP_DIR/README.md.template" > "$README_FILE"

shopt -s nullglob
for dfile in "$LC_SF_NIGHTLY"/*.dmg
do
    dmg_name=$(basename "$dfile")
    {
        printf '%s\n' '- - -'
        printf '[%s](http://sourceforge.net/projects/librecad/files/OSX/Nightly/%s/download/)\n' "$dmg_name" "$dmg_name"
        printf '\nMD5: %s\n' "$(md5 -r "$dfile" | awk '{print $1}')"
        printf '\nSHA1: %s\n' "$(shasum "$dfile" | awk '{print $1}')"
        printf '\nSHA256: %s\n' "$(shasum -a 256 "$dfile" | awk '{print $1}')"
    } >> "$README_FILE"
done
shopt -u nullglob

rsync -e ssh -Pac "$README_FILE" "$SF_URL"

echo "$(date): $LC_DMG_SF uploaded" >> $LOG_MSG
		
