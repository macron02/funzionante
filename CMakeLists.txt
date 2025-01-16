cmake_minimum_required(VERSION 3.10)

# Nome del progetto e versione di C++
project(gsoap_onvif)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)



# Directory principali
set(RESOURCE_DIR ${CMAKE_SOURCE_DIR}/resources)
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/source)

# File principali
set(MAIN_CPP ${RESOURCE_DIR}/main1.cpp)

# Librerie richieste
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# Aggiungi le directory di include
include_directories(
    ${SOURCE_DIR}
    ${SOURCE_DIR}/gsoap-2.8/gsoap
    ${SOURCE_DIR}/gsoap-2.8/gsoap/plugin
    ${SOURCE_DIR}/gsoap-2.8/gsoap/custom
)

# File sorgenti generati da gSOAP (C++)
set(GSOAP_SOURCES
    ${SOURCE_DIR}/soapC.cpp
    ${SOURCE_DIR}/wsddClient.cpp
    ${SOURCE_DIR}/wsddServer.cpp
    ${SOURCE_DIR}/soapAdvancedSecurityServiceBindingProxy.cpp
    ${SOURCE_DIR}/soapDeviceBindingProxy.cpp
    ${SOURCE_DIR}/soapDeviceIOBindingProxy.cpp
    ${SOURCE_DIR}/soapImagingBindingProxy.cpp
    ${SOURCE_DIR}/soapMediaBindingProxy.cpp
    ${SOURCE_DIR}/soapPTZBindingProxy.cpp
    ${SOURCE_DIR}/soapPullPointSubscriptionBindingProxy.cpp
    ${SOURCE_DIR}/soapRemoteDiscoveryBindingProxy.cpp
    ${SOURCE_DIR}/soapThermalBindingProxy.cpp
    ${SOURCE_DIR}/gsoap-2.8/gsoap/stdsoap2.cpp
    ${SOURCE_DIR}/gsoap-2.8/gsoap/dom.cpp
    ${SOURCE_DIR}/gsoap-2.8/gsoap/plugin/wsaapi.c # Questo verrà trattato come C++
    ${SOURCE_DIR}/gsoap-2.8/gsoap/plugin/wsseapi.c
    ${SOURCE_DIR}/gsoap-2.8/gsoap/plugin/wsddapi.c
    ${SOURCE_DIR}/gsoap-2.8/gsoap/custom/struct_timeval.c
)

# File sorgenti puri C
set(GSOAP_C_SOURCES
    ${SOURCE_DIR}/gsoap-2.8/gsoap/plugin/smdevp.c
    ${SOURCE_DIR}/gsoap-2.8/gsoap/plugin/mecevp.c
    #${SOURCE_DIR}/gsoap-2.8/gsoap/plugin/wsseapi.c
    #${SOURCE_DIR}/gsoap-2.8/gsoap/plugin/wsddapi.c
    ${SOURCE_DIR}/gsoap-2.8/gsoap/plugin/httpda.c
    #${SOURCE_DIR}/gsoap-2.8/gsoap/custom/struct_timeval.c
)


# Aggiungi wsaapi.c, trattandolo come C++
#list(APPEND SOURCES ${SOURCE_DIR}/gsoap-2.8/gsoap/plugin/wsaapi.c)
set_source_files_properties(${SOURCE_DIR}/gsoap-2.8/gsoap/plugin/wsaapi.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(${SOURCE_DIR}/gsoap-2.8/gsoap/plugin/wsseapi.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(${SOURCE_DIR}/gsoap-2.8/gsoap/plugin/wsddapi.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(${SOURCE_DIR}/gsoap-2.8/gsoap/custom/struct_timeval.c PROPERTIES LANGUAGE CXX)



# Definizioni per gSOAP
add_definitions(-std=c++11 -DWITH_OPENSSL -DWITH_DOM -DWITH_ZLIB -DWITH_NO_CGI -DWITH_NONAMESPACES)

# Crea l'eseguibile
add_executable(ipcamera ${MAIN_CPP} ${GSOAP_SOURCES} ${GSOAP_C_SOURCES})

# Specifica che i file C devono essere compilati come C puro
#set_source_files_properties(${GSOAP_C_SOURCES} PROPERTIES LANGUAGE CXX)

# Linka le librerie
target_link_libraries(ipcamera OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB pthread)

# Messaggi utili
message(STATUS "Resource directory: ${RESOURCE_DIR}")
message(STATUS "Source directory: ${SOURCE_DIR}")