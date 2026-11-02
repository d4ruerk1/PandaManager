// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <QObject>
#include <QString>
#include <QPixmap>

struct MediaMetadata {
    QString codec;
    QString container;
    int width       = 0;
    int height      = 0;
    int channels    = 0;
    int sampleRate  = 0;
    qint64 duration = 0; // ms
    QString artist;
    QString title;
    QString album;
};

class MediaPreviewEngine : public QObject {
    Q_OBJECT
public:
    enum class MediaType {
        Unknown, Image, RawPhoto, Video, Audio, Tracker
    };
    explicit MediaPreviewEngine(QObject* parent = nullptr);

    static MediaType classify(const QString& filePath);
    QPixmap thumbnail(const QString& filePath, int size = 256);
    MediaMetadata metadata(const QString& filePath);

signals:
    void thumbnailReady(const QString& filePath, const QPixmap& thumb);

private:
    static QPixmap makePlaceholder(const QString& filePath, int size);
};
