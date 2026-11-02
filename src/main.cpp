// SPDX-License-Identifier: Apache-2.0
// PandaManager v1.0.0 — created by d4ruerk1
// https://github.com/d4ruerk1/PandaManager
#include <QApplication>
#include <QMainWindow>
#include <QSplitter>
#include <QTreeView>
#include <QListView>
#include <QColumnView>
#include <QFileSystemModel>
#include <QToolBar>
#include <QStatusBar>
#include <QProgressBar>
#include <QLabel>
#include <QShortcut>
#include <QKeySequence>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QDialog>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QScrollArea>
#include <QPixmap>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStackedWidget>
#include <QComboBox>
#include <QToolButton>
#include <QFrame>
#include <QDir>
#include <QStandardPaths>
#include <QPalette>
#include <QStyleHints>
#include <QFont>
#include <QSortFilterProxyModel>
#include <QMenu>
#include <QMenuBar>
#include <QHeaderView>
#include <QTextEdit>
#include <QSlider>
#include <QStyle>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QClipboard>
#include <QMimeData>
#include <QDrag>
#include <QStack>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QAbstractItemView>
#include "AuditLog.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// AddressBar — editable path bar (like Windows Explorer address bar)
// ─────────────────────────────────────────────────────────────────────────────
class AddressBar : public QWidget {
    Q_OBJECT
public:
    explicit AddressBar(QWidget* parent = nullptr) : QWidget(parent) {
        auto* lay = new QHBoxLayout(this);
        lay->setContentsMargins(2, 2, 2, 2);
        lay->setSpacing(4);

        // Breadcrumb display (shown when not editing)
        m_breadcrumb = new QWidget(this);
        m_breadcrumbLayout = new QHBoxLayout(m_breadcrumb);
        m_breadcrumbLayout->setContentsMargins(4, 0, 4, 0);
        m_breadcrumbLayout->setSpacing(0);

        // Editable line edit (shown when clicking to type)
        m_lineEdit = new QLineEdit(this);
        m_lineEdit->setPlaceholderText(QStringLiteral("Type a path and press Enter…"));
        m_lineEdit->setAccessibleName(QStringLiteral("Address bar"));
        m_lineEdit->setStyleSheet(QStringLiteral(
            "QLineEdit { border: 1px solid palette(highlight); border-radius: 2px; padding: 2px 4px; }"
            "QLineEdit:focus { border: 2px solid palette(highlight); }"));
        m_lineEdit->hide();

        lay->addWidget(m_breadcrumb, 1);
        lay->addWidget(m_lineEdit, 1);

        setStyleSheet(QStringLiteral(
            "AddressBar { background: palette(base); border: 1px solid palette(mid); border-radius: 3px; }"));

        // Click on breadcrumb area → switch to edit mode
        m_breadcrumb->installEventFilter(this);

        connect(m_lineEdit, &QLineEdit::returnPressed, this, [this] {
            const QString path = m_lineEdit->text().trimmed();
            if (QDir(path).exists()) {
                emit pathEntered(path);
                setPath(path);
            } else {
                m_lineEdit->setStyleSheet(QStringLiteral(
                    "QLineEdit { border: 2px solid red; border-radius: 2px; padding: 2px 4px; }"));
                QTimer::singleShot(1500, m_lineEdit, [this] {
                    m_lineEdit->setStyleSheet(QStringLiteral(
                        "QLineEdit { border: 2px solid palette(highlight); border-radius: 2px; padding: 2px 4px; }"));
                });
            }
            switchToBreadcrumb();
        });

        // Escape → cancel editing
        auto* esc = new QShortcut(QKeySequence(Qt::Key_Escape), m_lineEdit);
        connect(esc, &QShortcut::activated, this, &AddressBar::switchToBreadcrumb);
    }

    void setPath(const QString& path) {
        m_currentPath = path;
        m_lineEdit->setText(path);
        rebuildBreadcrumb();
    }

    bool eventFilter(QObject* obj, QEvent* event) override {
        if (obj == m_breadcrumb && event->type() == QEvent::MouseButtonPress) {
            switchToEdit();
            return true;
        }
        return QWidget::eventFilter(obj, event);
    }

signals:
    void pathEntered(const QString& path);
    void pathSegmentClicked(const QString& path);

private slots:
    void switchToEdit() {
        m_breadcrumb->hide();
        m_lineEdit->show();
        m_lineEdit->setText(m_currentPath);
        m_lineEdit->setFocus();
        m_lineEdit->selectAll();
    }

    void switchToBreadcrumb() {
        m_lineEdit->hide();
        m_breadcrumb->show();
    }

private:
    void rebuildBreadcrumb() {
        // Remove old widgets
        QLayoutItem* item;
        while ((item = m_breadcrumbLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }

        const QChar sep = QLatin1Char('/');
        QStringList parts = m_currentPath.split(sep, Qt::SkipEmptyParts);

        // Root button
        auto* rootBtn = makeSegmentButton(QStringLiteral("⊟ /"), QStringLiteral("/"));
        m_breadcrumbLayout->addWidget(rootBtn);

        QString accumulated = QStringLiteral("/");
        for (const QString& part : parts) {
            // Separator chevron
            auto* chevron = new QLabel(QStringLiteral(" › "), m_breadcrumb);
            chevron->setStyleSheet(QStringLiteral("color: palette(mid); font-size: 10px;"));
            m_breadcrumbLayout->addWidget(chevron);

            accumulated += part + QLatin1Char('/');
            auto* btn = makeSegmentButton(part, accumulated);
            m_breadcrumbLayout->addWidget(btn);
        }
        m_breadcrumbLayout->addStretch();
    }

    QToolButton* makeSegmentButton(const QString& label, const QString& path) {
        auto* btn = new QToolButton(m_breadcrumb);
        btn->setText(label);
        btn->setAutoRaise(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QStringLiteral(
            "QToolButton { padding: 1px 4px; border-radius: 2px; font-size: 11px; }"
            "QToolButton:hover { background: palette(button); }"
            "QToolButton:focus { outline: 2px solid palette(highlight); outline-offset: 1px; }"));
        btn->setAccessibleName(QStringLiteral("Navigate to %1").arg(label));
        connect(btn, &QToolButton::clicked, this, [this, path] {
            emit pathSegmentClicked(path);
        });
        return btn;
    }

    QWidget*        m_breadcrumb        = nullptr;
    QHBoxLayout*    m_breadcrumbLayout  = nullptr;
    QLineEdit*      m_lineEdit          = nullptr;
    QString         m_currentPath;
};

// ─────────────────────────────────────────────────────────────────────────────
// QuickLookDialog — preview overlay with media controls
// ─────────────────────────────────────────────────────────────────────────────
class QuickLookDialog : public QDialog {
    Q_OBJECT
public:
    explicit QuickLookDialog(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle(QStringLiteral("Anteprima rapida — QuickLook"));
        setMinimumSize(720, 520);
        setAttribute(Qt::WA_DeleteOnClose, false);
        setModal(false);
        setAccessibleName(QStringLiteral("QuickLook preview dialog"));
        setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint);

        // Stack: 0=image, 1=text, 2=video/audio, 3=no preview
        m_stack = new QStackedWidget(this);

        // ── Image view
        m_imgLabel = new QLabel(this);
        m_imgLabel->setAlignment(Qt::AlignCenter);
        m_imgLabel->setStyleSheet(QStringLiteral("background: #1a1a1a;"));
        m_imgLabel->setMinimumSize(1, 1);
        m_imgLabel->setAccessibleDescription(QStringLiteral("Image preview"));

        // ── Text view
        m_textEdit = new QTextEdit(this);
        m_textEdit->setReadOnly(true);
        m_textEdit->setFontFamily(QStringLiteral("Monospace"));

        // ── Video/audio view
        auto* mediaWidget = new QWidget(this);
        auto* mediaLay    = new QVBoxLayout(mediaWidget);
        mediaLay->setContentsMargins(0, 0, 0, 0);
        m_videoWidget = new QVideoWidget(mediaWidget);
        m_videoWidget->setStyleSheet(QStringLiteral("background: #000;"));
        m_player = new QMediaPlayer(this);
        m_player->setVideoOutput(m_videoWidget);
        mediaLay->addWidget(m_videoWidget, 1);

        // Simple controls
        auto* controls = new QWidget(mediaWidget);
        auto* ctrlLay  = new QHBoxLayout(controls);
        ctrlLay->setContentsMargins(4, 4, 4, 4);
        m_playPauseBtn = new QPushButton(style()->standardIcon(QStyle::SP_MediaPlay), QString(), controls);
        m_playPauseBtn->setToolTip(QStringLiteral("Play / Pause (P)"));
        m_playPauseBtn->setFixedSize(32, 32);
        m_seekSlider = new QSlider(Qt::Horizontal, controls);
        m_seekSlider->setRange(0, 0);
        m_timeLabel = new QLabel(QStringLiteral("0:00 / 0:00"), controls);
        ctrlLay->addWidget(m_playPauseBtn);
        ctrlLay->addWidget(m_seekSlider, 1);
        ctrlLay->addWidget(m_timeLabel);
        mediaLay->addWidget(controls);

        connect(m_playPauseBtn, &QPushButton::clicked, this, [this] {
            if (m_player->playbackState() == QMediaPlayer::PlayingState)
                m_player->pause();
            else
                m_player->play();
        });
        connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState s) {
            m_playPauseBtn->setIcon(style()->standardIcon(
                s == QMediaPlayer::PlayingState ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay));
        });
        connect(m_player, &QMediaPlayer::durationChanged, this, [this](qint64 dur) {
            m_seekSlider->setRange(0, static_cast<int>(dur / 1000));
        });
        connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64 pos) {
            m_seekSlider->blockSignals(true);
            m_seekSlider->setValue(static_cast<int>(pos / 1000));
            m_seekSlider->blockSignals(false);
            const int sec = static_cast<int>(pos / 1000);
            const int dur = static_cast<int>(m_player->duration() / 1000);
            m_timeLabel->setText(QStringLiteral("%1:%2 / %3:%4")
                .arg(sec / 60).arg(sec % 60, 2, 10, QLatin1Char('0'))
                .arg(dur / 60).arg(dur % 60, 2, 10, QLatin1Char('0')));
        });
        connect(m_seekSlider, &QSlider::sliderMoved, m_player, [this](int v) {
            m_player->setPosition(qint64(v) * 1000);
        });

        // ── No preview
        m_noPreview = new QLabel(this);
        m_noPreview->setAlignment(Qt::AlignCenter);
        m_noPreview->setStyleSheet(QStringLiteral("color: palette(mid); font-size: 14px;"));

        m_stack->addWidget(m_imgLabel);   // 0
        m_stack->addWidget(m_textEdit);   // 1
        m_stack->addWidget(mediaWidget);  // 2
        m_stack->addWidget(m_noPreview);  // 3

        // Bottom bar
        m_fileInfoLabel = new QLabel(this);
        m_fileInfoLabel->setStyleSheet(QStringLiteral("color: palette(mid); font-size: 11px; padding: 2px 6px;"));

        auto* closeBtn = new QPushButton(QStringLiteral("Chiudi"), this);
        closeBtn->setDefault(true);
        connect(closeBtn, &QPushButton::clicked, this, &QDialog::hide);

        auto* bottomBar = new QWidget(this);
        auto* bottomLay = new QHBoxLayout(bottomBar);
        bottomLay->addWidget(m_fileInfoLabel, 1);
        bottomLay->addWidget(closeBtn);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(6, 6, 6, 6);
        layout->setSpacing(4);
        layout->addWidget(m_stack, 1);
        layout->addWidget(bottomBar);

        // Shortcuts
        auto* spaceKey = new QShortcut(QKeySequence(Qt::Key_Space), this);
        connect(spaceKey, &QShortcut::activated, this, &QDialog::hide);
        auto* escKey = new QShortcut(QKeySequence(Qt::Key_Escape), this);
        connect(escKey, &QShortcut::activated, this, &QDialog::hide);
        auto* pKey = new QShortcut(QKeySequence(Qt::Key_P), this);
        connect(pKey, &QShortcut::activated, m_playPauseBtn, &QPushButton::click);
    }

    void previewFile(const QString& filePath) {
        m_player->stop();
        const QFileInfo fi(filePath);
        setWindowTitle(fi.fileName());

        // File info line
        const double sizeMB = fi.size() / (1024.0 * 1024.0);
        const QString sizeStr = sizeMB >= 1.0
            ? QStringLiteral("%1 MB").arg(sizeMB, 0, 'f', 1)
            : QStringLiteral("%1 KB").arg(fi.size() / 1024.0, 0, 'f', 1);
        m_fileInfoLabel->setText(QStringLiteral("%1  •  %2  •  Modificato: %3")
            .arg(fi.fileName(), sizeStr,
                 fi.lastModified().toString(QStringLiteral("dd/MM/yyyy HH:mm"))));

        QMimeDatabase db;
        const QString mime = db.mimeTypeForFile(filePath).name();

        if (mime.startsWith(QStringLiteral("image/"))) {
            QPixmap pix(filePath);
            if (!pix.isNull()) {
                // Scale to fit maintaining aspect ratio, within window
                m_imgLabel->setPixmap(pix.scaled(
                    m_imgLabel->size().expandedTo(QSize(640, 440)),
                    Qt::KeepAspectRatio, Qt::SmoothTransformation));
                m_stack->setCurrentIndex(0);
                return;
            }
        }
        if (mime.startsWith(QStringLiteral("video/")) || mime.startsWith(QStringLiteral("audio/"))) {
            m_player->setSource(QUrl::fromLocalFile(filePath));
            m_player->play();
            m_stack->setCurrentIndex(2);
            return;
        }
        if (mime.startsWith(QStringLiteral("text/")) || mime == QStringLiteral("application/json")
            || mime == QStringLiteral("application/xml")) {
            QFile f(filePath);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                constexpr qint64 kLimit = 32768;
                QString text = QString::fromUtf8(f.read(kLimit));
                if (fi.size() > kLimit)
                    text += QStringLiteral("\n\n— [Anteprima troncata a 32 KB] —");
                m_textEdit->setPlainText(text);
                m_stack->setCurrentIndex(1);
                return;
            }
        }
        m_noPreview->setText(QStringLiteral("Nessuna anteprima disponibile\n\n%1").arg(mime));
        m_stack->setCurrentIndex(3);
    }

private:
    QStackedWidget* m_stack         = nullptr;
    QLabel*         m_imgLabel      = nullptr;
    QTextEdit*      m_textEdit      = nullptr;
    QVideoWidget*   m_videoWidget   = nullptr;
    QMediaPlayer*   m_player        = nullptr;
    QLabel*         m_noPreview     = nullptr;
    QPushButton*    m_playPauseBtn  = nullptr;
    QSlider*        m_seekSlider    = nullptr;
    QLabel*         m_timeLabel     = nullptr;
    QLabel*         m_fileInfoLabel = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
// PandaFilePane — single file panel with context menu and column config
// ─────────────────────────────────────────────────────────────────────────────
class PandaFilePane : public QWidget {
    Q_OBJECT
public:
    explicit PandaFilePane(QWidget* parent = nullptr) : QWidget(parent) {
        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(0);

        m_model = new QFileSystemModel(this);
        m_model->setRootPath(QStringLiteral("/"));
        m_model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

        m_proxy = new QSortFilterProxyModel(this);
        m_proxy->setSourceModel(m_model);
        m_proxy->setFilterKeyColumn(0);
        m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
        m_proxy->setRecursiveFilteringEnabled(true);

        m_view = new QTreeView(this);
        m_view->setModel(m_proxy);
        m_view->setRootIndex(m_proxy->mapFromSource(m_model->index(QDir::homePath())));
        m_view->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_view->setDragDropMode(QAbstractItemView::DragDrop);
        m_view->setDragDropOverwriteMode(false);
        m_view->setDefaultDropAction(Qt::MoveAction);
        m_view->setAcceptDrops(true);
        m_view->setDropIndicatorShown(true);
        // Accept drops from outside (e.g. file manager, desktop) as well.
        m_view->viewport()->setAcceptDrops(true);
        m_view->viewport()->installEventFilter(this);
        m_view->setSortingEnabled(true);
        m_view->setAlternatingRowColors(true);
        m_view->setAnimated(true);
        m_view->setUniformRowHeights(true);
        m_view->header()->setSectionResizeMode(QHeaderView::Interactive);
        m_view->header()->setDefaultSectionSize(160);
        m_view->setContextMenuPolicy(Qt::CustomContextMenu);

        // Configure columns widths (like Windows Explorer)
        m_view->setColumnWidth(0, 280); // Name
        m_view->setColumnWidth(1, 90);  // Size
        m_view->setColumnWidth(2, 140); // Type
        m_view->setColumnWidth(3, 140); // Date modified

        // Rename: F2
        auto* f2 = new QShortcut(QKeySequence(Qt::Key_F2), m_view);
        connect(f2, &QShortcut::activated, this, &PandaFilePane::startRename);

        // Delete: Del
        auto* del = new QShortcut(QKeySequence(Qt::Key_Delete), m_view);
        connect(del, &QShortcut::activated, this, &PandaFilePane::deleteSelected);

        // Copy: Ctrl+C
        auto* ctrlC = new QShortcut(QKeySequence::Copy, m_view);
        connect(ctrlC, &QShortcut::activated, this, [this] { copySelectedToClipboard(false); });

        // Cut: Ctrl+X
        auto* ctrlX = new QShortcut(QKeySequence::Cut, m_view);
        connect(ctrlX, &QShortcut::activated, this, [this] { copySelectedToClipboard(true); });

        // Paste: Ctrl+V
        auto* ctrlV = new QShortcut(QKeySequence::Paste, m_view);
        connect(ctrlV, &QShortcut::activated, this, &PandaFilePane::pasteFromClipboard);

        connect(m_view, &QTreeView::activated, this, [this](const QModelIndex& idx) {
            const QModelIndex src = m_proxy->mapToSource(idx);
            const QString path = m_model->filePath(src);
            if (m_model->isDir(src)) {
                navigateTo(path);
            } else {
                emit fileActivated(path);
            }
        });

        connect(m_view, &QTreeView::customContextMenuRequested,
                this, &PandaFilePane::showContextMenu);

        lay->addWidget(m_view);
        m_currentPath = QDir::homePath();
    }

    QFileSystemModel* model() const { return m_model; }
    QSortFilterProxyModel* proxy() const { return m_proxy; }
    QTreeView* view() const { return m_view; }
    QString currentPath() const { return m_currentPath; }

    void navigateTo(const QString& path) {
        if (!QDir(path).exists()) return;
        // Push history
        m_backStack.push(m_currentPath);
        m_forwardStack.clear();
        doNavigate(path);
    }

    void goBack() {
        if (m_backStack.isEmpty()) return;
        m_forwardStack.push(m_currentPath);
        doNavigate(m_backStack.pop());
    }

    void goForward() {
        if (m_forwardStack.isEmpty()) return;
        m_backStack.push(m_currentPath);
        doNavigate(m_forwardStack.pop());
    }

    void goUp() {
        QDir dir(m_currentPath);
        if (dir.cdUp()) navigateTo(dir.absolutePath());
    }

    void setFilter(const QString& pattern) {
        m_proxy->setFilterWildcard(pattern);
        m_proxy->invalidate();
    }

    void toggleHidden() {
        const auto filter = (m_model->filter() & QDir::Hidden)
            ? m_model->filter() & ~QDir::Hidden
            : m_model->filter() | QDir::Hidden;
        m_model->setFilter(filter);
    }

    bool canGoBack()    const { return !m_backStack.isEmpty(); }
    bool canGoForward() const { return !m_forwardStack.isEmpty(); }

signals:
    void pathChanged(const QString& path);
    void fileActivated(const QString& path);
    void selectionInfo(const QString& info);

private slots:
    void showContextMenu(const QPoint& pos) {
        const QModelIndex proxyIdx = m_view->indexAt(pos);
        QMenu menu(m_view);

        if (proxyIdx.isValid()) {
            const QModelIndex src = m_proxy->mapToSource(proxyIdx);
            const QString path = m_model->filePath(src);
            const bool isDir = m_model->isDir(src);

            if (isDir) {
                auto* openAct = menu.addAction(style()->standardIcon(QStyle::SP_DirOpenIcon),
                    QStringLiteral("Apri"));
                connect(openAct, &QAction::triggered, this, [this, path] { navigateTo(path); });
            } else {
                auto* openAct = menu.addAction(style()->standardIcon(QStyle::SP_FileIcon),
                    QStringLiteral("Apri"));
                connect(openAct, &QAction::triggered, this, [path] {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
                });
                auto* qlAct = menu.addAction(QStringLiteral("🔍 Anteprima rapida (Spazio)"));
                connect(qlAct, &QAction::triggered, this, [this, path] { emit fileActivated(path); });
            }

            menu.addSeparator();
            auto* cutAct  = menu.addAction(QStringLiteral("Taglia"));
            auto* copyAct = menu.addAction(QStringLiteral("Copia"));
            connect(cutAct,  &QAction::triggered, this, [this] { copySelectedToClipboard(true); });
            connect(copyAct, &QAction::triggered, this, [this] { copySelectedToClipboard(false); });
            menu.addSeparator();
            auto* renameAct = menu.addAction(QStringLiteral("Rinomina (F2)"));
            connect(renameAct, &QAction::triggered, this, &PandaFilePane::startRename);
            auto* delAct = menu.addAction(style()->standardIcon(QStyle::SP_TrashIcon),
                QStringLiteral("Elimina (Canc)"));
            connect(delAct, &QAction::triggered, this, &PandaFilePane::deleteSelected);
            menu.addSeparator();
        }

        auto* pasteAct = menu.addAction(QStringLiteral("Incolla"));
        pasteAct->setEnabled(!QApplication::clipboard()->mimeData()->urls().isEmpty());
        connect(pasteAct, &QAction::triggered, this, &PandaFilePane::pasteFromClipboard);
        menu.addSeparator();
        auto* newFolderAct = menu.addAction(style()->standardIcon(QStyle::SP_FileDialogNewFolder),
            QStringLiteral("Nuova cartella"));
        connect(newFolderAct, &QAction::triggered, this, &PandaFilePane::createNewFolder);

        menu.exec(m_view->viewport()->mapToGlobal(pos));
    }

    void startRename() {
        const QModelIndex idx = m_view->currentIndex();
        if (idx.isValid()) m_view->edit(idx);
    }

    void deleteSelected() {
        const auto selected = m_view->selectionModel()->selectedRows();
        if (selected.isEmpty()) return;
        const int ans = QMessageBox::question(this,
            QStringLiteral("Elimina"),
            QStringLiteral("Eliminare %1 elemento/i selezionato/i?").arg(selected.size()),
            QMessageBox::Yes | QMessageBox::No);
        if (ans != QMessageBox::Yes) return;
        for (const auto& proxyIdx : selected) {
            const QModelIndex src = m_proxy->mapToSource(proxyIdx);
            const QString path = m_model->filePath(src);
            if (m_model->isDir(src))
                QDir(path).removeRecursively();
            else
                QFile::remove(path);
        }
    }

    void copySelectedToClipboard(bool cut) {
        const auto selected = m_view->selectionModel()->selectedRows();
        if (selected.isEmpty()) return;
        QList<QUrl> urls;
        for (const auto& proxyIdx : selected) {
            const QModelIndex src = m_proxy->mapToSource(proxyIdx);
            urls << QUrl::fromLocalFile(m_model->filePath(src));
        }
        auto* mime = new QMimeData();
        mime->setUrls(urls);
        if (cut)
            mime->setData(QStringLiteral("x-special/gnome-copied-files"),
                          QByteArray("cut\n") + urls.first().toLocalFile().toUtf8());
        QApplication::clipboard()->setMimeData(mime);
    }

    void pasteFromClipboard() {
        const QMimeData* mime = QApplication::clipboard()->mimeData();
        if (!mime->hasUrls()) return;
        const bool cut = mime->data(QStringLiteral("x-special/gnome-copied-files"))
                             .startsWith("cut");
        for (const QUrl& url : mime->urls()) {
            const QString src = url.toLocalFile();
            const QString dst = m_currentPath + QLatin1Char('/') + QFileInfo(src).fileName();
            if (cut)
                QFile::rename(src, dst);
            else
                QFile::copy(src, dst);
        }
        if (cut) QApplication::clipboard()->clear();
    }

    void createNewFolder() {
        bool ok = false;
        const QString name = QInputDialog::getText(this,
            QStringLiteral("Nuova cartella"),
            QStringLiteral("Nome cartella:"), QLineEdit::Normal,
            QStringLiteral("Nuova cartella"), &ok);
        if (!ok || name.trimmed().isEmpty()) return;
        QDir(m_currentPath).mkdir(name.trimmed());
    }

    // ── Drag & drop event filter (installed on m_view->viewport()) ────────────
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (obj != m_view->viewport()) return QWidget::eventFilter(obj, event);

        switch (event->type()) {
        case QEvent::DragEnter: {
            auto* e = static_cast<QDragEnterEvent*>(event);
            if (e->mimeData()->hasUrls()) {
                e->setDropAction(e->keyboardModifiers() & Qt::ControlModifier
                                 ? Qt::CopyAction : Qt::MoveAction);
                e->accept();
            } else {
                e->ignore();
            }
            return true;
        }
        case QEvent::DragMove: {
            auto* e = static_cast<QDragMoveEvent*>(event);
            if (e->mimeData()->hasUrls()) {
                e->setDropAction(e->keyboardModifiers() & Qt::ControlModifier
                                 ? Qt::CopyAction : Qt::MoveAction);
                e->accept();
            } else {
                e->ignore();
            }
            return true;
        }
        case QEvent::Drop: {
            auto* e = static_cast<QDropEvent*>(event);
            if (!e->mimeData()->hasUrls()) { e->ignore(); return true; }
            const bool copy = (e->keyboardModifiers() & Qt::ControlModifier) ||
                              (e->dropAction() == Qt::CopyAction);
            // Determine destination: drop on a folder → use it; otherwise use current dir.
            QString destDir = m_currentPath;
            const QModelIndex proxyIdx = m_view->indexAt(e->position().toPoint());
            if (proxyIdx.isValid()) {
                const QModelIndex srcIdx = m_proxy->mapToSource(proxyIdx);
                if (m_model->isDir(srcIdx))
                    destDir = m_model->filePath(srcIdx);
            }
            dropFiles(e->mimeData()->urls(), destDir, copy);
            e->acceptProposedAction();
            return true;
        }
        default:
            break;
        }
        return QWidget::eventFilter(obj, event);
    }

private:
    // ── Drop helper ────────────────────────────────────────────────────────────
    void dropFiles(const QList<QUrl>& urls, const QString& destDir, bool copy)
    {
        for (const QUrl& url : urls) {
            if (!url.isLocalFile()) continue;
            const QString src  = url.toLocalFile();
            const QString name = QFileInfo(src).fileName();
            QString dst  = destDir + QLatin1Char('/') + name;

            // Avoid overwriting: append (1), (2) … if needed.
            if (QFileInfo::exists(dst)) {
                const QString base = QFileInfo(src).completeBaseName();
                const QString ext  = QFileInfo(src).suffix();
                int n = 1;
                do {
                    dst = ext.isEmpty()
                        ? QStringLiteral("%1/%2 (%3)").arg(destDir, base).arg(n++)
                        : QStringLiteral("%1/%2 (%3).%4").arg(destDir, base).arg(n++).arg(ext);
                } while (QFileInfo::exists(dst) && n < 1000);
            }

            bool ok = false;
            if (QFileInfo(src).isDir()) {
                ok = copy ? copyDirRecursive(src, dst)
                          : QDir().rename(src, dst);
            } else {
                ok = copy ? QFile::copy(src, dst)
                          : QFile::rename(src, dst);
            }
            if (!ok)
                qWarning() << "[DragDrop] Failed to" << (copy ? "copy" : "move")
                           << src << "→" << dst;
        }
    }

    static bool copyDirRecursive(const QString& src, const QString& dst)
    {
        QDir srcDir(src);
        if (!srcDir.exists()) return false;
        if (!QDir().mkpath(dst)) return false;
        const auto entries = srcDir.entryInfoList(
            QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        for (const QFileInfo& fi : entries) {
            const QString dstPath = dst + QLatin1Char('/') + fi.fileName();
            if (fi.isDir()) {
                if (!copyDirRecursive(fi.filePath(), dstPath)) return false;
            } else {
                if (!QFile::copy(fi.filePath(), dstPath)) return false;
            }
        }
        return true;
    }

    void doNavigate(const QString& path) {
        m_currentPath = path;
        const QModelIndex srcIdx = m_model->index(path);
        m_view->setRootIndex(m_proxy->mapFromSource(srcIdx));
        emit pathChanged(path);
        // Update selection info
        emit selectionInfo(QStringLiteral("0 elementi selezionati"));
    }

    QFileSystemModel*       m_model       = nullptr;
    QSortFilterProxyModel*  m_proxy       = nullptr;
    QTreeView*              m_view        = nullptr;
    QString                 m_currentPath;
    QStack<QString>         m_backStack;
    QStack<QString>         m_forwardStack;
};

// ─────────────────────────────────────────────────────────────────────────────
// PandaMainWindow
// ─────────────────────────────────────────────────────────────────────────────
class PandaMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit PandaMainWindow(QWidget* parent = nullptr)
        : QMainWindow(parent)
        , m_quickLook(new QuickLookDialog(this))
    {
        setWindowTitle(QStringLiteral("PandaManager"));
        setWindowIcon(QIcon(QStringLiteral(":/icons/panda_icon.png")));
        setMinimumSize(1100, 640);

        setupMenuBar();
        setupToolbar();
        setupAddressBar();
        setupSidebar();
        setupCentralArea();
        setupStatusBar();
        setupGlobalShortcuts();
        applyWindowsExplorerStyle();

        connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged,
                this, &PandaMainWindow::applyWindowsExplorerStyle);

        // Subtle one-time status message
        QTimer::singleShot(4000, this, [this] {
            m_statusLabel->setText(
                QStringLiteral("♥ PandaManager v1.0.0 — supporta il progetto su Ko-fi!"));
            QTimer::singleShot(7000, this, [this] { updateStatusBar(); });
        });
    }

private slots:
    // ── Style / theme ─────────────────────────────────────────────────────────
    void applyWindowsExplorerStyle() {
        const bool dark = (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark);
        // Use system palette — only add subtle Explorer-like styling
        const QString base = dark
            ? QStringLiteral(
                "QMainWindow { background: #202020; }"
                "QDockWidget { background: #252525; }"
                "QDockWidget::title { background: #1e1e1e; padding: 4px 8px; font-weight: 600; }"
                "QToolBar { background: #2d2d2d; border-bottom: 1px solid #3a3a3a; spacing: 2px; }"
                "QStatusBar { background: #1a1a1a; color: #aaaaaa; font-size: 11px; }"
                "QListWidget { background: #252525; border: none; }"
                "QListWidget::item { padding: 4px 8px; border-radius: 4px; }"
                "QListWidget::item:selected { background: #0078d4; color: white; }"
                "QListWidget::item:hover { background: #333333; }"
                "QTreeView { background: #1e1e1e; alternate-background-color: #252525; border: none; }"
                "QTreeView::item { padding: 2px; }"
                "QTreeView::item:selected { background: #0078d4; color: white; }"
                "QTreeView::item:hover { background: #2a2a2a; }"
                "QHeaderView::section { background: #2d2d2d; border: none; border-right: 1px solid #3a3a3a; "
                "                        padding: 4px 8px; font-weight: 600; }")
            : QStringLiteral(
                "QMainWindow { background: #f3f3f3; }"
                "QDockWidget { background: #f3f3f3; }"
                "QDockWidget::title { background: #e5e5e5; padding: 4px 8px; font-weight: 600; }"
                "QToolBar { background: #f9f9f9; border-bottom: 1px solid #dcdcdc; spacing: 2px; }"
                "QStatusBar { background: #f0f0f0; color: #555555; font-size: 11px; }"
                "QListWidget { background: #f3f3f3; border: none; }"
                "QListWidget::item { padding: 4px 8px; border-radius: 4px; }"
                "QListWidget::item:selected { background: #cce4f7; color: #000; }"
                "QListWidget::item:hover { background: #e5f1fb; }"
                "QTreeView { background: white; alternate-background-color: #fafafa; border: none; }"
                "QTreeView::item { padding: 2px; }"
                "QTreeView::item:selected { background: #cce4f7; color: #000; }"
                "QTreeView::item:hover { background: #e5f1fb; }"
                "QHeaderView::section { background: #f3f3f3; border: none; border-right: 1px solid #dcdcdc; "
                "                        padding: 4px 8px; font-weight: 600; }");
        setStyleSheet(base);
    }

    // ── Navigation helpers ─────────────────────────────────────────────────────
    void onPanePathChanged(const QString& path) {
        m_addressBar->setPath(path);
        setWindowTitle(QStringLiteral("PandaManager — %1").arg(path));
        updateNavButtons();
        updateStatusBar();
    }

    void updateNavButtons() {
        m_actBack->setEnabled(m_activePane->canGoBack());
        m_actForward->setEnabled(m_activePane->canGoForward());
    }

    void updateStatusBar() {
        const QString path = m_activePane->currentPath();
        QDir dir(path);
        const int count = static_cast<int>(dir.entryList(
            QDir::AllEntries | QDir::NoDotAndDotDot).size());
        m_statusLabel->setText(QStringLiteral("%1 elementi").arg(count));
        m_pathLabel->setText(path);
    }

    void toggleSplitPane() {
        m_splitMode = !m_splitMode;
        m_rightPane->setVisible(m_splitMode);
        if (m_splitMode && !m_rightPane->view()->rootIndex().isValid())
            m_rightPane->navigateTo(m_leftPane->currentPath());
        m_actSplit->setChecked(m_splitMode);
    }

    void onFileActivated(const QString& path) {
        // Space / double-click on file → QuickLook
        m_quickLook->previewFile(path);
        m_spaceShortcut->setEnabled(false);
        connect(m_quickLook, &QDialog::finished, this, [this] {
            m_spaceShortcut->setEnabled(true);
        }, Qt::SingleShotConnection);
        m_quickLook->show();
        m_quickLook->raise();
    }

    void triggerQuickLook() {
        // Space → preview selected file in active pane
        const QModelIndex proxyIdx = m_activePane->view()->currentIndex();
        if (!proxyIdx.isValid()) return;
        const QModelIndex srcIdx = m_activePane->proxy()->mapToSource(proxyIdx);
        const QString path = m_activePane->model()->filePath(srcIdx);
        if (QFileInfo(path).isFile()) onFileActivated(path);
    }

private:
    // ── Menu bar ───────────────────────────────────────────────────────────────
    void setupMenuBar() {
        // File menu
        auto* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
        fileMenu->addAction(style()->standardIcon(QStyle::SP_FileDialogNewFolder),
            QStringLiteral("&Nuova cartella\tCtrl+Shift+N"), this, [this] {
                // delegate to active pane's context menu action
            });
        fileMenu->addSeparator();
        fileMenu->addAction(QStringLiteral("&Chiudi"), this, &QMainWindow::close, QKeySequence::Close);

        // View menu
        auto* viewMenu = menuBar()->addMenu(QStringLiteral("&Visualizza"));
        m_actSplit = viewMenu->addAction(QStringLiteral("Doppio pannello (&F3)"));
        m_actSplit->setCheckable(true);
        connect(m_actSplit, &QAction::triggered, this, &PandaMainWindow::toggleSplitPane);

        viewMenu->addSeparator();
        auto* hiddenAct = viewMenu->addAction(QStringLiteral("Mostra file nascosti\tCtrl+H"));
        hiddenAct->setCheckable(true);
        connect(hiddenAct, &QAction::triggered, this, [this] {
            m_leftPane->toggleHidden();
            m_rightPane->toggleHidden();
        });

        // Help menu
        auto* helpMenu = menuBar()->addMenu(QStringLiteral("&?"));
        helpMenu->addAction(QStringLiteral("Supporta su Ko-fi ☕"), this, [] {
            QDesktopServices::openUrl(QUrl(QStringLiteral("https://ko-fi.com/d4ruerk1")));
        });
        helpMenu->addAction(QStringLiteral("Informazioni su PandaManager"), this, [this] {
            QMessageBox::about(this, QStringLiteral("PandaManager"),
                QStringLiteral("<b>PandaManager v1.0.0</b><br>"
                               "File manager moderno per CachyOS / KDE Plasma 6<br><br>"
                               "Creato da <b>d4ruerk1</b><br>"
                               "<a href='https://github.com/d4ruerk1/PandaManager'>GitHub</a>"));
        });
    }

    // ── Toolbar ────────────────────────────────────────────────────────────────
    void setupToolbar() {
        auto* tb = addToolBar(QStringLiteral("Navigazione"));
        tb->setMovable(false);
        tb->setIconSize(QSize(20, 20));
        tb->setToolButtonStyle(Qt::ToolButtonIconOnly);

        m_actBack = tb->addAction(style()->standardIcon(QStyle::SP_ArrowBack),
            QStringLiteral("Indietro"));
        m_actBack->setToolTip(QStringLiteral("Indietro (Alt+←)"));
        m_actBack->setEnabled(false);
        connect(m_actBack, &QAction::triggered, this, [this] {
            m_activePane->goBack();
        });

        m_actForward = tb->addAction(style()->standardIcon(QStyle::SP_ArrowForward),
            QStringLiteral("Avanti"));
        m_actForward->setToolTip(QStringLiteral("Avanti (Alt+→)"));
        m_actForward->setEnabled(false);
        connect(m_actForward, &QAction::triggered, this, [this] {
            m_activePane->goForward();
        });

        auto* actUp = tb->addAction(style()->standardIcon(QStyle::SP_ArrowUp),
            QStringLiteral("Su"));
        actUp->setToolTip(QStringLiteral("Cartella superiore (Alt+↑)"));
        connect(actUp, &QAction::triggered, this, [this] {
            m_activePane->goUp();
        });

        tb->addSeparator();

        // Search box (debounced, 300 ms)
        auto* searchBox = new QLineEdit(tb);
        searchBox->setPlaceholderText(QStringLiteral("🔍 Cerca…"));
        searchBox->setClearButtonEnabled(true);
        searchBox->setMaximumWidth(230);
        searchBox->setMinimumWidth(150);
        searchBox->setAccessibleName(QStringLiteral("Ricerca file"));
        searchBox->setToolTip(QStringLiteral("Filtra per nome (wildcards: *, ?)"));
        searchBox->setStyleSheet(QStringLiteral(
            "QLineEdit { border-radius: 12px; padding: 3px 10px; border: 1px solid palette(mid); }"
            "QLineEdit:focus { border: 1px solid palette(highlight); }"));
        tb->addWidget(searchBox);

        auto* searchTimer = new QTimer(this);
        searchTimer->setSingleShot(true);
        searchTimer->setInterval(300);
        connect(searchBox, &QLineEdit::textChanged, searchTimer, [searchTimer] { searchTimer->start(); });
        connect(searchTimer, &QTimer::timeout, this, [this, searchBox] {
            const QString p = searchBox->text().trimmed();
            m_leftPane->setFilter(p);
            m_rightPane->setFilter(p);
        });

        tb->addSeparator();

        // Split pane button
        auto* splitBtn = tb->addAction(style()->standardIcon(QStyle::SP_ToolBarVerticalExtensionButton),
            QStringLiteral("Doppio pannello (F3)"));
        splitBtn->setCheckable(true);
        connect(splitBtn, &QAction::triggered, this, &PandaMainWindow::toggleSplitPane);
        m_actSplit = splitBtn; // re-use the same action

        tb->addSeparator();

        // Ko-fi donate
        auto* donateBtn = new QPushButton(QStringLiteral("☕ Supporta"), tb);
        donateBtn->setFlat(true);
        donateBtn->setCursor(Qt::PointingHandCursor);
        donateBtn->setToolTip(QStringLiteral("Supporta PandaManager su Ko-fi — grazie! ♥"));
        donateBtn->setStyleSheet(QStringLiteral(
            "QPushButton { color: #e8853d; font-weight: 600; padding: 2px 8px; border-radius: 4px; }"
            "QPushButton:hover { background: rgba(232,133,61,0.12); text-decoration: underline; }"));
        connect(donateBtn, &QPushButton::clicked, this, [] {
            QDesktopServices::openUrl(QUrl(QStringLiteral("https://ko-fi.com/d4ruerk1")));
        });
        tb->addWidget(donateBtn);
    }

    // ── Address bar (second toolbar row) ──────────────────────────────────────
    void setupAddressBar() {
        auto* addrBar = new QToolBar(QStringLiteral("Barra indirizzo"), this);
        addrBar->setMovable(false);
        addrBar->setFloatable(false);
        addToolBar(Qt::TopToolBarArea, addrBar);
        // Force it below the main toolbar
        insertToolBarBreak(addrBar);

        m_addressBar = new AddressBar(addrBar);
        m_addressBar->setPath(QDir::homePath());
        m_addressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_addressBar->setFixedHeight(28);

        addrBar->addWidget(m_addressBar);

        connect(m_addressBar, &AddressBar::pathEntered, this, [this](const QString& p) {
            m_activePane->navigateTo(p);
        });
        connect(m_addressBar, &AddressBar::pathSegmentClicked, this, [this](const QString& p) {
            m_activePane->navigateTo(p);
        });
    }

    // ── Sidebar (like Windows Explorer nav pane) ──────────────────────────────
    void setupSidebar() {
        auto* dock = new QDockWidget(QStringLiteral("Navigazione"), this);
        dock->setObjectName(QStringLiteral("SidebarDock"));
        dock->setAllowedAreas(Qt::LeftDockWidgetArea);
        dock->setFeatures(QDockWidget::DockWidgetMovable);
        dock->setMinimumWidth(180);
        dock->setMaximumWidth(280);

        auto* sideWidget = new QWidget(dock);
        auto* sideLayout = new QVBoxLayout(sideWidget);
        sideLayout->setContentsMargins(0, 4, 0, 4);
        sideLayout->setSpacing(0);

        m_bookmarkList = new QListWidget(sideWidget);
        m_bookmarkList->setFrameShape(QFrame::NoFrame);
        m_bookmarkList->setSpacing(0);
        m_bookmarkList->setAccessibleName(QStringLiteral("Pannello di navigazione"));

        // Section helper
        auto addSection = [&](const QString& title) {
            auto* item = new QListWidgetItem(title, m_bookmarkList);
            item->setFlags(Qt::NoItemFlags);
            QFont f = item->font();
            f.setBold(true);
            f.setPointSizeF(f.pointSizeF() * 0.85);
            item->setFont(f);
            item->setForeground(QApplication::palette().color(QPalette::Mid));
        };

        auto addBookmark = [&](const QString& icon, const QString& label, const QString& path) {
            auto* item = new QListWidgetItem(icon + QLatin1Char(' ') + label, m_bookmarkList);
            item->setData(Qt::UserRole, path);
            item->setToolTip(path);
            item->setToolTip(label);
        };

        addSection(QStringLiteral("⭐  Preferiti"));
        addBookmark(QStringLiteral("🏠"), QStringLiteral("Home"),
            QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
        addBookmark(QStringLiteral("🖥"), QStringLiteral("Desktop"),
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        addBookmark(QStringLiteral("⬇"), QStringLiteral("Download"),
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
        addBookmark(QStringLiteral("📄"), QStringLiteral("Documenti"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));

        addSection(QStringLiteral("📁  Questo PC"));
        addBookmark(QStringLiteral("🖼"), QStringLiteral("Immagini"),
            QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
        addBookmark(QStringLiteral("🎵"), QStringLiteral("Musica"),
            QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
        addBookmark(QStringLiteral("🎬"), QStringLiteral("Video"),
            QStandardPaths::writableLocation(QStandardPaths::MoviesLocation));
        addBookmark(QStringLiteral("💾"), QStringLiteral("Filesystem Root"),
            QStringLiteral("/"));

        addSection(QStringLiteral("☁️  Cloud"));
        addBookmark(QStringLiteral("☁"), QStringLiteral("Cloud (rclone)"),
            QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QStringLiteral("/Cloud"));

        connect(m_bookmarkList, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) {
            const QString path = item->data(Qt::UserRole).toString();
            if (!path.isEmpty()) m_activePane->navigateTo(path);
        });

        sideLayout->addWidget(m_bookmarkList);
        dock->setWidget(sideWidget);
        addDockWidget(Qt::LeftDockWidgetArea, dock);
    }

    // ── Central area: two panes ────────────────────────────────────────────────
    void setupCentralArea() {
        m_splitter = new QSplitter(Qt::Horizontal, this);
        m_splitter->setHandleWidth(1);

        m_leftPane  = new PandaFilePane(m_splitter);
        m_rightPane = new PandaFilePane(m_splitter);
        m_rightPane->setVisible(false);

        m_activePane = m_leftPane;

        // Connect left pane
        connect(m_leftPane, &PandaFilePane::pathChanged, this, [this](const QString& p) {
            if (m_activePane == m_leftPane) {
                m_addressBar->setPath(p);
                setWindowTitle(QStringLiteral("PandaManager — %1").arg(p));
                updateNavButtons();
                updateStatusBar();
            }
        });
        connect(m_leftPane, &PandaFilePane::fileActivated, this, &PandaMainWindow::onFileActivated);
        connect(m_leftPane->view(), &QTreeView::pressed, this, [this] { m_activePane = m_leftPane; });

        // Connect right pane
        connect(m_rightPane, &PandaFilePane::pathChanged, this, [this](const QString& p) {
            if (m_activePane == m_rightPane) {
                m_addressBar->setPath(p);
                setWindowTitle(QStringLiteral("PandaManager — %1").arg(p));
                updateNavButtons();
                updateStatusBar();
            }
        });
        connect(m_rightPane, &PandaFilePane::fileActivated, this, &PandaMainWindow::onFileActivated);
        connect(m_rightPane->view(), &QTreeView::pressed, this, [this] { m_activePane = m_rightPane; });

        m_splitter->addWidget(m_leftPane);
        m_splitter->addWidget(m_rightPane);
        m_splitter->setSizes({500, 500});

        setCentralWidget(m_splitter);
    }

    // ── Status bar ─────────────────────────────────────────────────────────────
    void setupStatusBar() {
        m_statusLabel = new QLabel(QStringLiteral("Pronto"), this);
        m_pathLabel   = new QLabel(QDir::homePath(), this);
        m_pathLabel->setStyleSheet(QStringLiteral("color: palette(mid); font-size: 10px;"));
        m_progress = new QProgressBar(this);
        m_progress->setVisible(false);
        m_progress->setMaximumWidth(180);
        m_progress->setTextVisible(true);

        statusBar()->addWidget(m_statusLabel);
        statusBar()->addWidget(new QLabel(QStringLiteral("  |  "), this));
        statusBar()->addWidget(m_pathLabel, 1);
        statusBar()->addPermanentWidget(m_progress);
    }

    // ── Global shortcuts ───────────────────────────────────────────────────────
    void setupGlobalShortcuts() {
        // F3 — toggle split pane
        auto* f3 = new QShortcut(QKeySequence(Qt::Key_F3), this);
        connect(f3, &QShortcut::activated, this, &PandaMainWindow::toggleSplitPane);

        // Ctrl+H — toggle hidden files
        auto* ctrlH = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_H), this);
        connect(ctrlH, &QShortcut::activated, this, [this] {
            m_leftPane->toggleHidden();
            m_rightPane->toggleHidden();
        });

        // Alt+Left — back
        auto* altLeft = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Left), this);
        connect(altLeft, &QShortcut::activated, this, [this] { m_activePane->goBack(); });

        // Alt+Right — forward
        auto* altRight = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Right), this);
        connect(altRight, &QShortcut::activated, this, [this] { m_activePane->goForward(); });

        // Alt+Up — up
        auto* altUp = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Up), this);
        connect(altUp, &QShortcut::activated, this, [this] { m_activePane->goUp(); });

        // Space — QuickLook
        m_spaceShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
        connect(m_spaceShortcut, &QShortcut::activated, this, &PandaMainWindow::triggerQuickLook);
    }

    // ── Members ────────────────────────────────────────────────────────────────
    AddressBar*         m_addressBar    = nullptr;
    QSplitter*          m_splitter      = nullptr;
    PandaFilePane*      m_leftPane      = nullptr;
    PandaFilePane*      m_rightPane     = nullptr;
    PandaFilePane*      m_activePane    = nullptr;
    bool                m_splitMode     = false;

    QListWidget*        m_bookmarkList  = nullptr;
    QLabel*             m_statusLabel   = nullptr;
    QLabel*             m_pathLabel     = nullptr;
    QProgressBar*       m_progress      = nullptr;

    QAction*            m_actBack       = nullptr;
    QAction*            m_actForward    = nullptr;
    QAction*            m_actSplit      = nullptr;

    QuickLookDialog*    m_quickLook     = nullptr;
    QShortcut*          m_spaceShortcut = nullptr;
};

#include "main.moc"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("CachyOS"));
    app.setApplicationName(QStringLiteral("PandaManager"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));

    PandaManager::AuditLog::instance().open();

    PandaMainWindow win;
    win.show();
    return app.exec();
}
