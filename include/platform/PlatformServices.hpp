// SPDX-License-Identifier: Apache-2.0
// PandaManager — Platform abstraction layer
// Provides compile-time feature detection macros used across the codebase.
#pragma once

// Secret storage backend
#if defined(HAVE_KF6) && defined(PLATFORM_LINUX)
#  define PM_SECRET_BACKEND_KWALLET 1
#elif defined(PLATFORM_MACOS)
#  define PM_SECRET_BACKEND_KEYCHAIN 1
#elif defined(PLATFORM_WINDOWS)
#  define PM_SECRET_BACKEND_WINCRED 1
#else
#  define PM_SECRET_BACKEND_NONE 1
#endif

// Device watcher backend
#if defined(HAVE_KF6) && defined(PLATFORM_LINUX)
#  define PM_DEVICE_BACKEND_UDISKS2 1
#elif defined(PLATFORM_MACOS)
#  define PM_DEVICE_BACKEND_DISKARBITRATION 1
#elif defined(PLATFORM_WINDOWS)
#  define PM_DEVICE_BACKEND_WMI 1
#else
#  define PM_DEVICE_BACKEND_NONE 1
#endif

// Privileged operations
#if defined(HAVE_KF6) && defined(PLATFORM_LINUX)
#  define PM_PRIV_BACKEND_DBUS 1
#else
#  define PM_PRIV_BACKEND_STUB 1
#endif
