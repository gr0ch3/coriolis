#!/bin/bash

# Arguments
SRC_DIR=$1
if [[ -z "${SRC_DIR}" ]]; then
	SRC_DIR=${PWD}
fi

PREFIX=$2
if [[ -z "${PREFIX}" ]]; then
	PREFIX=${PWD}/${BUILD_TYPE}
fi

OUT_DIR=$3
if [[ -z "${OUT_DIR}" ]]; then
	OUT_DIR=${PWD}/${BUILD_TYPE}
fi

PKG_NAME=python3-coriolis-eda
PKG_VERSION=2.5.5
PKG_REVISION=1
PKG_MAINTAINER="Gabriel Rocherolle <gabriel.rocherolle@protonmail.com>"

PKG_ARCH=""
case $(uname -m) in
    i386 | i686)   PKG_ARCH="386" ;;
    x86_64) PKG_ARCH="amd64" ;;
    arm | aarch64)    dpkg --print-architecture | grep -q "arm64" && PKG_ARCH="arm64" || PKG_ARCH="arm" ;;
	*)   echo "Unable to determine system architecture."; exit 1 ;;
esac

DEB_PKG=${PKG_NAME}_${PKG_VERSION}-${PKG_REVISION}_${PKG_ARCH}

PKG_PATH=${OUT_DIR}/pkgs/${DEB_PKG}

mkdir -p ${PKG_PATH}/DEBIAN

# PKG control
cat << EOF > "${PKG_PATH}/DEBIAN/control"
Package: ${PKG_NAME}
Version: ${PKG_VERSION}
Architecture: ${PKG_ARCH}
Section: universe/python
Maintainer: ${PKG_MAINTAINER}
Build-Depends:
 build-essential,
 ccache,
 python3,
 python3-pip,
 python3-venv,
 bison,
 flex,
 rapidjson-dev,
 libboost-all-dev,
 libeigen3-dev,
 libxml2-dev,
 libcairo2-dev,
 qtbase5-dev,
 libqt5svg5-dev,
 libqwt-qt5-dev,
 libbz2-dev,
Depends: python3:any, python3-pip, python3-venv
Recommends: doxygen, pelican, texlive-latex-recommanded
Provides: cyclop
Homepage: https://coriolis.lip6.fr
Description: Coriolis 2 EDA toolchain
 Coriolis provides several tools to perform the layout of VLSI circuits.
 Its main components are the Hurricane database, the Etesian placer and the Katana router, but other tools can use the Hurricane database and the parsers provided.
EOF

# PKG contents
mkdir -p ${PKG_PATH}/usr
cp -r ${PREFIX}/* ${PKG_PATH}/usr

# PKG copyright
echo "Generate copyright file"
python3 ${SRC_DIR}/packaging/ubuntu24.04/gen_coriolis-eda_copyright.py ${PKG_PATH}

# Build PKG
DISTRIB_NAME=$(grep -e "^ID=" /etc/os-release | cut -d '=' -f 2)

echo "Building ${PKG_NAME} for ${DISTRIB_NAME}";
dpkg-deb --build --root-owner-group ${PKG_PATH}