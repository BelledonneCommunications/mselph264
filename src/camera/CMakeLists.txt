project(h264camera
    VERSION 0.2.0
    LANGUAGES CXX
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(V4L REQUIRED IMPORTED_TARGET libv4l2 libv4l1 libv4lconvert)

include(GNUInstallDirs)

add_library(elph264 SHARED
    include/h264camera/elp_usb100w04h.hpp
    include/h264camera/types.hpp
    include/h264camera/v4l2_device.hpp
    src/data_helper.hpp
    src/elp_usb100w04h.cpp
    src/v4l2_device.cpp
)
add_library(mselph264::camera ALIAS elph264)
target_compile_options(elph264 PRIVATE "-Wall;-Wextra;-Werror;-pedantic")
target_compile_features(elph264 PUBLIC cxx_std_17)
target_include_directories(elph264
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PUBLIC $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(elph264 PRIVATE PkgConfig::V4L)
set_target_properties(elph264 PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

if(CLANG_TIDY_BIN)
    set_target_properties(elph264 PROPERTIES
        CXX_CLANG_TIDY "${CLANG_TIDY_BIN};-p;${CMAKE_BINARY_DIR};-checks='*'"
    )
endif()

install(TARGETS elph264 EXPORT elph264Config
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES elp-camera.rules
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/udev/rules.d
)
install(EXPORT elph264Config
    DESTINATION share/elph264/cmake
)

export(TARGETS elph264 FILE elph264Config.cmake)

if(ENABLE_TEST)
    find_package(Catch2 REQUIRED)
    enable_testing()
    add_executable(h264camera_test
        test/main.cpp
        test/tc_elp_usb100w04h.cpp
        test/tc_v4l2_device.cpp
    )
    target_link_libraries(h264camera_test
        PRIVATE Catch2::Catch2
        PRIVATE mselph264::camera
    )
    add_test(NAME h264camera_test COMMAND ${CMAKE_CURRENT_BINARY_DIR}/h264camera_test)

    find_package(fmt REQUIRED)
    add_executable(capture_tester test/capture_tester.cpp)
    target_link_libraries(capture_tester
        PRIVATE mselph264::camera
        PRIVATE fmt::fmt-header-only
    )
endif(ENABLE_TEST)