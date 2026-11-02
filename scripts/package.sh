#!/bin/bash

set -ex

srcdir=`realpath $1`

version="2.3.3"

if [ -n "$BYGFOOT_VERSION" ]; then
    version=$BYGFOOT_VERSION
fi

builddir=./build-$version
tarname=bygfoot-$version

if [ -n "$2" ]; then
    installdir=$2
    tarname=$2
fi

installdir=$tarname
installdir_abs=`realpath $installdir`

mkdir -p $builddir
mkdir -p $installdir

# Don't use the -B option here, because some older versions of
# cmake don't support it.
pushd $builddir
if [ "$BYGFOOT_MINGW" -eq 1 ]; then
    git clone https://github.com/json-c/json-c
    pushd json-c
    git checkout json-c-0.16-20220414
    mkdir -p install
    json_c_installdir=`pwd`/install
    mkdir -p build
    pushd build
    cmake -G Ninja -S .. -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32/sys-root/ -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=$json_c_installdir
    ninja install
    popd
    popd

    cmake -G Ninja  -S $srcdir -DCMAKE_INSTALL_PREFIX=$installdir_abs -DCMAKE_C_FLAGS=-DVERS=\"\\\"$version\\\"\" -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32/sys-root/ -DJSON-C_DIR=$json_c_installdir/lib/cmake/json-c/ -DZLIB_LIBRARY=/usr/x86_64-w64-mingw32/sys-root/mingw/lib/libz.dll.a -DZLIB_INCLUDE_DIR=/usr/x86_64-w64-mingw32/sys-root/mingw/include/ -DPKG_CONFIG_EXECUTABLE=/usr/bin/mingw64-pkg-config -DJSON-C_LIBRARIES=json-c::json-c-static
else
    cmake -G Ninja  -S $srcdir -DCMAKE_INSTALL_PREFIX=$installdir_abs -DCMAKE_C_FLAGS=-DVERS=\"\\\"$version\\\"\"
fi
popd

ninja -v -C $builddir install


# Fixup install dir so that bygfoot can find the support
# files.

mv $installdir/bin/bygfoot* $installdir/
rmdir $installdir/bin

mv $installdir/share/bygfoot/support_files $installdir

if [ -d $installdir/share/locale ]; then
    mv $installdir/share/locale $installdir
fi
rmdir $installdir/share/bygfoot
rmdir $installdir/share

# The original package script did this, so lets do it too.
for f in AUTHORS COPYING ChangeLog INSTALL README TODO UPDATE ReleaseNotes; do
    cp $srcdir/$f $installdir/
done

tar -cjf $tarname.tar.bz2 $installdir

