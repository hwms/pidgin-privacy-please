# Introduction #

This is a very unexplained list of commands that will get the plugin built against pidgin 2.8.0

# Commands #

```
sudo apt-get install mingw32 mingw32-binutils mingw32-runtime nsis

export PIDGIN_VERSION=2.8.0
export PIDGIN_PP_VERSION=0.7.0
export PIDGIN_DEV_ROOT=/tmp/build/
export SOURCES_DIR=${PIDGIN_DEV_ROOT}/sources/
export DEV_DIR=${PIDGIN_DEV_ROOT}/win32-dev
export MINGW_DIR=${DEV_DIR}/mingw
export PIDGIN_DIR=${PIDGIN_DEV_ROOT}/pidgin-${PIDGIN_VERSION}

mkdir -p ${SOURCES_DIR}
mkdir -p ${MINGW_DIR}

cd ${SOURCES_DIR}
wget -nv http://sourceforge.net/projects/mingw/files/MinGW/BaseSystem/GNU-Binutils/binutils-2.20/binutils-2.20-1-mingw32-bin.tar.gz
wget -nv http://sourceforge.net/projects/mingw/files/MinGW/BaseSystem/RuntimeLibrary/MinGW-RT/mingwrt-3.17/mingwrt-3.17-mingw32-dev.tar.gz
wget -nv http://sourceforge.net/projects/mingw/files/MinGW/BaseSystem/RuntimeLibrary/MinGW-RT/mingwrt-3.17/mingwrt-3.17-mingw32-dll.tar.gz
wget -nv http://sourceforge.net/projects/mingw/files/MinGW/BaseSystem/RuntimeLibrary/Win32-API/w32api-3.14/w32api-3.14-mingw32-dev.tar.gz
wget -nv http://sourceforge.net/projects/mingw/files/MinGW/BaseSystem/GCC/Version4/Previous%20Release%20gcc-4.4.0/gmp-4.2.4-mingw32-dll.tar.gz
wget -nv http://sourceforge.net/projects/mingw/files/MinGW/libiconv/libiconv-1.13.1-1/libiconv-1.13.1-1-mingw32-dll-2.tar.lzma
wget -nv http://sourceforge.net/projects/mingw/files/MinGW/BaseSystem/GCC/Version4/Previous%20Release%20gcc-4.4.0/mpfr-2.4.1-mingw32-dll.tar.gz
wget -nv http://sourceforge.net/projects/mingw/files/MinGW/BaseSystem/GCC/Version4/Previous%20Release%20gcc-4.4.0/pthreads-w32-2.8.0-mingw32-dll.tar.gz
wget -nv http://sourceforge.net/projects/mingw/files/MinGW/BaseSystem/GCC/Version4/Previous%20Release%20gcc-4.4.0/gcc-core-4.4.0-mingw32-bin.tar.gz
wget -nv http://sourceforge.net/projects/mingw/files/MinGW/BaseSystem/GCC/Version4/Previous%20Release%20gcc-4.4.0/gcc-core-4.4.0-mingw32-dll.tar.gz

cd ${MINGW_DIR}
for file in ${SOURCES_DIR}/*tar.gz ; do tar xzvf ${file} ; done
tar xvf ${SOURCES_DIR}/libiconv-1.13.1-1-mingw32-dll-2.tar.lzma

cd ${SOURCES_DIR}
wget -nv http://ftp.gnome.org/pub/gnome/binaries/win32/gtk+/2.14/gtk+-bundle_2.14.7-20090119_win32.zip
wget -nv http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/gettext-tools-0.17.zip
wget -nv http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/gettext-runtime-0.17-1.zip
wget -nv http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/libxml2-dev_2.7.4-1_win32.zip
wget -nv http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/libxml2_2.7.4-1_win32.zip
wget -nv http://developer.pidgin.im/static/win32/tcl-8.4.5.tar.gz
wget -nv http://developer.pidgin.im/static/win32/gtkspell-2.0.16.tar.bz2
wget -nv http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/enchant_1.5.0-2_win32.zip
wget -nv http://developer.pidgin.im/static/win32/nss-3.12.5-nspr-4.8.2.tar.gz
wget -nv http://developer.pidgin.im/static/win32/silc-toolkit-1.1.8.tar.gz
wget -nv http://developer.pidgin.im/static/win32/meanwhile-1.0.2_daa2-win32.zip
wget -nv http://developer.pidgin.im/static/win32/cyrus-sasl-2.1.22-daa1.zip
wget -nv http://ftp.acc.umu.se/pub/GNOME/binaries/win32/intltool/0.40/intltool_0.40.4-1_win32.zip
wget -nv http://prdownloads.sourceforge.net/pidgin/pidgin-${PIDGIN_VERSION}.tar.bz2

unzip ${SOURCES_DIR}/gtk+-bundle_2.14.7-20090119_win32.zip -d ${DEV_DIR}/gtk_2_0-2.14
unzip ${SOURCES_DIR}/gettext-tools-0.17.zip -d ${DEV_DIR}/gettext-0.17
unzip ${SOURCES_DIR}/gettext-runtime-0.17-1.zip -d ${DEV_DIR}/gettext-0.17
unzip ${SOURCES_DIR}/libxml2-dev_2.7.4-1_win32.zip -d ${DEV_DIR}/libxml2-2.7.4
unzip ${SOURCES_DIR}/libxml2_2.7.4-1_win32.zip -d ${DEV_DIR}/libxml2-2.7.4
unzip ${SOURCES_DIR}/enchant_1.5.0-2_win32.zip -d ${DEV_DIR}/enchant_1.5.0-2_win32
unzip ${SOURCES_DIR}/meanwhile-1.0.2_daa2-win32.zip -d ${DEV_DIR}
unzip ${SOURCES_DIR}/cyrus-sasl-2.1.22-daa1.zip -d ${DEV_DIR}
unzip ${SOURCES_DIR}/intltool_0.40.4-1_win32.zip -d ${DEV_DIR}/intltool_0.40.4-1_win32

cd ${DEV_DIR}
tar xzvf ${SOURCES_DIR}/tcl-8.4.5.tar.gz
tar xjvf ${SOURCES_DIR}/gtkspell-2.0.16.tar.bz2
tar xzvf ${SOURCES_DIR}/nss-3.12.5-nspr-4.8.2.tar.gz
tar xzvf ${SOURCES_DIR}/silc-toolkit-1.1.8.tar.gz

cd ${PIDGIN_DEV_ROOT}
tar xjvf ${SOURCES_DIR}/pidgin-${PIDGIN_VERSION}.tar.bz2
```

Now create `${PIDGIN_DIR}/local.mak`:
```
CC := /usr/bin/i586-mingw32msvc-cc
GMSGFMT := msgfmt
MAKENSIS := /usr/bin/makensis
PERL := /usr/bin/perl
EXTUTILS := /usr/share/perl/5.10/ExtUtils
WINDRES := /usr/bin/i586-mingw32msvc-windres
STRIP := /usr/bin/i586-mingw32msvc-strip

INCLUDE_PATHS := -I$(PIDGIN_TREE_TOP)/../win32-dev/w32api/include
LIB_PATHS := -L$(PIDGIN_TREE_TOP)/../win32-dev/w32api/lib
```

And continue:
```
sed -i /^PIDGIN_TRAY/d ${PIDGIN_DIR}/pidgin/win32/pidgin_dll_rc.rc.in

cd ${PIDGIN_DIR}/pidgin/plugins/
tar xzvf ~/pidgin-privacy-please-${PIDGIN_PP_VERSION}.tar.gz
cd pidgin-privacy-please-${PIDGIN_PP_VERSION}
make -f Makefile.mingw
make -f Makefile.mingw dist
```

Or, as xaminmo points out, to build the latest development version of the plugin:
```
sed -i /^PIDGIN_TRAY/d ${PIDGIN_DIR}/pidgin/win32/pidgin_dll_rc.rc.in

cd ${PIDGIN_DIR}/pidgin/plugins/
svn checkout http://pidgin-privacy-please.googlecode.com/svn/trunk/ pidgin-privacy-please-read-only
cd pidgin-privacy-please-read-only
make -f Makefile.mingw
make -f Makefile.mingw dist
```