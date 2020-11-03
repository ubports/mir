if (MIR_BUILD_PLATFORM_MESA_KMS)
  add_subdirectory(kms/)
endif()
if (MIR_BUILD_PLATFORM_MESA_X11)
  add_subdirectory(x11/)
endif()

add_library(
  mirsharedmesaservercommon-static STATIC

  buffer_allocator.cpp
  display_helpers.cpp
  gbm_buffer.cpp
  ipc_operations.cpp
  software_buffer.cpp
  gbm_platform.cpp
  nested_authentication.cpp
  drm_native_platform.cpp
)

target_include_directories(
  mirsharedmesaservercommon-static
  PUBLIC
    ${server_common_include_dirs}
    ${DRM_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(
  mirsharedmesaservercommon-static
  mirsharedmesa-static

  server_platform_common
  kms_utils
  mirplatform
  mirwayland
)
