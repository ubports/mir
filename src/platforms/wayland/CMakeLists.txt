include_directories(include/)
add_definitions(-DMIR_LOG_COMPONENT_FALLBACK="wayland")
add_subdirectory(client/)
add_subdirectory(server/)
