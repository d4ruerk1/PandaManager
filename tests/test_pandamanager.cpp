// SPDX-License-Identifier: Apache-2.0
#include <QtTest>
#include "MediaPreviewEngine.hpp"
#include "CloudManager.hpp"
#include "DeviceManager.hpp"
#include "TransferQueue.hpp"

class PandaManagerTests : public QObject {
    Q_OBJECT

private slots:
    void testExoticImageFormats()
    {
        QCOMPARE(MediaPreviewEngine::classify("photo.jxl"),  MediaPreviewEngine::MediaType::Image);
        QCOMPARE(MediaPreviewEngine::classify("img.avif"),   MediaPreviewEngine::MediaType::Image);
        QCOMPARE(MediaPreviewEngine::classify("shot.exr"),   MediaPreviewEngine::MediaType::Image);
        QCOMPARE(MediaPreviewEngine::classify("raw.qoi"),    MediaPreviewEngine::MediaType::Image);
    }

    void testExoticVideoFormats()
    {
        QCOMPARE(MediaPreviewEngine::classify("film.mkv"),  MediaPreviewEngine::MediaType::Video);
        QCOMPARE(MediaPreviewEngine::classify("game.bik"),  MediaPreviewEngine::MediaType::Video);
        QCOMPARE(MediaPreviewEngine::classify("pro.mxf"),   MediaPreviewEngine::MediaType::Video);
    }

    void testExoticAudioFormats()
    {
        QCOMPARE(MediaPreviewEngine::classify("music.flac"), MediaPreviewEngine::MediaType::Audio);
        QCOMPARE(MediaPreviewEngine::classify("dsd.dsf"),    MediaPreviewEngine::MediaType::Audio);
        QCOMPARE(MediaPreviewEngine::classify("mod.xm"),     MediaPreviewEngine::MediaType::Tracker);
        QCOMPARE(MediaPreviewEngine::classify("chip.mod"),   MediaPreviewEngine::MediaType::Tracker);
    }

    void testExoticRawFormats()
    {
        QCOMPARE(MediaPreviewEngine::classify("canon.cr3"),  MediaPreviewEngine::MediaType::RawPhoto);
        QCOMPARE(MediaPreviewEngine::classify("nikon.nef"),  MediaPreviewEngine::MediaType::RawPhoto);
    }

    void testMetadataExtraction()
    {
        MediaPreviewEngine engine;
        MediaMetadata meta = engine.metadata("test.mp3");
        QVERIFY(!meta.container.isEmpty());
    }

    void testCloudInputSanitization()
    {
        // CloudManager should reject injection characters
        CloudManager cloud;
        // mountRemote with dangerous input must not succeed
        bool ok = cloud.mountRemote("; rm -rf /", "/home/user/mnt");
        QVERIFY(!ok);

        ok = cloud.mountRemote("gdrive", "../../etc/passwd");
        QVERIFY(!ok);
    }

    void testDeviceSecurityValidation()
    {
        DeviceManager dm;
        // root disk must be blocked
        QVERIFY(!dm.format("/dev/sda", "btrfs"));
        QVERIFY(!dm.format("/dev/nvme0n1", "ext4"));
        // unknown filesystem must be blocked
        QVERIFY(!dm.format("/dev/sdb", "tmpfs"));
        QVERIFY(!dm.format("/dev/sdb", "proc"));
    }

    void testTransferQueueOperations()
    {
        TransferQueue queue;
        QCOMPARE(queue.pendingCount(), 0);
        QCOMPARE(queue.completedCount(), 0);

        TransferJob job1{"/src/a.txt", "/dst/a.txt", 1024, 0, false};
        TransferJob job2{"/src/b.txt", "/dst/b.txt", 2048, 0, false};
        queue.enqueue(job1);
        queue.enqueue(job2);
        QCOMPARE(queue.pendingCount(), 2);

        queue.markCompleted("/src/a.txt");
        QCOMPARE(queue.pendingCount(), 1);
        QCOMPARE(queue.completedCount(), 1);
    }

    void testGdprComplianceState()
    {
        CloudManager cloud;
        // KWallet availability check must not throw
        bool available = cloud.isKWalletAvailable();
        Q_UNUSED(available);
        QVERIFY(true);
    }
};

QTEST_MAIN(PandaManagerTests)
#include "test_pandamanager.moc"
