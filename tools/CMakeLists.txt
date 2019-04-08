find_package(Mediastreamer2 REQUIRED)
find_package(BcToolbox REQUIRED)
find_package(ORTP REQUIRED)

add_executable(local_rtp local_rtp.cpp)
target_compile_definitions(local_rtp
    PRIVATE "-DMS_PLUGIN_DIRECTORY=\"${CMAKE_BINARY_DIR}/src/plugin\""
)
target_include_directories(local_rtp
    PRIVATE
        ${BCTOOLBOX_INCLUDE_DIRS}
        ${MEDIASTREAMER2_INCLUDE_DIRS}
        ${ORTP_INCLUDE_DIRS}
)
target_link_libraries(local_rtp
    PRIVATE
        ${BCTOOLBOX_CORE_LIBRARIES}
        ${MEDIASTREAMER2_LIBRARIES}
        ${ORTP_LIBRARIES}
)