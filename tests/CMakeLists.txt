# SPDX-License-Identifier: Apache-2.0
# PandaManager v1.0.0 — Test suite CMake
cmake_minimum_required(VERSION 3.24)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)
find_package(Qt6 QUIET  COMPONENTS DBus)

# ── Test 1: Unit tests ────────────────────────────────────────────────────────
add_executable(test_pandamanager
    test_pandamanager.cpp
    ../src/MediaPreviewEngine.cpp
    ../src/CloudManager.cpp
    ../src/TransferQueue.cpp
    # BUG FIX: CloudManager dipende da KWalletManager e RcloneDbus;
    # DeviceManager dipende da UDisks2Manager e SolidDeviceWatcher su Linux.
    # Includiamo i .cpp necessari per evitare errori di linker "undefined reference".
    ../src/KWalletManager.cpp
    ../src/RcloneDbus.cpp
    ../src/DeviceManager.cpp
)
target_include_directories(test_pandamanager PRIVATE ../include ../src/platform)
target_link_libraries(test_pandamanager
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Test
)
if(TARGET Qt6::DBus)
    target_link_libraries(test_pandamanager Qt6::DBus)
endif()
if(TARGET KF6::Wallet)
    target_link_libraries(test_pandamanager KF6::Wallet)
endif()
if(TARGET KF6::Solid)
    target_link_libraries(test_pandamanager KF6::Solid)
endif()
if(TARGET KF6::KIO)
    target_link_libraries(test_pandamanager KF6::KIO)
endif()
add_test(NAME PandaManagerUnitTests COMMAND test_pandamanager)

# ── Test 2: UI automated tests ────────────────────────────────────────────────
add_executable(test_ui
    test_ui.cpp
)
target_include_directories(test_ui PRIVATE ../include)
target_link_libraries(test_ui
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test
)
add_test(NAME PandaManagerUITests COMMAND test_ui)

# ── Test 3: Rclone mock D-Bus integration tests ───────────────────────────────
add_executable(test_rclone_mock
    test_rclone_mock.cpp
)
target_include_directories(test_rclone_mock PRIVATE ../include)
target_link_libraries(test_rclone_mock
    Qt6::Core Qt6::Test
)
if(TARGET Qt6::DBus)
    target_link_libraries(test_rclone_mock Qt6::DBus)
endif()
add_test(NAME PandaManagerRcloneMockTests COMMAND test_rclone_mock)

# ── Coverage ──────────────────────────────────────────────────────────────────
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        foreach(t test_pandamanager test_ui test_rclone_mock)
            target_compile_options(${t} PRIVATE --coverage -O0 -g)
            target_link_options(${t}    PRIVATE --coverage)
        endforeach()
        message(STATUS "Coverage: gcov enabled (GCC)")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        foreach(t test_pandamanager test_ui test_rclone_mock)
            target_compile_options(${t} PRIVATE -fprofile-instr-generate -fcoverage-mapping -O0 -g)
            target_link_options(${t}    PRIVATE -fprofile-instr-generate)
        endforeach()
        message(STATUS "Coverage: llvm-cov enabled (Clang)")
    endif()
endif()
