# Building PandaManager — Cross-platform guide

PandaManager v1.0.0 compiles on **Linux** (CachyOS, Arch, Ubuntu, Fedora, openSUSE),
**macOS** (13 Ventura+, Apple Silicon & Intel), and **Windows** (10/11, MSVC 2022).

---

## Dependencies

| Library | Linux | macOS | Windows |
|---------|-------|-------|---------|
| Qt ≥ 6.6 | distro pkg | `brew install qt@6` | aqtinstall |
| FFmpeg (libav*) | distro pkg | `brew install ffmpeg` | vcpkg |
| libraw | distro pkg | `brew install libraw` | vcpkg |
| taglib | distro pkg | `brew install taglib` | vcpkg |
| KDE Frameworks 6 | optional / recommended | optional (KDE Craft) | optional (KDE Craft) |
| CMake ≥ 3.24 | ✓ | ✓ | ✓ |

KDE Frameworks (KF6) enable: KWallet, UDisks2/Solid device watcher, D-Bus rclone backend, Polkit privileged helper.
Without KF6 the app uses platform-native backends (Keychain / WinCred / DiskArbitration / WMI).

---

## Linux (CachyOS / Arch)

```bash
# CachyOS / Arch
sudo pacman -S qt6-base qt6-multimedia kf6-solid kf6-kwallet kf6-kio \
               ffmpeg libraw taglib cmake ninja extra-cmake-modules

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
sudo cmake --install build
```

### Ubuntu 24.04
```bash
sudo apt install qt6-base-dev qt6-multimedia-dev \
  libkf6coreaddons-dev libkf6i18n-dev libkf6kio-dev libkf6solid-dev libkf6wallet-dev \
  libavformat-dev libavcodec-dev libavutil-dev libswscale-dev \
  libraw-dev libtag1-dev cmake ninja-build extra-cmake-modules

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Fedora 40+
```bash
sudo dnf install qt6-qtbase-devel qt6-qtmultimedia-devel \
  kf6-kcoreaddons-devel kf6-ki18n-devel kf6-kio-devel kf6-solid-devel kf6-kwallet-devel \
  ffmpeg-devel libraw-devel taglib-devel cmake ninja-build extra-cmake-modules

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### openSUSE Tumbleweed
```bash
sudo zypper install qt6-base-devel qt6-multimedia-devel \
  kf6-kcoreaddons-devel kf6-ki18n-devel kf6-kio-devel kf6-solid-devel kf6-kwallet-devel \
  ffmpeg-devel libraw-devel taglib-devel cmake ninja extra-cmake-modules

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## macOS (13 Ventura+)

```bash
brew install qt@6 ffmpeg libraw taglib cmake ninja pkg-config

export PATH="$(brew --prefix qt@6)/bin:$PATH"
export PKG_CONFIG_PATH="$(brew --prefix ffmpeg)/lib/pkgconfig:\
$(brew --prefix libraw)/lib/pkgconfig:\
$(brew --prefix taglib)/lib/pkgconfig"

cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
cmake --build build -j$(sysctl -n hw.logicalcpu)

# Optional: create .app bundle + .dmg
macdeployqt build/pandamanager.app -dmg
```

**macOS-specific backends:**
- Secrets → macOS Keychain (`Security.framework`)
- Devices → DiskArbitration (`DiskArbitration.framework`)
- Privileged ops → `diskutil` CLI / `AuthorizationExecuteWithPrivileges`
- Cloud sync → rclone launched as a child process (install via `brew install rclone`)

---

## Windows (10 / 11, MSVC 2022)

```powershell
# 1. Install Qt6 via aqtinstall
pip install aqtinstall
aqt install-qt windows desktop 6.7.0 win64_msvc2019_64 -m qtmultimedia

# 2. Install FFmpeg/libraw/taglib via vcpkg
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg install ffmpeg libraw taglib --triplet x64-windows

# 3. Configure & build
cmake -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE="vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.7.0/msvc2019_64"
cmake --build build --config Release -j $env:NUMBER_OF_PROCESSORS

# 4. Deploy Qt DLLs
windeployqt build\Release\pandamanager.exe --release
```

**Windows-specific backends:**
- Secrets → Windows Credential Store (`wincred.h`)
- Devices → WMI Win32_DiskDrive (`wbemuuid`)
- Privileged ops → `ShellExecuteEx` with "runas" verb / SDelete for secure delete
- Cloud sync → rclone launched as a child process

---

## Feature matrix

| Feature | Linux + KF6 | Linux (no KF6) | macOS | Windows |
|---------|-------------|----------------|-------|---------|
| File manager UI | ✅ | ✅ | ✅ | ✅ |
| Media preview | ✅ | ✅ | ✅ | ✅ |
| Cloud sync (rclone) | ✅ D-Bus | ✅ process | ✅ process | ✅ process |
| Secret storage | ✅ KWallet | ⚠️ no backend | ✅ Keychain | ✅ WinCred |
| Device watcher | ✅ UDisks2+Solid | ⚠️ stub | ✅ DiskArbitration | ✅ WMI |
| Privileged ops | ✅ Polkit/D-Bus | ⚠️ stub | ✅ diskutil | ⚠️ partial |
| seccomp sandbox | ✅ | ✅ | ❌ (no-op) | ❌ (no-op) |
| Flatpak | ✅ | ✅ | ❌ | ❌ |
