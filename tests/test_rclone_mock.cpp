// SPDX-License-Identifier: Apache-2.0
// PandaManager v1.0.0 — Rclone D-Bus Integration Tests (Mock)
// SPDX-FileCopyrightText: 2026 d4ruerk1 <https://github.com/d4ruerk1/PandaManager>

#include <QtTest>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// ─────────────────────────────────────────────────────────────────────────────
// MockRcloneDbus — risponde ai metodi JSON-RPC di rclone sul bus di sessione
// ─────────────────────────────────────────────────────────────────────────────
class MockRcloneService : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.rclone.Manager")

public:
    explicit MockRcloneService(QObject* parent = nullptr)
        : QObject(parent), m_mounted(false) {}

    bool isMounted() const { return m_mounted; }

public slots:
    // Simula rclone rc core/version
    QString coreVersion()
    {
        QJsonObject obj;
        obj["version"] = "v1.65.0-mock";
        obj["os"]      = "linux";
        obj["arch"]    = "amd64";
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }

    // Simula rclone rc mount/mount
    bool mount(const QString& remote, const QString& mountPoint)
    {
        if (remote.isEmpty() || mountPoint.isEmpty()) return false;
        if (remote.contains(';') || remote.contains("..")) return false;
        if (mountPoint.contains("..")) return false;
        m_mounted = true;
        m_lastRemote = remote;
        m_lastMountPoint = mountPoint;
        return true;
    }

    // Simula rclone rc mount/unmount
    bool unmount(const QString& mountPoint)
    {
        if (!m_mounted) return false;
        if (mountPoint != m_lastMountPoint) return false;
        m_mounted = false;
        return true;
    }

    // Simula rclone rc operations/list
    QString listRemote(const QString& remote, const QString& path)
    {
        Q_UNUSED(path)
        QJsonArray files;
        if (remote == "gdrive") {
            QJsonObject f1; f1["name"] = "documento.pdf"; f1["size"] = 102400; files.append(f1);
            QJsonObject f2; f2["name"] = "foto.jpg";      f2["size"] = 204800; files.append(f2);
        }
        QJsonObject result;
        result["list"] = files;
        return QJsonDocument(result).toJson(QJsonDocument::Compact);
    }

    // Simula rclone rc core/stats
    QString coreStats()
    {
        QJsonObject stats;
        stats["bytes"]          = 1048576;
        stats["speed"]          = 5242880.0;
        stats["transferring"]   = QJsonArray();
        stats["errors"]         = 0;
        return QJsonDocument(stats).toJson(QJsonDocument::Compact);
    }

private:
    bool    m_mounted      = false;
    QString m_lastRemote;
    QString m_lastMountPoint;
};

// ─────────────────────────────────────────────────────────────────────────────
// RcloneDbusTests
// ─────────────────────────────────────────────────────────────────────────────
class RcloneDbusTests : public QObject {
    Q_OBJECT

private:
    MockRcloneService* m_mock = nullptr;

private slots:

    void initTestCase()
    {
        m_mock = new MockRcloneService(this);
    }

    // ── Test: versione mock restituisce JSON valido ───────────────────────────
    void testCoreVersionParsing()
    {
        QString json = m_mock->coreVersion();
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        QVERIFY(!doc.isNull());
        QVERIFY(doc.isObject());

        QJsonObject obj = doc.object();
        QVERIFY(obj.contains("version"));
        QVERIFY(obj["version"].toString().startsWith("v"));
        QCOMPARE(obj["os"].toString(), QString("linux"));
    }

    // ── Test: mount con parametri validi ─────────────────────────────────────
    void testMountValidRemote()
    {
        bool ok = m_mock->mount("gdrive:", "/home/user/gdrive");
        QVERIFY(ok);
        QVERIFY(m_mock->isMounted());
    }

    // ── Test: mount con injection deve fallire ────────────────────────────────
    void testMountInjectionRejected()
    {
        MockRcloneService localMock;
        bool ok = localMock.mount("; rm -rf /", "/home/user/mnt");
        QVERIFY(!ok);
        QVERIFY(!localMock.isMounted());
    }

    // ── Test: mount con path traversal deve fallire ───────────────────────────
    void testMountPathTraversalRejected()
    {
        MockRcloneService localMock;
        bool ok = localMock.mount("gdrive:", "../../etc/passwd");
        QVERIFY(!ok);
    }

    // ── Test: unmount dopo mount ──────────────────────────────────────────────
    void testUnmountAfterMount()
    {
        MockRcloneService localMock;
        localMock.mount("onedrive:", "/home/user/onedrive");
        QVERIFY(localMock.isMounted());

        bool ok = localMock.unmount("/home/user/onedrive");
        QVERIFY(ok);
        QVERIFY(!localMock.isMounted());
    }

    // ── Test: unmount su mount point sbagliato fallisce ───────────────────────
    void testUnmountWrongMountPoint()
    {
        MockRcloneService localMock;
        localMock.mount("dropbox:", "/home/user/dropbox");
        bool ok = localMock.unmount("/home/user/OTHER");
        QVERIFY(!ok);
        QVERIFY(localMock.isMounted()); // ancora mounted
    }

    // ── Test: listRemote restituisce file per gdrive ──────────────────────────
    void testListRemoteGDrive()
    {
        QString json = m_mock->listRemote("gdrive", "/");
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        QVERIFY(!doc.isNull());

        QJsonArray list = doc.object()["list"].toArray();
        QCOMPARE(list.size(), 2);
        QCOMPARE(list[0].toObject()["name"].toString(), QString("documento.pdf"));
        QCOMPARE(list[1].toObject()["name"].toString(), QString("foto.jpg"));
    }

    // ── Test: listRemote su remote sconosciuto restituisce lista vuota ────────
    void testListRemoteUnknown()
    {
        QString json = m_mock->listRemote("unknown_remote", "/");
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        QVERIFY(!doc.isNull());
        QVERIFY(doc.object()["list"].toArray().isEmpty());
    }

    // ── Test: coreStats restituisce struttura valida ──────────────────────────
    void testCoreStatsParsing()
    {
        QString json = m_mock->coreStats();
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        QVERIFY(!doc.isNull());

        QJsonObject stats = doc.object();
        QVERIFY(stats.contains("bytes"));
        QVERIFY(stats.contains("speed"));
        QVERIFY(stats.contains("errors"));
        QCOMPARE(stats["errors"].toInt(), 0);
        QVERIFY(stats["speed"].toDouble() > 0.0);
    }

    void cleanupTestCase()
    {
        delete m_mock;
        m_mock = nullptr;
    }
};

QTEST_MAIN(RcloneDbusTests)
#include "test_rclone_mock.moc"
