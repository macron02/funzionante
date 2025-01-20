#!/bin/bash
THIS_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
RESOURCE_DIR="${THIS_DIR}/resources"
GSOAP_ARCHIVE="${RESOURCE_DIR}/gsoap_2.8.135.zip"
SOURCE_DIR="${THIS_DIR}/source"
GSOAP_SOURCE="${SOURCE_DIR}/gsoap-2.8/gsoap"
TYPEMAD_FILE_ORIG="${GSOAP_SOURCE}/typemap.dat"
TYPEMAD_FILE="${SOURCE_DIR}/typemap.dat"

if [ "$1" == "-g" ]; then

mkdir -p "${SOURCE_DIR}"

unzip "${GSOAP_ARCHIVE}" -d "${SOURCE_DIR}" || exit 1

cp "$TYPEMAD_FILE_ORIG" "$TYPEMAD_FILE" || exit 1

# Modifica typemap
sed -i -e 's/^#\(wsdd5  = <http:\/\/schemas\.xmlsoap\.org\/ws\/2005\/04\/discovery>\)/\1/' \
       -e 's/^\(wsdd10  = <http:\/\/schemas\.xmlsoap\.org\/ws\/2005\/04\/discovery>\)/#\1/' "${TYPEMAD_FILE}" || exit 1

echo "tth     = \"http://www.onvif.org/ver10/thermal/wsdl\"" >> "${TYPEMAD_FILE}"

cd "${SOURCE_DIR}" || exit 1
#tolto meno x per prova di metodi
wsdl2h -O4 -P -o onvif.h \
    "http://www.onvif.org/onvif/ver10/device/wsdl/devicemgmt.wsdl" \
    "http://www.onvif.org/onvif/ver10/events/wsdl/event.wsdl" \
    "http://www.onvif.org/onvif/ver10/deviceio.wsdl" \
    "http://www.onvif.org/onvif/ver20/imaging/wsdl/imaging.wsdl" \
    "http://www.onvif.org/onvif/ver10/media/wsdl/media.wsdl" \
    "http://www.onvif.org/onvif/ver20/ptz/wsdl/ptz.wsdl" \
    "http://www.onvif.org/onvif/ver10/network/wsdl/remotediscovery.wsdl" \
    "http://www.onvif.org/ver10/advancedsecurity/wsdl/advancedsecurity.wsdl" \
    "http://www.onvif.org/ver10/thermal/wsdl/thermal.wsdl" || exit 1

soapcpp2 -2 -I "${GSOAP_SOURCE}/import" "onvif.h" || exit 1

(echo "#import \"wsse.h\""; cat "onvif.h") > temp_file && mv temp_file "onvif.h"

soapcpp2 -2 -C -I "${GSOAP_SOURCE}/import" -I "${GSOAP_SOURCE}" -j -x onvif.h || exit 1
soapcpp2 -a -x -L -pwsdd -I "${GSOAP_SOURCE}/import" "${GSOAP_SOURCE}/import/wsdd5.h" || exit 1
fi

cd "${SOURCE_DIR}" || exit 1
# -DWITH_OPENSSL 
c++ -o ipcamera -Wall -std=c++11 -DWITH_OPENSSL -DWITH_DOM -DWITH_ZLIB -g \
    -I. \
    -I ${GSOAP_SOURCE}/plugin \
    -I ${GSOAP_SOURCE}/custom \
    -I ${GSOAP_SOURCE}/plugin \
    ${RESOURCE_DIR}/main1.cpp \
    soapC.cpp \
    wsddClient.cpp \
    wsddServer.cpp \
    soapAdvancedSecurityServiceBindingProxy.cpp \
    soapDeviceBindingProxy.cpp \
    soapDeviceIOBindingProxy.cpp \
    soapImagingBindingProxy.cpp \
    soapMediaBindingProxy.cpp \
    soapPTZBindingProxy.cpp \
    soapPullPointSubscriptionBindingProxy.cpp \
    soapRemoteDiscoveryBindingProxy.cpp \
    soapThermalBindingProxy.cpp \
    ${GSOAP_SOURCE}/stdsoap2.cpp \
    ${GSOAP_SOURCE}/dom.cpp \
    ${GSOAP_SOURCE}/plugin/smdevp.c \
    ${GSOAP_SOURCE}/plugin/mecevp.c \
    ${GSOAP_SOURCE}/plugin/wsaapi.c \
    ${GSOAP_SOURCE}/plugin/wsseapi.c \
    ${GSOAP_SOURCE}/plugin/wsddapi.c \
    ${GSOAP_SOURCE}/plugin/httpda.c \
    ${GSOAP_SOURCE}/custom/struct_timeval.c \
    -lcrypto -lssl -lz -lpthread
echo "TUTTAPPOST"
# Esegui CMake e Make
#cd "${THIS_DIR}" || exit 1
#cmake -S . -B build || exit 1
#cmake --build build || exit 1
#
#echo "Compilazione completata con successo."
