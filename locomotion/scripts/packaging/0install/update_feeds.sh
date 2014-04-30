#!/bin/sh

csdir=`cat Jamfile | grep "^TOP ?=" | sed -e 's/.*"\(.*\)".*/\1/'`
csbindir=$csdir/bin

fetch_rcsrev()
{
  if svn info $csdir 2> /dev/null ; then
    # Use rcsrev script info source dir is svn working copy
    csver_svnrev=`$csbindir/rcsrev print`
  else
    # Otherwise, csver.h extraction
    csver_svnrev=`cat $csdir/include/csver.h | grep "^#define *CS_VERSION_RCSREV" | sed -e "s/[^0-9]*//"`
  fi
  echo $csver_svnrev
}

csver_major=`cat $csdir/include/csver.h | grep "^#define *CS_VERSION_NUM_MAJOR" | sed -e "s/[^0-9]*//"`
csver_minor=`cat $csdir/include/csver.h | grep "^#define *CS_VERSION_NUM_MINOR" | sed -e "s/[^0-9]*//"`
csver_rel=`cat $csdir/include/csver.h | grep "^#define *CS_VERSION_NUM_RELEASE" | sed -e "s/[^0-9]*//"`
csver_svnrev=`fetch_rcsrev`
CSVER=$csver_major.$csver_minor.$csver_rel.$csver_svnrev
FEEDVER=$csver_major.$csver_minor

create_archive()
{
  feedprefix=$1
  shift
  lists=$*
  
  archive=$feedprefix-$CSVER
  $csbindir/archive-from-lists.sh $archive $lists
}

_feedpath()
{
  filename=$1
  echo $csdir/scripts/packaging/0install/$filename
}

_update_feed()
{
  feedprefix=$1
  arch=$2
  download_dir=$3
  
  archive=$feedprefix-$CSVER
  archive_url=http://crystalspace3d.org/downloads/binary/$FEEDVER/${download_dir}$archive.tar.xz
  feedname=$feedprefix-$FEEDVER
  feedpath=`_feedpath $feedname.xml`
  
  if [ -e $feedpath ] ; then
    0publish -u $feedpath
    0publish	\
      --add-version=$CSVER	\
      --archive-url=$archive_url	\
      --archive-file=$archive.tar.xz	\
      --archive-extract=$archive	\
      --set-arch=$arch	\
      --set-stability=testing	\
      --set-released=`date +%Y-%m-%d`	\
      $feedpath
  else
    0publish -c $feedpath
    0publish	\
      --set-version=$CSVER	\
      --archive-url=$archive_url	\
      --archive-file=$archive.tar.xz	\
      --archive-extract=$archive	\
      --set-arch=$arch	\
      --set-stability=testing	\
      --set-released=`date +%Y-%m-%d`	\
      $feedpath
    echo "* A new 0install feed was created; you will have to adjust it's contents"
  fi
    
  echo "* Upload $archive.tar.lzma to $archive_url ."
  echo "* Don't forget to re-sign the feed file!"
  echo "     0publish -x -k <gpgkey> $feedpath"
}

update_feed()
{
  arch=`uname`-`uname -m`
  arch_lc=`echo $arch | tr "[:upper:]" "[:lower:]"`
  _update_feed $1 $arch $arch_lc/
}

update_feed_neutral()
{
  _update_feed $1 "*-*" ""
}

fixup_feed()
{
  feedprefix=$1
  feedname=$feedprefix-$FEEDVER
  feedpath=`_feedpath $feedname.xml`
  
  # The *-sdk* feeds use a self-dependency to set the CRYSTAL_1_4 env var
  # 0publish doesn't have a command for that, so inject manually
  cat $feedpath | sed -e "s@\(<implementation[^>]*arch=\"$arch\"[^>]*version=\"$CSVER\">\)@\1\n\
      <requires interface=\"http://crystalspace3d.org/0install/$feedname.xml\">\n\
	    <environment insert=\"\" mode=\"prepend\" name=\"CRYSTAL_${csver_major}_${csver_minor}\"/>\n\
      </requires>@g" > $feedpath.tmp
  mv $feedpath.tmp $feedpath
}

jam filelists

create_archive crystalspace-libs libs-shared@lib
# SDKs: we need to lump everything together
# splitting things over multiple feeds works, but is less robust
# (e.g. cs-config won't find the libs dir when invoked w/o 0launch)
SDK_LISTS="libs-static@lib libs-shared@lib cs-config@bin bin-tool@bin headers@include headers-platform@include"
create_archive crystalspace-sdk $SDK_LISTS
create_archive crystalspace-sdk-staticplugins $SDK_LISTS libs-staticplugins@lib
create_archive crystalspace-data data-runtime@data vfs
create_archive crystalspace-docs-manual doc-manual doc-util-open

update_feed crystalspace-libs
update_feed crystalspace-sdk
fixup_feed crystalspace-sdk
update_feed crystalspace-sdk-staticplugins
fixup_feed crystalspace-sdk-staticplugins
update_feed_neutral crystalspace-data
update_feed_neutral crystalspace-docs-manual
