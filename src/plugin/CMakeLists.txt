find_package(bctoolbox REQUIRED)
find_package(Mediastreamer2 REQUIRED)

add_library(mselph264 MODULE
    src/cam.cpp
    src/filter.cpp
    src/filter.hpp
    src/h264helper.cpp
    src/h264helper.hpp
    src/mselph264.cpp
    src/utils.hpp
)
target_compile_options(mselph264 PRIVATE "-Wall" "-Wextra")
target_compile_features(mselph264 PUBLIC cxx_std_14)
target_compile_definitions(mselph264
    PRIVATE
        BCTBX_LOG_DOMAIN="mselph264"
        BCTBX_DEBUG_MODE=1
)
target_include_directories(mselph264
    PRIVATE ${BCTOOLBOX_INCLUDE_DIRS}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/
    PRIVATE ${MEDIASTREAMER2_INCLUDE_DIRS}
)
target_link_libraries(mselph264
    PRIVATE elph264
    PRIVATE ${MEDIASTREAMER2_LIBRARIES}
)

install(TARGETS mselph264
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/mediastreamer/plugins
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mediastreamer/plugins
)

if(ENABLE_TEST)
    find_package(Catch2 REQUIRED)
    find_package(ortp REQUIRED)
    enable_testing()

    add_executable(plugin_test
        test/helper.cpp
        test/helper.hpp
        test/main.cpp
        test/tc_plugin.cpp
    )
    target_include_directories(plugin_test
        PRIVATE
            ${MEDIASTREAMER2_INCLUDE_DIRS}
            src/
    )
    target_compile_definitions(plugin_test
        PRIVATE
            MS_PLUGIN_DIRECTORY="${CMAKE_CURRENT_BINARY_DIR}"
            BCTBX_LOG_DOMAIN="plugin_test"
    )
    target_link_libraries(plugin_test
        PRIVATE Catch2::Catch2
        PRIVATE ${BCTOOLBOX_CORE_LIBRARIES}
        PRIVATE ${MEDIASTREAMER2_LIBRARIES}
        PRIVATE ${ORTP_LIBRARIES}
    )
    add_test(NAME plugin_test COMMAND ${CMAKE_CURRENT_BINARY_DIR}/plugin_test)
endif()
