#!/bin/bash -x
# scripts for nightly builds in OS/X
# 
# 1 checkout for new HEAD in git repository
# 2 build and upload generated DMG files up to 10 DMGs at sf.net
# 3 compute checksums, and upload README.md

if [ ! $(expr $(date -u +%H)) == 3 ]
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

cd $LC_DIR

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
echo $LC_DMG_SF

#remove existing DMG
rm -f $LC_DMG
#build DMG
./scripts/build-osx.sh -p="" -q="" 2>&1 >> $LOG_ERR
if ! [ -f $LC_DMG ]
then
	echo "$(date): building failed, exiting" >> $LOG_ERR
	exit 1
fi

#copy to local folder
mkdir -p $LC_SF_NIGHTLY
pushd $LC_SF_NIGHTLY
for f in $(ls -rv|sed -e '1,9 d')
do
	rm -f $f
done

popd 
cp -v $LC_DMG $LC_SF_NIGHTLY/$LC_DMG_SF

#copy to sf.net
rsync -e ssh --delete -Pac $LC_SF_NIGHTLY/ "$SF_URL"
if [ $? -ne 0 ]
then
	echo "$(date): failed to upload $LC_DMG_SF" >> $LOG_ERR
	exit -1
fi

#README.md
cd ..
README_FILE=$TOP_DIR/README.md
cp $TOP_DIR/README.md.template $README_FILE
cat $TOP_DIR/README.md.template | \
sed -e "s:DATE_PLACE_HOLDER:${DATE_STR}:g" \
-e "s:COMMIT_PLACE_HOLDER:${COMMIT_STR}:g" \
> $README_FILE

pushd $LC_SF_NIGHTLY
for dfile in *.dmg
do
echo -e "- - -\n[$dfile](http://sourceforge.net/projects/librecad/files/OSX/Nightly/$dfile/download/)" >> $README_FILE
echo -e "\nMD5: $(md5 -r $dfile|awk '{print $1}')">> $README_FILE
echo -e "\nSHA1: $(shasum $dfile|awk '{print $1}')">> $README_FILE
echo -e "\nSHA256: $(shasum -a 256 $dfile|awk '{print $1}')">> $README_FILE
done
popd

rsync -e ssh -Pac $README_FILE "$SF_URL"

echo "$(date): $LC_DMG_SF uploaded" >> $LOG_MSG
		
