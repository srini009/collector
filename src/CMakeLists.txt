# set source files
set (server-src-files
     provider.c)

set (client-src-files
     client.c)

set (admin-src-files
     admin.c)

set (dummy-src-files
     dummy/dummy-backend.c)

set (bedrock-module-src-files
     bedrock-module.c)

# load package helper for generating cmake CONFIG packages
include (CMakePackageConfigHelpers)

# where to install files for "find_package"
set (collector-pkg "share/cmake/collector")

# set version fo shared library
set (collector-vers "${COLLECTOR_VERSION_MAJOR}.${COLLECTOR_VERSION_MINOR}")

if(${ENABLE_AGGREGATOR})
  add_definitions("-DUSE_AGGREGATOR")
endif(${ENABLE_AGGREGATOR})

# server library
add_library (collector-server ${server-src-files} ${dummy-src-files})
target_link_libraries (collector-server
    PkgConfig::MARGO
    PkgConfig::ABTIO
    PkgConfig::UUID)
#    PkgConfig::JSONC)

if(${ENABLE_AGGREGATOR})
  add_definitions("-DUSE_AGGREGATOR")
  target_include_directories (collector-server PUBLIC ${AGGREGATOR_INCLUDE_DIR})
  target_link_libraries (collector-server ${AGGREGATOR_LIBRARIES})
endif(${ENABLE_AGGREGATOR})

target_include_directories (collector-server PUBLIC $<INSTALL_INTERFACE:include>)
target_include_directories (collector-server BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>)
target_include_directories (collector-server BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
set_target_properties (collector-server
    PROPERTIES VERSION ${COLLECTOR_VERSION}
    SOVERSION ${COLLECTOR_VERSION_MAJOR})

# client library
add_library (collector-client ${client-src-files})
target_link_libraries (collector-client PkgConfig::MARGO PkgConfig::UUID)
target_include_directories (collector-client PUBLIC $<INSTALL_INTERFACE:include>)
target_include_directories (collector-client BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>)
target_include_directories (collector-client BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
set_target_properties (collector-client
    PROPERTIES VERSION ${COLLECTOR_VERSION}
    SOVERSION ${COLLECTOR_VERSION_MAJOR})

# admin library
add_library (collector-admin ${admin-src-files})
target_link_libraries (collector-admin PkgConfig::MARGO PkgConfig::UUID)
target_include_directories (collector-admin PUBLIC $<INSTALL_INTERFACE:include>)
target_include_directories (collector-admin BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>)
target_include_directories (collector-admin BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
set_target_properties (collector-admin
    PROPERTIES VERSION ${COLLECTOR_VERSION}
    SOVERSION ${COLLECTOR_VERSION_MAJOR})

if (${ENABLE_BEDROCK})
# bedrock-module library
add_library (collector-bedrock-module ${bedrock-module-src-files})
target_link_libraries (collector-bedrock-module collector-server collector-admin collector-client)
target_include_directories (collector-bedrock-module PUBLIC $<INSTALL_INTERFACE:include>)
target_include_directories (collector-bedrock-module BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>)
target_include_directories (collector-bedrock-module BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
set_target_properties (collector-bedrock-module
    PROPERTIES VERSION ${COLLECTOR_VERSION}
    SOVERSION ${COLLECTOR_VERSION_MAJOR})
endif ()

# installation stuff (packaging and install commands)
write_basic_package_version_file(
    "collector-config-version.cmake"
    VERSION ${COLLECTOR_VERSION}
    COMPATIBILITY AnyNewerVersion)

# generate our config file for find_package()
set(INCLUDE_INSTALL_DIR include/)
set(LIB_INSTALL_DIR lib/)
configure_package_config_file(collector-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/collector-config.cmake
  INSTALL_DESTINATION ${LIB_INSTALL_DIR}/collector/cmake
  PATH_VARS INCLUDE_INSTALL_DIR)

# some bits for the pkg-config file
set (DEST_DIR "${CMAKE_INSTALL_PREFIX}")
set (SERVER_PRIVATE_LIBS "-lcollector-server")
set (CLIENT_PRIVATE_LIBS "-lcollector-client")
set (ADMIN_PRIVATE_LIBS  "-lcollector-admin")
configure_file ("collector-server.pc.in" "collector-server.pc" @ONLY)
configure_file ("collector-client.pc.in" "collector-client.pc" @ONLY)
configure_file ("collector-admin.pc.in"  "collector-admin.pc"  @ONLY)

# configure config.h
configure_file ("config.h.in" "config.h" @ONLY)

# "make install" rules
install (TARGETS collector-server collector-client collector-admin
         EXPORT collector-targets
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib)
if (${ENABLE_BEDROCK})
    install (TARGETS collector-bedrock-module
             ARCHIVE DESTINATION lib
             LIBRARY DESTINATION lib)
endif ()
install (EXPORT collector-targets
         DESTINATION ${collector-pkg}
         FILE "collector-targets.cmake")
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/collector-config.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/collector-config-version.cmake"
         DESTINATION ${collector-pkg} )
install (DIRECTORY ../include/collector
         DESTINATION include
         FILES_MATCHING PATTERN "*.h")
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/collector-server.pc"
               "${CMAKE_CURRENT_BINARY_DIR}/collector-client.pc"
               "${CMAKE_CURRENT_BINARY_DIR}/collector-admin.pc"
         DESTINATION "lib/pkgconfig/")
