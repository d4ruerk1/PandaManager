# Maintainer: d4ruerk1 <https://github.com/d4ruerk1/PandaManager>
# SPDX-License-Identifier: Apache-2.0

pkgname=pandamanager
pkgver=1.0.0
pkgrel=1
pkgdesc="File manager Next-Gen Dual-Pane & Hybrid per KDE Plasma 6 (CachyOS)"
arch=('x86_64' 'aarch64')
url="https://github.com/d4ruerk1/PandaManager"
license=('Apache-2.0')
depends=(
    'qt6-base'
    'qt6-multimedia'
    'kf6-coreaddons'
    'kf6-i18n'
    'kf6-kio'
    'kf6-solid'
    'kf6-wallet'
    'ffmpeg'
    'libraw'
    'taglib'
    'rclone'
    'udisks2'
    'polkit'
)
makedepends=(
    'cmake'
    'ninja'
    'clang'
    'extra-cmake-modules'
    'qt6-tools'
)
optdepends=(
    'clamav: scansione malware per file cloud'
    'gnupg: verifica firma GPG pacchetti cloud'
    'libopenmpt: anteprima tracker MOD/XM/IT/S3M'
)
source=("${pkgname}-${pkgver}.tar.gz::${url}/archive/refs/tags/v${pkgver}.tar.gz")
sha256sums=('SKIP')  # sostituire con il checksum reale del tarball firmato

# ── Verificare firma GPG del tarball ─────────────────────────────────────────
# validpgpkeys=('FINGERPRINT_SVILUPPATORE')

prepare() {
    cd "${pkgname}-${pkgver}"
}

build() {
    cmake \
        -B build \
        -S "${pkgname}-${pkgver}" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DENABLE_SANITIZERS=OFF \
        -Wno-dev
    cmake --build build -j"$(nproc)"
}

check() {
    ctest --test-dir "${pkgname}-${pkgver}/build" --output-on-failure
}

package() {
    DESTDIR="${pkgdir}" cmake --install "${pkgname}-${pkgver}/build"

    # Licenza
    install -Dm644 "${pkgname}-${pkgver}/LICENSE" \
        "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"

    # Documentazione
    for doc in README.md CHANGELOG.md SECURITY.md PRIVACY.md ACCESSIBILITY.md; do
        [[ -f "${pkgname}-${pkgver}/${doc}" ]] && \
            install -Dm644 "${pkgname}-${pkgver}/${doc}" \
                "${pkgdir}/usr/share/doc/${pkgname}/${doc}"
    done

    # AppStream metainfo
    install -Dm644 \
        "${pkgname}-${pkgver}/assets/org.cachyos.PandaManager.appdata.xml" \
        "${pkgdir}/usr/share/metainfo/org.cachyos.PandaManager.appdata.xml"

    # Icona
    install -Dm644 \
        "${pkgname}-${pkgver}/assets/panda_icon.png" \
        "${pkgdir}/usr/share/icons/hicolor/256x256/apps/org.cachyos.PandaManager.png"
}
