#!/bin/bash
#APP_VER=0.0.1
APP_NAME=shvspy
SRC_DIR=/home/fanda/proj/quickbox
QT_DIR=/home/fanda/programs/qt5/5.14.1/gcc_64
WORK_DIR=/home/fanda/t/_distro

APP_IMAGE_TOOL=/home/fanda/programs/appimagetool-x86_64.AppImage

help() {
	echo "Usage: make-dist.sh [ options ... ]"
	echo "required options: src-dir, qt-dir, work-dir, image-tool"
	echo -e "\n"
	echo "avaible options"
	echo "    --app-version <version>  application version, ie: 1.0.0 or my-test"
	echo "    --src-dir <path>         shvspy project root dir, *.pro file is located, ie: /home/me/quickbox"
	echo "    --qt-dir <path>          QT dir, ie: /home/me/qt5/5.13.1/gcc_64"
	echo "    --work-dir <path>        directory where build files and AppImage will be created, ie: /home/me/quickevent/AppImage"
	echo "    --appimage-tool <path>      path to AppImageTool, ie: /home/me/appimagetool-x86_64.AppImage"
	echo "    --no-clean               do not rebuild whole project when set to 1"
	echo -e "\n"
	echo "example: make-dist.sh --src-dir /home/me/quickbox --qt-dir /home/me/qt5/5.13.1/gcc_64 --work-dir /home/me/quickevent/AppImage --image-tool /home/me/appimagetool-x86_64.AppImage"
	exit 0
}

error() {
	echo -e "\e[31m${1}\e[0m"
}

while [[ $# -gt 0 ]]
do
key="$1"
# echo key: $key
case $key in
	--app-name)
	APP_NAME="$2"
	shift # past argument
	shift # past value
	;;
	--app-version)
	APP_VER="$2"
	shift # past argument
	shift # past value
	;;
	--qt-dir)
	QT_DIR="$2"
	shift # past argument
	shift # past value
	;;
	--src-dir)
	SRC_DIR="$2"
	shift # past argument
	shift # past value
	;;
	--work-dir)
	WORK_DIR="$2"
	shift # past argument
	shift # past value
	;;
	--appimage-tool)
	APP_IMAGE_TOOL="$2"
	shift # past argument
	shift # past value
	;;
	--no-clean)
	NO_CLEAN=1
	shift # past value
	;;
	-h|--help)
	shift # past value
	help
	;;
	*)    # unknown option
	echo "ignoring argument $1"
	shift # past argument
	;;
esac
done

SRC_DIR=`readlink -f $SRC_DIR`
WORK_DIR=`readlink -f $WORK_DIR`

if [ ! -d $SRC_DIR ]; then
   	error "invalid source dir, use --src-dir <path> to specify it\n"
	help
fi
if [ ! -d $QT_DIR ]; then
	error "invalid QT dir, use --qt-dir <path> to specify it\n"
	help
fi
if [ $WORK_DIR = "/home/fanda/t/_distro" ] && [ ! -d "/home/fanda/t/_distro" ]; then
	error "invalid work dir, use --work-dir <path> to specify it\n"
	help
fi
if [ ! -f $APP_IMAGE_TOOL ]; then
	error "invalid path to AppImageTool, use --appimage-tool <path> to specify it\n"
	help
fi
if [ ! -x $APP_IMAGE_TOOL ]; then
	error "AppImageTool file must be executable, use chmod +x $APP_IMAGE_TOOL\n"
	help
fi


if [ -z $APP_VER ]; then
	APP_VER=`grep APP_VERSION $SRC_DIR/shvspy/src/appversion.h | cut -d\" -f2`
	echo "Distro version not specified, deduced from source code: $APP_VER" >&2
	#exit 1
fi

echo APP_VER: $APP_VER
echo APP_NAME: $APP_NAME
echo SRC_DIR: $SRC_DIR
echo WORK_DIR: $WORK_DIR
echo NO_CLEAN: $NO_CLEAN

if [ -z $USE_SYSTEM_QT ]; then
	QT_LIB_DIR=$QT_DIR/lib
	DISTRO_NAME=$APP_NAME-$APP_VER-linux64
else
	QT_DIR=/usr/lib/i386-linux-gnu/qt5
	QT_LIB_DIR=/usr/lib/i386-linux-gnu
	DISTRO_NAME=$APP_NAME-$APP_VER-linux32
fi

echo QT_DIR: $QT_DIR

BUILD_DIR=$WORK_DIR/_build
DIST_DIR=$WORK_DIR/$DISTRO_NAME
DIST_LIB_DIR=$DIST_DIR/lib
DIST_BIN_DIR=$DIST_DIR/bin
DIST_QML_DIR=$DIST_DIR/qml

if [ -z $NO_CLEAN ]; then
	echo removing directory $WORK_DIR
	rm -r $BUILD_DIR
fi

mkdir -p $BUILD_DIR
cd $BUILD_DIR
CFLAGS="-Werror" CXXFLAGS="-Werror" cmake -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$QT_DIR" -DCMAKE_INSTALL_PREFIX=. ../..
make -j2
if [ $? -ne 0 ]; then
	echo "Make Error" >&2
	exit 1
fi
make install
if [ $? -ne 0 ]; then
	echo "Make Error" >&2
	exit 1
fi

rm -r $DIST_DIR
mkdir -p $DIST_DIR

RSYNC='rsync -av --exclude *.debug'
# $RSYNC expands as: rsync -av '--exclude=*.debug'

$RSYNC $BUILD_DIR/lib/ $DIST_LIB_DIR
$RSYNC $BUILD_DIR/bin/ $DIST_BIN_DIR

$RSYNC $QT_LIB_DIR/libQt6Core.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6Gui.so* $DIST_LIB_DIR

$RSYNC $QT_LIB_DIR/libQt6QuickWidgets.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6Location.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6PositioningQuick.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6Positioning.so* $DIST_LIB_DIR

$RSYNC $QT_LIB_DIR/libQt6Widgets.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6XmlPatterns.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6Network.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6WebSockets.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6Sql.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6Xml.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6Qml.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6Quick.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6QuickControls2.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6QuickTemplates2.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6QmlWorkerScript.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6QmlModels.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6Svg.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6Script.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6ScriptTools.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6PrintSupport.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6SerialPort.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6DBus.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6Multimedia.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6XcbQpa.so* $DIST_LIB_DIR
$RSYNC $QT_LIB_DIR/libQt6WebSockets.so* $DIST_LIB_DIR

$RSYNC $QT_LIB_DIR/libQt6OpcUa.so* $DIST_LIB_DIR

$RSYNC $QT_LIB_DIR/libQt6Mqtt.so* $DIST_LIB_DIR

$RSYNC $QT_LIB_DIR/libicu*.so* $DIST_LIB_DIR

$RSYNC $QT_DIR/plugins/platforms/ $DIST_BIN_DIR/platforms
$RSYNC $QT_DIR/plugins/printsupport/ $DIST_BIN_DIR/printsupport
$RSYNC $QT_DIR/plugins/geoservices/ $DIST_BIN_DIR/geoservices

mkdir -p $DIST_BIN_DIR/imageformats
$RSYNC $QT_DIR/plugins/imageformats/libqjpeg.so $DIST_BIN_DIR/imageformats/
$RSYNC $QT_DIR/plugins/imageformats/libqsvg.so $DIST_BIN_DIR/imageformats/

mkdir -p $DIST_BIN_DIR/sqldrivers
$RSYNC $QT_DIR/plugins/sqldrivers/libqsqlite.so $DIST_BIN_DIR/sqldrivers/
$RSYNC $QT_DIR/plugins/sqldrivers/libqsqlpsql.so $DIST_BIN_DIR/sqldrivers/

mkdir -p $DIST_BIN_DIR/audio
$RSYNC $QT_DIR/plugins/audio/ $DIST_BIN_DIR/audio/

mkdir -p $DIST_QML_DIR
$RSYNC $QT_DIR/qml/QtLocation $DIST_QML_DIR/
$RSYNC $QT_DIR/qml/QtPositioning $DIST_QML_DIR/
$RSYNC $QT_DIR/qml/QtQuick $DIST_QML_DIR/
$RSYNC $QT_DIR/qml/QtQuick.2 $DIST_QML_DIR/

ARTIFACTS_DIR=$WORK_DIR/artifacts
mkdir -p $ARTIFACTS_DIR

tar -cvzf $ARTIFACTS_DIR/$DISTRO_NAME.tgz  -C $WORK_DIR ./$DISTRO_NAME

rsync -av $SRC_DIR/distro/shvspy.AppDir/* $DIST_DIR/
ARCH=x86_64 $APP_IMAGE_TOOL $DIST_DIR $ARTIFACTS_DIR/$APP_NAME-${APP_VER}-linux64.AppImage

