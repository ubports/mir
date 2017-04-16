find_package(PkgConfig)
pkg_check_modules(PKG_WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(PKG_WAYLAND_SERVER REQUIRED wayland-server)
pkg_check_modules(PKG_WAYLAND_EGL REQUIRED wayland-egl)

if (NOT PKG_WAYLAND_CLIENT_FOUND)
    message(FATAL_ERROR "No wayland-client")
endif(NOT PKG_WAYLAND_CLIENT_FOUND)

if (NOT PKG_WAYLAND_SERVER_FOUND)
    message(FATAL_ERROR "No wayland-server")
endif(NOT PKG_WAYLAND_SERVER_FOUND)

if (NOT PKG_WAYLAND_EGL_FOUND)
    message(FATAL_ERROR "No wayland-egl")
endif(NOT PKG_WAYLAND_EGL_FOUND)

# find include paths
find_path(WAYLAND_CLIENT_INCLUDE_DIR wayland-client.h ${PKG_WAYLAND_CLIENT_INCLUDE_DIRS})
find_path(WAYLAND_SERVER_INCLUDE_DIR wayland-server.h ${PKG_WAYLAND_SERVER_INCLUDE_DIRS})
find_path(WAYLAND_SERVER_INCLUDE_DIR wayland-egl.h ${PKG_WAYLAND_EGL_INCLUDE_DIRS})

# find libs
find_library(WAYLAND_CLIENT_LIBRARIES NAMES wayland-client PATHS ${PKG_WAYLAND_CLIENT_LIBRARY_DIRS})
find_library(WAYLAND_SERVER_LIBRARIES NAMES wayland-server PATHS ${PKG_WAYLAND_SERVER_LIBRARY_DIRS})
find_library(WAYLAND_EGL_LIBRARIES NAMES wayland-egl PATHS ${PKG_WAYLAND_EGL_LIBRARY_DIRS})

# set _FOUND vars
set(WAYLAND_CLIENT_FOUND TRUE)
set(WAYLAND_SERVER_FOUND TRUE)
set(WAYLAND_EGL_FOUND TRUE)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WAYLAND DEFAULT_MSG WAYLAND_CLIENT_INCLUDE_DIR WAYLAND_CLIENT_LIBRARIES
    WAYLAND_SERVER_INCLUDE_DIR WAYLAND_SERVER_LIBRARIES
    WAYLAND_EGL_INCLUDE_DIR WAYLAND_EGL_LIBRARIES)
mark_as_advanced(WAYLAND_CLIENT_INCLUDE_DIR WAYLAND_CLIENT_LIBRARIES
    WAYLAND_SERVER_INCLUDE_DIR WAYLAND_SERVER_LIBRARIES
    WAYLAND_EGL_INCLUDE_DIR WAYLAND_EGL_LIBRARIES)

include_directories(../../mesa/server)
include_directories(../../mesa/include)

include_directories(
  ${server_common_include_dirs}
  ${GBM_INCLUDE_DIRS}
  ${DRM_INCLUDE_DIRS}
  ${EGL_INCLUDE_DIRS}
  ${GLESv2_INCLUDE_DIRS}
  ${EPOXY_INCLUDE_DIRS}
  ${WAYLAND_INCLUDE_DIRS}
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map.in
  ${CMAKE_CURRENT_BINARY_DIR}/symbols.map
)
set(symbol_map ${CMAKE_CURRENT_BINARY_DIR}/symbols.map)

add_library(mirplatformgraphicswaylandserverobjects OBJECT
  platform_symbols.cpp
  platform.cpp
  platform.h
  egl_helper.cpp
  egl_helper.h
  display_buffer.cpp
  display_buffer.h
  display_configuration.cpp
  display_configuration.h
  display.cpp
  display.h
  buffer_allocator.cpp
  software_buffer.cpp
)

add_library(mirplatformgraphicswaylandserver MODULE
  $<TARGET_OBJECTS:mirplatformgraphicswaylandserverobjects>
)

target_link_libraries(
  mirplatformgraphicswaylandserver

  PRIVATE
  mirplatform
  server_platform_common
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  mirsharedmesaservercommon-static
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
  ${EPOXY_LDFLAGS} ${EPOXY_LIBRARIES}
  ${WAYLAND_LDFLAGS} ${WAYLAND_LIBRARIES}
  ${WAYLAND_CLIENT_LIBRARIES}
  ${WAYLAND_EGL_LIBRARIES}
  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
  ${GBM_LDFLAGS} ${GBM_LIBRARIES}
)

set_target_properties(
  mirplatformgraphicswaylandserver PROPERTIES
  OUTPUT_NAME graphics-wayland
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
  PREFIX ""
  SUFFIX ".so.${MIR_SERVER_GRAPHICS_PLATFORM_ABI}"
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

install(TARGETS mirplatformgraphicswaylandserver LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
