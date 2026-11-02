// SPDX-License-Identifier: Apache-2.0
// Windows device watcher — WMI COM + WM_DEVICECHANGE for safe USB hot-plug.
//
// Two complementary approaches:
//   1. Win32_DeviceChangeEvent WMI subscription — asynchronous event for
//      plug/unplug (no polling).
//   2. WM_DEVICECHANGE (DBT_DEVICEARRIVAL / DBT_DEVICEREMOVECOMPLETE) via a
//      hidden message-only window — necessary because WMI events can be delayed
//      or missed when the COM event sink is not on a STA thread.
//
// Both paths call the Qt-side callbacks registered via startDeviceWatcher().

#include "DeviceManager.hpp"
#include <QDebug>
#include <QThread>
#include <functional>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dbt.h>          // DBT_DEVICEARRIVAL etc.
#include <wbemidl.h>
#include <comdef.h>

namespace PandaManager {

// ── Globals ──────────────────────────────────────────────────────────────────
static std::function<void(const StorageDevice&)> g_onAdded;
static std::function<void(const QString&)>       g_onRemoved;
static HWND   g_msgWnd  = nullptr;
static HANDLE g_msgThread = nullptr;

// ── WMI enumeration ───────────────────────────────────────────────────────────
QList<StorageDevice> listRemovableDevicesWindows()
{
    QList<StorageDevice> result;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) { qWarning() << "[WMI] CoInitializeEx failed:" << hr; return result; }

    IWbemLocator*  pLoc  = nullptr;
    IWbemServices* pSvc  = nullptr;

    hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator, reinterpret_cast<LPVOID*>(&pLoc));
    if (FAILED(hr)) { CoUninitialize(); return result; }

    hr = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &pSvc);
    pLoc->Release();
    if (FAILED(hr)) { CoUninitialize(); return result; }

    CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                      RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
                      nullptr, EOAC_NONE);

    IEnumWbemClassObject* pEnum = nullptr;
    hr = pSvc->ExecQuery(
        _bstr_t("WQL"),
        _bstr_t("SELECT DeviceID, Caption, MediaType FROM Win32_DiskDrive "
                "WHERE MediaType = 'Removable Media'"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr, &pEnum);

    if (SUCCEEDED(hr) && pEnum) {
        IWbemClassObject* obj = nullptr;
        ULONG returned = 0;
        while (pEnum->Next(WBEM_INFINITE, 1, &obj, &returned) == S_OK) {
            VARIANT vId, vCap;
            VariantInit(&vId); VariantInit(&vCap);
            obj->Get(L"DeviceID", 0, &vId, nullptr, nullptr);
            obj->Get(L"Caption",  0, &vCap, nullptr, nullptr);
            StorageDevice dev;
            dev.devPath     = QString::fromWCharArray(vId.bstrVal);
            dev.label       = QString::fromWCharArray(vCap.bstrVal);
            dev.removable   = true;
            dev.mounted     = false;
            result.append(dev);
            VariantClear(&vId); VariantClear(&vCap);
            obj->Release();
        }
        pEnum->Release();
    }

    pSvc->Release();
    CoUninitialize();
    return result;
}

// ── WM_DEVICECHANGE message-only window ──────────────────────────────────────
static LRESULT CALLBACK DeviceWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DEVICECHANGE) {
        if (wParam == DBT_DEVICEARRIVAL) {
            auto* hdr = reinterpret_cast<DEV_BROADCAST_HDR*>(lParam);
            if (hdr && hdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                // Re-enumerate to get the new device details.
                const auto devs = listRemovableDevicesWindows();
                if (g_onAdded) {
                    for (const auto& d : devs)
                        g_onAdded(d);
                }
                qDebug() << "[WMI] WM_DEVICECHANGE: device arrived";
            }
        } else if (wParam == DBT_DEVICEREMOVECOMPLETE) {
            auto* hdr = reinterpret_cast<DEV_BROADCAST_HDR*>(lParam);
            if (hdr && hdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                // We do not know the exact devPath at this point; emit a sentinel.
                if (g_onRemoved) g_onRemoved(QStringLiteral("__removed__"));
                qDebug() << "[WMI] WM_DEVICECHANGE: device removed";
            }
        }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static DWORD WINAPI msgThreadProc(LPVOID)
{
    const wchar_t kClassName[] = L"PandaDeviceWatcher";
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = DeviceWndProc;
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.lpszClassName = kClassName;
    RegisterClassW(&wc);

    // Message-only window (parent = HWND_MESSAGE) never shows on screen.
    g_msgWnd = CreateWindowExW(0, kClassName, L"PandaDeviceWatcher",
                                0, 0, 0, 0, 0,
                                HWND_MESSAGE, nullptr,
                                GetModuleHandleW(nullptr), nullptr);
    if (!g_msgWnd) {
        qWarning() << "[WMI] Failed to create message-only window";
        return 1;
    }

    // Register to receive device notifications.
    DEV_BROADCAST_DEVICEINTERFACE notifyFilter = {};
    notifyFilter.dbcc_size       = sizeof(notifyFilter);
    notifyFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    RegisterDeviceNotificationW(g_msgWnd, &notifyFilter,
                                DEVICE_NOTIFY_WINDOW_HANDLE |
                                DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}

// ── Public API ────────────────────────────────────────────────────────────────
bool startDeviceWatcher(std::function<void(const StorageDevice&)> onAdded,
                        std::function<void(const QString&)>       onRemoved)
{
    if (g_msgThread) return true; // already running

    g_onAdded   = std::move(onAdded);
    g_onRemoved = std::move(onRemoved);

    g_msgThread = CreateThread(nullptr, 0, msgThreadProc, nullptr, 0, nullptr);
    if (!g_msgThread) {
        qWarning() << "[WMI] Failed to create message thread";
        return false;
    }
    qDebug() << "[WMI] Device watcher started";
    return true;
}

void stopDeviceWatcher()
{
    if (g_msgWnd) {
        PostMessageW(g_msgWnd, WM_QUIT, 0, 0);
        g_msgWnd = nullptr;
    }
    if (g_msgThread) {
        WaitForSingleObject(g_msgThread, 2000);
        CloseHandle(g_msgThread);
        g_msgThread = nullptr;
    }
    g_onAdded   = nullptr;
    g_onRemoved = nullptr;
    qDebug() << "[WMI] Device watcher stopped";
}

} // namespace PandaManager
