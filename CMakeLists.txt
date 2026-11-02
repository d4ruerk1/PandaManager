cmake_minimum_required(VERSION 3.24)
project(PandaManager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Qt6 (cross-platform) ────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Multimedia MultimediaWidgets)

# ── PkgConfig per FFmpeg/libraw/taglib ──────────────────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG  REQUIRED libavformat libavcodec libavutil libswscale)
pkg_check_modules(LIBRAW  REQUIRED libraw)
pkg_check_modules(TAGLIB  REQUIRED taglib)

# ── KDE Frameworks — opzionali su non-Linux (usabili via KDE Craft) ─────────
set(HAVE_KF6 OFF)
if(UNIX AND NOT APPLE)
    find_package(ECM QUIET)
    if(ECM_FOUND)
        list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
    endif()
    find_package(KF6 QUIET COMPONENTS CoreAddons I18n KIO Solid Wallet JobWidgets WidgetsAddons)
    find_package(Qt6 QUIET COMPONENTS DBus)
    if(KF6_FOUND AND Qt6DBus_FOUND)
        set(HAVE_KF6 ON)
    endif()
elseif(WIN32 OR APPLE)
    # KF6 via KDE Craft — try, but don't require
    find_package(ECM QUIET)
    if(ECM_FOUND)
        list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
    endif()
    find_package(KF6 QUIET COMPONENTS CoreAddons I18n KIO Solid Wallet JobWidgets WidgetsAddons)
    find_package(Qt6 QUIET COMPONENTS DBus)
    if(KF6_FOUND AND Qt6DBus_FOUND)
        set(HAVE_KF6 ON)
    endif()
endif()

message(STATUS "KDE Frameworks 6 support: ${HAVE_KF6}")

# ── Opzioni build ────────────────────────────────────────────────────────────
option(ENABLE_SANITIZERS "Enable ASan + UBSan" OFF)

# ── Flags di compilazione — condizionali per compilatore ────────────────────
if(NOT MSVC)
    add_compile_options(
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion
        -Wformat=2 -Wnull-dereference -Wdouble-promotion
        -fPIE -fstack-protector-strong
    )
    add_link_options(-pie)
    if(UNIX AND NOT APPLE)
        add_link_options(-Wl,-z,relro,-z,now)
    endif()
    if(ENABLE_SANITIZERS)
        add_compile_options(-fsanitize=address,undefined)
        add_link_options(-fsanitize=address,undefined)
    endif()
else()
    add_compile_options(/W4 /WX- /permissive- /sdl /GS)
    if(ENABLE_SANITIZERS)
        add_compile_options(/fsanitize=address)
    endif()
endif()

if(NOT MSVC)
    add_compile_definitions(_FORTIFY_SOURCE=3)
endif()

# ── Resources ────────────────────────────────────────────────────────────────
qt6_add_resources(QRC_SOURCES assets/resources.qrc)

# ── Sorgenti comuni (cross-platform) ─────────────────────────────────────────
set(COMMON_SOURCES
    src/main.cpp
    src/MediaPreviewEngine.cpp
    src/FFmpegThumbnailer.cpp
    src/LibRawPreview.cpp
    src/TagLibReader.cpp
    src/CloudManager.cpp
    src/TransferQueue.cpp
    src/TransferWorker.cpp
    src/AuditLog.cpp
    src/SecureDelete.cpp
    src/ChecksumVerifier.cpp
    src/ClamAvScanner.cpp
    src/GpgVerifier.cpp
    src/SeccompSandbox.cpp
    ${QRC_SOURCES}
)

# ── Sorgenti Linux-only (D-Bus / UDisks2 / KWallet / Solid / Polkit) ─────────
set(LINUX_SOURCES
    src/RcloneDbus.cpp
    src/KWalletManager.cpp
    src/UDisks2Manager.cpp
    src/SolidDeviceWatcher.cpp
    src/PrivilegedHelper.cpp
)

# ── Sorgenti stub per macOS/Windows ──────────────────────────────────────────
set(STUB_SOURCES
    src/platform/stub/RcloneProcess.cpp
    src/platform/stub/SecretManager.cpp
    src/platform/stub/DeviceWatcher.cpp
    src/platform/stub/PrivilegedHelperStub.cpp
)

# ── Sorgenti macOS ───────────────────────────────────────────────────────────
set(MACOS_SOURCES
    src/platform/macos/KeychainManager.mm
    src/platform/macos/DiskArbitrationWatcher.mm
)

# ── Sorgenti Windows ─────────────────────────────────────────────────────────
set(WINDOWS_SOURCES
    src/platform/windows/CredentialStoreManager.cpp
    src/platform/windows/WMIDeviceWatcher.cpp
)

# ── Selezione sorgenti per piattaforma ───────────────────────────────────────
if(UNIX AND NOT APPLE AND HAVE_KF6)
    set(PLATFORM_SOURCES ${LINUX_SOURCES})
elseif(APPLE)
    set(PLATFORM_SOURCES ${STUB_SOURCES} ${MACOS_SOURCES})
elseif(WIN32)
    set(PLATFORM_SOURCES ${STUB_SOURCES} ${WINDOWS_SOURCES})
else()
    # Linux senza KF6 o altra piattaforma UNIX
    set(PLATFORM_SOURCES ${STUB_SOURCES})
endif()

add_executable(pandamanager
    ${COMMON_SOURCES}
    ${PLATFORM_SOURCES}
)

# ── Define per feature detection runtime ─────────────────────────────────────
if(HAVE_KF6)
    target_compile_definitions(pandamanager PRIVATE HAVE_KF6=1)
endif()
if(UNIX AND NOT APPLE)
    target_compile_definitions(pandamanager PRIVATE PLATFORM_LINUX=1)
elseif(APPLE)
    target_compile_definitions(pandamanager PRIVATE PLATFORM_MACOS=1)
elseif(WIN32)
    target_compile_definitions(pandamanager PRIVATE PLATFORM_WINDOWS=1)
endif()

# ── Include dirs ─────────────────────────────────────────────────────────────
target_include_directories(pandamanager PRIVATE
    include
    src/platform
    ${FFMPEG_INCLUDE_DIRS}
    ${LIBRAW_INCLUDE_DIRS}
    ${TAGLIB_INCLUDE_DIRS}
)

# ── Link Qt6 ─────────────────────────────────────────────────────────────────
target_link_libraries(pandamanager PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Multimedia Qt6::MultimediaWidgets
    ${FFMPEG_LIBRARIES}
    ${LIBRAW_LIBRARIES}
    ${TAGLIB_LIBRARIES}
)

# ── Link KDE Frameworks (solo se disponibili) ────────────────────────────────
if(HAVE_KF6)
    target_link_libraries(pandamanager PRIVATE
        Qt6::DBus
        KF6::CoreAddons KF6::I18n KF6::KIO KF6::Solid KF6::Wallet KF6::JobWidgets KF6::WidgetsAddons
    )
endif()

# ── Link piattaforma-specifica ────────────────────────────────────────────────
if(APPLE)
    target_link_libraries(pandamanager PRIVATE
        "-framework Security"
        "-framework DiskArbitration"
        "-framework Foundation"
    )
endif()
if(WIN32)
    target_link_libraries(pandamanager PRIVATE advapi32 wbemuuid ole32 oleaut32)
endif()

# ── Install ──────────────────────────────────────────────────────────────────
install(TARGETS pandamanager DESTINATION bin)
if(UNIX AND NOT APPLE)
    install(FILES pandamanager.desktop DESTINATION share/applications)
    install(FILES assets/panda_icon.svg
            DESTINATION share/icons/hicolor/scalable/apps
            RENAME org.cachyos.PandaManager.svg)
endif()

# ── Tests ────────────────────────────────────────────────────────────────────
enable_testing()
add_subdirectory(tests)

# ── Documentazione ───────────────────────────────────────────────────────────
if(UNIX AND NOT APPLE)
    install(FILES docs/man/pandamanager.1
            DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
    if(TARGET KF6::I18n)
        ki18n_install(po)
    endif()
endif()

find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generazione documentazione Doxygen"
        VERBATIM)
endif()
