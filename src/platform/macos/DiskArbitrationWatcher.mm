// SPDX-License-Identifier: Apache-2.0
// macOS DiskArbitration device watcher — replaces UDisks2Manager + SolidDeviceWatcher.
// Uses DARegisterDiskAppearedCallback / DARegisterDiskDisappearedCallback.
// Properly signals DeviceManager and supports clean teardown.
#include "DeviceManager.hpp"
#include <QDebug>
#import <DiskArbitration/DiskArbitration.h>
#import <Foundation/Foundation.h>

namespace PandaManager {

static DASessionRef g_daSession = nullptr;
// Callback set from DeviceManager so hot-plug events can reach Qt land.
static std::function<void(const StorageDevice&)> g_onAdded;
static std::function<void(const QString&)>       g_onRemoved;

static StorageDevice deviceFromDisk(DADiskRef disk)
{
    StorageDevice dev;
    CFDictionaryRef desc = DADiskCopyDescription(disk);
    if (!desc) return dev;

    if (auto* bsd = static_cast<CFStringRef>(
            CFDictionaryGetValue(desc, kDADiskDescriptionMediaBSDNameKey)))
        dev.devPath = QLatin1String("/dev/") + QString::fromCFString(bsd);

    if (auto* vol = static_cast<CFStringRef>(
            CFDictionaryGetValue(desc, kDADiskDescriptionVolumeNameKey)))
        dev.label = QString::fromCFString(vol);

    if (auto* mp = static_cast<CFURLRef>(
            CFDictionaryGetValue(desc, kDADiskDescriptionVolumePathKey)))
        dev.mountPoint = QUrl::fromNSURL((__bridge NSURL*)mp).toLocalFile();

    dev.mounted    = !dev.mountPoint.isEmpty();
    dev.removable  = true;

    // Only consider actually removable media (skip internal disks).
    auto* removable = static_cast<CFBooleanRef>(
        CFDictionaryGetValue(desc, kDADiskDescriptionMediaRemovableKey));
    if (removable && !CFBooleanGetValue(removable)) {
        dev.removable = false;
    }

    CFRelease(desc);
    return dev;
}

static void diskAppeared(DADiskRef disk, void* /*context*/)
{
    StorageDevice dev = deviceFromDisk(disk);
    if (dev.devPath.isEmpty() || !dev.removable) return;
    qDebug() << "[DiskArbitration] Disk appeared:" << dev.devPath;
    if (g_onAdded) g_onAdded(dev);
}

static void diskDisappeared(DADiskRef disk, void* /*context*/)
{
    // Build a minimal StorageDevice just to get the devPath.
    StorageDevice dev = deviceFromDisk(disk);
    const QString path = dev.devPath.isEmpty()
        ? (DADiskGetBSDName(disk) ? QLatin1String("/dev/") + QString::fromLatin1(DADiskGetBSDName(disk)) : QString())
        : dev.devPath;
    if (path.isEmpty()) return;
    qDebug() << "[DiskArbitration] Disk disappeared:" << path;
    if (g_onRemoved) g_onRemoved(path);
}

/**
 * @brief Start the DiskArbitration watcher.
 * @param onAdded   Called (from the main run-loop) when a removable disk appears.
 * @param onRemoved Called (from the main run-loop) when a removable disk disappears.
 */
bool startDeviceWatcher(std::function<void(const StorageDevice&)> onAdded,
                        std::function<void(const QString&)>       onRemoved)
{
    if (g_daSession) return true; // already running

    g_onAdded   = std::move(onAdded);
    g_onRemoved = std::move(onRemoved);

    g_daSession = DASessionCreate(kCFAllocatorDefault);
    if (!g_daSession) {
        qWarning() << "[DiskArbitration] Failed to create DA session";
        return false;
    }
    DARegisterDiskAppearedCallback(g_daSession, nullptr, diskAppeared, nullptr);
    DARegisterDiskDisappearedCallback(g_daSession, diskDisappeared, nullptr);
    DASessionScheduleWithRunLoop(g_daSession, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
    qDebug() << "[DiskArbitration] Watcher started";
    return true;
}

/** @brief Stop the watcher and release the DA session. */
void stopDeviceWatcher()
{
    if (!g_daSession) return;
    DASessionUnscheduleFromRunLoop(g_daSession, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
    CFRelease(g_daSession);
    g_daSession = nullptr;
    g_onAdded   = nullptr;
    g_onRemoved = nullptr;
    qDebug() << "[DiskArbitration] Watcher stopped";
}

QList<StorageDevice> listRemovableDevicesMacOS()
{
    // Use `diskutil list -plist` for enumeration.
    QList<StorageDevice> result;
    @autoreleasepool {
        NSTask* task = [NSTask new];
        task.executableURL = [NSURL fileURLWithPath:@"/usr/sbin/diskutil"];
        task.arguments = @[@"list", @"-plist"];
        NSPipe* pipe = [NSPipe pipe];
        task.standardOutput = pipe;
        NSError* err = nil;
        if (![task launchAndReturnError:&err]) {
            qWarning() << "[DiskArbitration] diskutil failed:"
                       << (err ? QString::fromNSString(err.localizedDescription) : QStringLiteral("unknown"));
            return result;
        }
        NSData* data = [[pipe fileHandleForReading] readDataToEndOfFile];
        [task waitUntilExit];

        NSDictionary* plist = [NSPropertyListSerialization
            propertyListWithData:data
            options:NSPropertyListImmutable
            format:nil
            error:nil];
        // AllDisksAndPartitions is an array of dictionaries.
        for (NSDictionary* entry in plist[@"AllDisksAndPartitions"]) {
            NSString* diskId = entry[@"DeviceIdentifier"];
            if (!diskId) continue;
            StorageDevice dev;
            dev.devPath    = QLatin1String("/dev/") + QString::fromNSString(diskId);
            dev.removable  = true;
            // Try to get mount point from Partitions.
            for (NSDictionary* part in entry[@"Partitions"]) {
                NSString* mp = part[@"MountPoint"];
                if (mp && mp.length) {
                    dev.mountPoint = QString::fromNSString(mp);
                    dev.mounted    = true;
                    break;
                }
            }
            result.append(dev);
        }
    }
    return result;
}

} // namespace PandaManager
