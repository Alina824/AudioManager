#include "mainwindow.h"
#include <QStandardItemModel>
#include <QStandardItem>
#include <QHeaderView>
#include <QImageReader>
#include <QPixmap>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QStandardPaths>
#include <QApplication>
#include <QRandomGenerator>
#include <QKeySequence>
#include <QMenu>
#include <QProcess>
#include <QProgressDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_currentPlaylistId(-1)
    , m_currentTrackIndex(-1)
    , m_shuffleEnabled(false)
    , m_repeatEnabled(false)
    , m_seeking(false)
{
    m_dbManager = new DatabaseManager(this);
    m_dbManager->initializeDatabase();
    
    m_audioPlayer = new AudioPlayer(m_dbManager, this);
    m_playlistModel = new PlaylistModel(this);
    m_historyModel = new PlaylistModel(this);
    
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupConnections();
    loadPlaylists();
    loadTracks();
    setWindowTitle("Аудио Плеер");
    resize(1200, 800);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_leftPanel = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(m_leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    m_leftTabs = new QTabWidget(this);
    QWidget *playlistTab = new QWidget(this);
    QVBoxLayout *playlistTabLayout = new QVBoxLayout(playlistTab);
    QHBoxLayout *playlistControlsLayout = new QHBoxLayout();
    m_playlistCombo = new QComboBox(this);
    m_createPlaylistBtn = new QPushButton("+", this);
    m_createPlaylistBtn->setMaximumWidth(30);
    m_deletePlaylistBtn = new QPushButton("-", this);
    m_deletePlaylistBtn->setMaximumWidth(30);
    playlistControlsLayout->addWidget(m_playlistCombo);
    playlistControlsLayout->addWidget(m_createPlaylistBtn);
    playlistControlsLayout->addWidget(m_deletePlaylistBtn);
    
    m_playlistTree = new QTreeView(this);
    m_playlistTree->setHeaderHidden(true);
    m_playlistTree->setRootIsDecorated(false);
    playlistTabLayout->addLayout(playlistControlsLayout);
    playlistTabLayout->addWidget(m_playlistTree);
    m_leftTabs->addTab(playlistTab, "Плейлисты");
    
    QWidget *historyTab = new QWidget(this);
    QVBoxLayout *historyTabLayout = new QVBoxLayout(historyTab);
    m_historyList = new QListView(this);
    m_historyList->setModel(m_historyModel);
    QPushButton *clearHistoryBtn = new QPushButton("Очистить историю", this);
    historyTabLayout->addWidget(m_historyList);
    historyTabLayout->addWidget(clearHistoryBtn);
    connect(clearHistoryBtn, &QPushButton::clicked, this, [this]() {
        m_dbManager->clearHistory();
        loadHistory();
    });
    m_leftTabs->addTab(historyTab, "История");
    
    leftLayout->addWidget(m_leftTabs);
    m_leftPanel->setMaximumWidth(250);
    m_centerPanel = new QWidget(this);
    QVBoxLayout *centerLayout = new QVBoxLayout(m_centerPanel);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *searchLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Поиск треков...");
    searchLayout->addWidget(new QLabel("Поиск:", this));
    searchLayout->addWidget(m_searchEdit);
    
    QHBoxLayout *filterLayout = new QHBoxLayout();
    m_artistFilter = new QComboBox(this);
    m_artistFilter->setEditable(true);
    m_artistFilter->setInsertPolicy(QComboBox::NoInsert);
    m_artistFilter->addItem("Все исполнители");
    m_albumFilter = new QComboBox(this);
    m_albumFilter->setEditable(true);
    m_albumFilter->setInsertPolicy(QComboBox::NoInsert);
    m_albumFilter->addItem("Все альбомы");
    m_clearFiltersBtn = new QPushButton("Очистить", this);
    m_addToPlaylistBtn = new QPushButton("Добавить в плейлист", this);
    
    filterLayout->addWidget(new QLabel("Исполнитель:", this));
    filterLayout->addWidget(m_artistFilter);
    filterLayout->addWidget(new QLabel("Альбом:", this));
    filterLayout->addWidget(m_albumFilter);
    filterLayout->addWidget(m_clearFiltersBtn);
    filterLayout->addWidget(m_addToPlaylistBtn);
    filterLayout->addStretch();
    m_trackList = new QListView(this);
    m_trackList->setModel(m_playlistModel);
    
    centerLayout->addLayout(searchLayout);
    centerLayout->addLayout(filterLayout);
    centerLayout->addWidget(m_trackList);
    m_rightPanel = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(m_rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    m_albumCoverLabel = new QLabel(this);
    m_albumCoverLabel->setAlignment(Qt::AlignCenter);
    m_albumCoverLabel->setMinimumSize(200, 200);
    m_albumCoverLabel->setMaximumSize(300, 300);
    m_albumCoverLabel->setStyleSheet("border: 1px solid gray; background-color: #2b2b2b;");
    m_albumCoverLabel->setText("Нет обложки");
    m_albumCoverLabel->setScaledContents(false);
    m_trackInfoLabel = new QLabel("Трек не выбран", this);
    m_trackInfoLabel->setAlignment(Qt::AlignCenter);
    m_trackInfoLabel->setWordWrap(true);
    rightLayout->addWidget(m_albumCoverLabel);
    rightLayout->addWidget(m_trackInfoLabel);
    rightLayout->addStretch();
    m_rightPanel->setMaximumWidth(350);
    
    m_mainSplitter->addWidget(m_leftPanel);
    m_mainSplitter->addWidget(m_centerPanel);
    m_mainSplitter->addWidget(m_rightPanel);
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 2);
    m_mainSplitter->setStretchFactor(2, 0);
    
    m_controlsPanel = new QWidget(this);
    QVBoxLayout *controlsLayout = new QVBoxLayout(m_controlsPanel);
    controlsLayout->setContentsMargins(5, 5, 5, 5);
    QHBoxLayout *positionLayout = new QHBoxLayout();
    m_positionSlider = new QSlider(Qt::Horizontal, this);
    m_timeLabel = new QLabel("00:00 / 00:00", this);
    m_timeLabel->setMinimumWidth(100);
    positionLayout->addWidget(m_positionSlider);
    positionLayout->addWidget(m_timeLabel);
    
    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    m_previousBtn = new QPushButton("Предыдущий", this);
    m_playPauseBtn = new QPushButton("Воспроизведение", this);
    m_stopBtn = new QPushButton("С начала", this);
    m_nextBtn = new QPushButton("Следующий", this);
    m_shuffleBtn = new QPushButton("Рандомный выбор следующего", this);
    m_repeatBtn = new QPushButton("Повтор", this);
    m_volumeLabel = new QLabel("Громкость", this);
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(50);
    
    buttonsLayout->addWidget(m_previousBtn);
    buttonsLayout->addWidget(m_playPauseBtn);
    buttonsLayout->addWidget(m_stopBtn);
    buttonsLayout->addWidget(m_nextBtn);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(m_shuffleBtn);
    buttonsLayout->addWidget(m_repeatBtn);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(m_volumeLabel);
    buttonsLayout->addWidget(m_volumeSlider);
    m_volumeSlider->setMaximumWidth(100);
    
    controlsLayout->addLayout(positionLayout);
    controlsLayout->addLayout(buttonsLayout);
    m_controlsPanel->setMaximumHeight(120);
    mainLayout->addWidget(m_mainSplitter);
    mainLayout->addWidget(m_controlsPanel);
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu("Файл");
    m_addFilesAction = fileMenu->addAction("Добавить файлы...");
    m_addFilesAction->setShortcut(QKeySequence::Open);
    m_addFolderAction = fileMenu->addAction("Добавить папку...");
    fileMenu->addSeparator();
    m_exitAction = fileMenu->addAction("Выход");
    m_exitAction->setShortcut(QKeySequence::Quit);
    
    QMenu *viewMenu = menuBar()->addMenu("Вид");
    m_showHistoryAction = viewMenu->addAction("Показать историю");
    m_showHistoryAction->setCheckable(true);
    m_showHistoryAction->setChecked(true);
    
}

void MainWindow::setupToolBar()
{
    QToolBar *toolBar = addToolBar("Главная");
    toolBar->addAction(m_addFilesAction);
    toolBar->addAction(m_addFolderAction);
    toolBar->addSeparator();
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage("Готов");
}

void MainWindow::setupConnections()
{
    connect(m_addFilesAction, &QAction::triggered, this, &MainWindow::onAddFiles);
    connect(m_addFolderAction, &QAction::triggered, this, &MainWindow::onAddFolder);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    connect(m_showHistoryAction, &QAction::toggled, this, &MainWindow::onShowHistory);
    
    connect(m_createPlaylistBtn, &QPushButton::clicked, this, &MainWindow::onCreatePlaylist);
    connect(m_deletePlaylistBtn, &QPushButton::clicked, this, &MainWindow::onDeletePlaylist);
    connect(m_playlistCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::onPlaylistSelected);
    connect(m_addToPlaylistBtn, &QPushButton::clicked, this, &MainWindow::onAddToPlaylist);
    connect(m_playPauseBtn, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStop);
    connect(m_previousBtn, &QPushButton::clicked, this, &MainWindow::onPrevious);
    connect(m_nextBtn, &QPushButton::clicked, this, &MainWindow::onNext);
    connect(m_shuffleBtn, &QPushButton::clicked, this, &MainWindow::onShuffle);
    connect(m_repeatBtn, &QPushButton::clicked, this, &MainWindow::onRepeat);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    connect(m_positionSlider, &QSlider::sliderPressed, this, &MainWindow::onSliderPressed);
    connect(m_positionSlider, &QSlider::sliderReleased, this, &MainWindow::onSliderReleased);
    connect(m_positionSlider, &QSlider::valueChanged, this, &MainWindow::onSeekPositionChanged);
    
    connect(m_audioPlayer, &AudioPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_audioPlayer, &AudioPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_audioPlayer, &AudioPlayer::stateChanged, this, &MainWindow::onStateChanged);
    connect(m_audioPlayer, &AudioPlayer::trackChanged, this, &MainWindow::onTrackChanged);
    connect(m_audioPlayer, &AudioPlayer::errorOccurred, this, [this](const QString &error) {
        QMessageBox::warning(this, "Ошибка воспроизведения", error);
    });
    
    connect(m_trackList, &QListView::doubleClicked, this, &MainWindow::onTrackDoubleClicked);
    connect(m_trackList->selectionModel(), &QItemSelectionModel::currentChanged, 
            this, [this](const QModelIndex &current, const QModelIndex &previous) {
        Q_UNUSED(previous)
        if (current.isValid()) {
            TrackInfo track = m_playlistModel->trackAt(current.row());
            TrackInfo playingTrack = m_audioPlayer->currentTrack();
            if (playingTrack.id < 0) {
                updateAlbumCoverForTrack(track);
            }
        }
    });
    m_trackList->setContextMenuPolicy(Qt::CustomContextMenu);
    
    connect(m_historyList, &QListView::doubleClicked, this, [this](const QModelIndex &index) {
        TrackInfo track = m_historyModel->trackAt(index.row());
        if (track.id >= 0) {
            bool found = false;
            for (int i = 0; i < m_playlistModel->trackCount(); ++i) {
                if (m_playlistModel->trackAt(i).id == track.id) {
                    m_currentTrackIndex = i;
                    m_trackList->setCurrentIndex(m_playlistModel->index(i));
                    found = true;
                    break;
                }
            }
            if (!found) {
                m_playlistModel->addTrack(track);
                m_currentTrackIndex = m_playlistModel->trackCount() - 1;
            }
            m_audioPlayer->setTrack(track);
            m_audioPlayer->play();
        }
    });
    connect(m_trackList, &QListView::customContextMenuRequested, this, [this](const QPoint &pos) {
        QModelIndex index = m_trackList->indexAt(pos);
        if (!index.isValid()) return;
        
        QMenu menu(this);
        QAction *playAction = menu.addAction("Воспроизвести");
        QAction *addToPlaylistAction = menu.addAction("Добавить в плейлист...");
        QAction *removeFromPlaylistAction = menu.addAction("Удалить из плейлиста");
        QAction *addArtistAction = menu.addAction("Добавить исполнителя...");
        QAction *removeArtistAction = menu.addAction("Удалить исполнителя...");
        menu.addSeparator();
        QAction *addAlbumAction = menu.addAction("Добавить альбом...");
        QAction *removeAlbumAction = menu.addAction("Удалить альбом...");
        QAction *loadCoverAction = menu.addAction("Загрузить обложку...");
        menu.addSeparator();
        QAction *deleteTrackAction = menu.addAction("Удалить композицию");
        
        TrackInfo track = m_playlistModel->trackAt(index.row());
        TrackInfo fullTrack = m_dbManager->getTrack(track.id);
        if (fullTrack.album.isEmpty()) {
            removeAlbumAction->setEnabled(false);
        }
        if (fullTrack.artist.isEmpty()) {
            removeArtistAction->setEnabled(false);
        }
        
        if (m_currentPlaylistId < 0) {
            removeFromPlaylistAction->setEnabled(false);
        }
        
        QAction *selected = menu.exec(m_trackList->mapToGlobal(pos));
        if (selected == playAction) {
            m_trackList->setCurrentIndex(index);
            TrackInfo playTrack = m_playlistModel->trackAt(index.row());
            if (playTrack.id >= 0) {
                m_currentTrackIndex = index.row();
                m_audioPlayer->setTrack(playTrack);
                m_audioPlayer->play();
            }
        } else if (selected == addToPlaylistAction) {
            m_trackList->setCurrentIndex(index);
            onAddToPlaylist();
        } else if (selected == removeFromPlaylistAction) {
            m_trackList->setCurrentIndex(index);
            onRemoveFromPlaylist();
        } else if (selected == addArtistAction) {
            m_trackList->setCurrentIndex(index);
            onAddArtist();
        } else if (selected == removeArtistAction) {
            m_trackList->setCurrentIndex(index);
            onRemoveArtist();
        } else if (selected == addAlbumAction) {
            m_trackList->setCurrentIndex(index);
            onAddAlbum();
        } else if (selected == removeAlbumAction) {
            m_trackList->setCurrentIndex(index);
            onRemoveAlbum();
        } else if (selected == loadCoverAction) {
            m_trackList->setCurrentIndex(index);
            onLoadCoverImage();
        } else if (selected == deleteTrackAction) {
            m_trackList->setCurrentIndex(index);
            onDeleteTrack();
        }
    });
    
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_artistFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::onFilterChanged);
    connect(m_albumFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::onFilterChanged);
    connect(m_clearFiltersBtn, &QPushButton::clicked, this, [this]() {
        m_searchEdit->clear();
        m_artistFilter->setCurrentIndex(0);
        m_albumFilter->setCurrentIndex(0);
        applyFilters();
    });
}

void MainWindow::onAddFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this, "Добавить аудио файлы", "",
        "Все поддерживаемые файлы (*.mp3 *.wav *.flac *.ogg *.m4a *.aac *.wma *.mp4 *.m4v);;Аудио файлы (*.mp3 *.wav *.flac *.ogg *.m4a *.aac *.wma);;Видео файлы (*.mp4 *.m4v);;Все файлы (*.*)"
    );
    
    if (files.isEmpty()) {
        return;
    }
    
    int added = 0;
    int converted = 0;
    
    QProgressDialog progress("Обработка файлов...", "Отмена", 0, files.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    
    for (int i = 0; i < files.size(); ++i) {
        const QString &file = files[i];
        progress.setValue(i);
        progress.setLabelText(QString("Обработка: %1").arg(QFileInfo(file).fileName()));
        
        QApplication::processEvents();
        if (progress.wasCanceled()) {
            break;
        }
        
        QString fileToAdd = file;
        
        if (file.endsWith(".mp4", Qt::CaseInsensitive) || file.endsWith(".m4v", Qt::CaseInsensitive)) {
            QString mp3Path = convertMp4ToMp3(file);
            if (!mp3Path.isEmpty()) {
                fileToAdd = mp3Path;
                converted++;
            } else {
                statusBar()->showMessage(QString("Ошибка конвертации: %1").arg(file), 5000);
                continue;
            }
        }
        
        int trackId = m_dbManager->addTrack(fileToAdd);
        if (trackId >= 0) {
            added++;
        }
    }
    
    progress.setValue(files.size());
    
    QString message = QString("Добавлено файлов: %1").arg(added);
    if (converted > 0) {
        message += QString(", конвертировано: %1").arg(converted);
    }
    statusBar()->showMessage(message, 3000);
    loadTracks();
}

void MainWindow::onAddFolder()
{
    QString folder = QFileDialog::getExistingDirectory(this, "Добавить папку");
    
    if (folder.isEmpty()) {
        return;
    }
    
    QDir dir(folder);
    QStringList filters = {"*.mp3", "*.wav", "*.flac", "*.ogg", "*.m4a", "*.aac", "*.wma", "*.mp4", "*.m4v"};
    QStringList files = dir.entryList(filters, QDir::Files);
    
    int added = 0;
    int converted = 0;
    
    QProgressDialog progress("Обработка файлов...", "Отмена", 0, files.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    
    for (int i = 0; i < files.size(); ++i) {
        const QString &file = files[i];
        progress.setValue(i);
        progress.setLabelText(QString("Обработка: %1").arg(file));
        
        QApplication::processEvents();
        if (progress.wasCanceled()) {
            break;
        }
        
        QString filePath = dir.absoluteFilePath(file);
        QString fileToAdd = filePath;
        
        if (file.endsWith(".mp4", Qt::CaseInsensitive) || file.endsWith(".m4v", Qt::CaseInsensitive)) {
            QString mp3Path = convertMp4ToMp3(filePath);
            if (!mp3Path.isEmpty()) {
                fileToAdd = mp3Path;
                converted++;
            } else {
                continue;
            }
        }
        
        int trackId = m_dbManager->addTrack(fileToAdd);
        if (trackId >= 0) {
            added++;
        }
    }
    
    progress.setValue(files.size());
    
    QString message = QString("Добавлено файлов из папки: %1").arg(added);
    if (converted > 0) {
        message += QString(", конвертировано: %1").arg(converted);
    }
    statusBar()->showMessage(message, 3000);
    loadTracks();
}

void MainWindow::onPlayPause()
{
    if (m_audioPlayer->state() == QMediaPlayer::PlayingState) {
        m_audioPlayer->pause();
    } else {
        if (m_currentTrackIndex < 0 || m_audioPlayer->currentTrack().id < 0) {
            QModelIndex currentIndex = m_trackList->currentIndex();
            if (currentIndex.isValid()) {
                onTrackDoubleClicked(currentIndex);
                return;
            } else if (m_playlistModel->trackCount() > 0) {
                m_currentTrackIndex = 0;
                TrackInfo track = m_playlistModel->trackAt(0);
                if (track.id >= 0) {
                    m_audioPlayer->setTrack(track);
                    m_trackList->setCurrentIndex(m_playlistModel->index(0));
                }
            } else {
                statusBar()->showMessage("Нет треков для воспроизведения", 2000);
                return;
            }
        }
        m_audioPlayer->play();
    }
}

void MainWindow::onStop()
{
    if (m_currentTrackIndex >= 0 && m_currentTrackIndex < m_playlistModel->trackCount()) {
        TrackInfo track = m_playlistModel->trackAt(m_currentTrackIndex);
        if (track.id >= 0) {
            m_audioPlayer->setTrack(track);
            m_audioPlayer->play();
        }
    } else {
        m_audioPlayer->stop();
        m_currentTrackIndex = -1;
    }
}

void MainWindow::onPrevious()
{
    if (m_playlistModel->trackCount() == 0) {
        return;
    }
    
    if (m_shuffleEnabled) {
        m_currentTrackIndex = QRandomGenerator::global()->bounded(m_playlistModel->trackCount());
    } else {
        m_currentTrackIndex--;
        if (m_currentTrackIndex < 0) {
            m_currentTrackIndex = m_playlistModel->trackCount() - 1;
        }
    }
    
    TrackInfo track = m_playlistModel->trackAt(m_currentTrackIndex);
    if (track.id >= 0) {
        m_audioPlayer->setTrack(track);
        m_audioPlayer->play();
    }
}

void MainWindow::onNext()
{
    if (m_playlistModel->trackCount() == 0) {
        return;
    }
    
    if (m_shuffleEnabled) {
        m_currentTrackIndex = QRandomGenerator::global()->bounded(m_playlistModel->trackCount());
    } else {
        m_currentTrackIndex++;
        if (m_currentTrackIndex >= m_playlistModel->trackCount()) {
            m_currentTrackIndex = m_repeatEnabled ? 0 : -1;
        }
    }
    
    if (m_currentTrackIndex >= 0) {
        TrackInfo track = m_playlistModel->trackAt(m_currentTrackIndex);
        if (track.id >= 0) {
            m_audioPlayer->setTrack(track);
            m_audioPlayer->play();
        }
    } else {
        m_audioPlayer->stop();
    }
}

void MainWindow::onShuffle()
{
    m_shuffleEnabled = !m_shuffleEnabled;
    m_shuffleBtn->setStyleSheet(m_shuffleEnabled ? "font-weight: bold;" : "");
}

void MainWindow::onRepeat()
{
    m_repeatEnabled = !m_repeatEnabled;
    m_repeatBtn->setStyleSheet(m_repeatEnabled ? "font-weight: bold;" : "");
}

void MainWindow::onVolumeChanged(int value)
{
    float volume = value / 100.0f;
    m_audioPlayer->setVolume(volume);
    m_volumeLabel->setText(value == 0 ? "Громкость: Выкл" : QString("Громкость: %1%").arg(value));
}

void MainWindow::onPositionChanged(qint64 position)
{
    if (!m_seeking) {
        m_positionSlider->setValue(position);
        updateTimeLabels();
        
        qint64 duration = m_audioPlayer->duration();
        if (duration > 0 && position >= duration - 100 && m_repeatEnabled) {
            if (m_currentTrackIndex >= 0 && m_currentTrackIndex < m_playlistModel->trackCount()) {
                TrackInfo track = m_playlistModel->trackAt(m_currentTrackIndex);
                if (track.id >= 0 && track.id == m_audioPlayer->currentTrack().id) {
                    m_audioPlayer->setPosition(0);
                    m_audioPlayer->play();
                }
            }
        }
    }
}

void MainWindow::onDurationChanged(qint64 duration)
{
    m_positionSlider->setMaximum(duration);
    updateTimeLabels();
}

void MainWindow::onStateChanged(QMediaPlayer::PlaybackState state)
{
    updatePlayButton();
    
    if (state == QMediaPlayer::PlayingState) {
        TrackInfo track = m_audioPlayer->currentTrack();
        if (track.id >= 0) {
            m_dbManager->addToHistory(track.id);
            if (m_showHistoryAction->isChecked()) {
                loadHistory();
            }
        }
    }
    
    if (state == QMediaPlayer::StoppedState) {
        if (m_repeatEnabled) {
            if (m_currentTrackIndex >= 0 && m_currentTrackIndex < m_playlistModel->trackCount()) {
                TrackInfo track = m_playlistModel->trackAt(m_currentTrackIndex);
                if (track.id >= 0) {
                    m_audioPlayer->setTrack(track);
                    m_audioPlayer->play();
                }
            }
        } else {
            if (m_currentTrackIndex >= 0 && 
                m_currentTrackIndex < m_playlistModel->trackCount() - 1) {
                onNext();
            }
        }
    }
}

void MainWindow::onTrackChanged(const TrackInfo &track)
{
    updateAlbumCover();
    
    QString info = QString("<b>%1</b><br>%2<br>%3")
                   .arg(track.title.isEmpty() ? QFileInfo(track.filePath).baseName() : track.title)
                   .arg(track.artist.isEmpty() ? "Неизвестный исполнитель" : track.artist)
                   .arg(track.album.isEmpty() ? "Неизвестный альбом" : track.album);
    m_trackInfoLabel->setText(info);
    
    for (int i = 0; i < m_playlistModel->trackCount(); ++i) {
        if (m_playlistModel->trackAt(i).id == track.id) {
            m_currentTrackIndex = i;
            m_trackList->setCurrentIndex(m_playlistModel->index(i));
            break;
        }
    }
}

void MainWindow::onTrackDoubleClicked(const QModelIndex &index)
{
    TrackInfo track = m_playlistModel->trackAt(index.row());
    if (track.id >= 0) {
        m_currentTrackIndex = index.row();
        m_audioPlayer->setTrack(track);
        m_audioPlayer->play();
    }
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    applyFilters();
}

void MainWindow::onFilterChanged()
{
    applyFilters();
}


void MainWindow::onCreatePlaylist()
{
    bool ok;
    QString name = QInputDialog::getText(this, "Создать плейлист", "Название плейлиста:",
                                         QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        int playlistId = m_dbManager->createPlaylist(name);
        if (playlistId >= 0) {
            loadPlaylists();
            m_playlistCombo->setCurrentIndex(m_playlistCombo->count() - 1);
        }
    }
}

void MainWindow::onDeletePlaylist()
{
    if (m_currentPlaylistId < 0) {
        return;
    }
    
    int ret = QMessageBox::question(this, "Удалить плейлист", 
                                     "Вы уверены, что хотите удалить этот плейлист?",
                                     QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        m_dbManager->deletePlaylist(m_currentPlaylistId);
        loadPlaylists();
        m_currentPlaylistId = -1;
        m_playlistModel->clear();
    }
}

void MainWindow::onPlaylistSelected(int index)
{
    if (index <= 0) {
        m_currentPlaylistId = -1;
        loadTracks();
        return;
    }
    
    QList<PlaylistInfo> playlists = m_dbManager->getAllPlaylists();
    if (index - 1 < playlists.size()) {
        m_currentPlaylistId = playlists[index - 1].id;
        QList<TrackInfo> tracks = m_dbManager->getPlaylistTracks(m_currentPlaylistId);
        m_playlistModel->setTracks(tracks);
    }
}

void MainWindow::onShowHistory()
{
    if (m_showHistoryAction->isChecked()) {
        loadHistory();
    }
}

void MainWindow::onAddToPlaylist()
{
    QModelIndex index = m_trackList->currentIndex();
    if (!index.isValid()) {
        return;
    }
    
    TrackInfo track = m_playlistModel->trackAt(index.row());
    if (track.id < 0) {
        return;
    }
    
    QList<PlaylistInfo> playlists = m_dbManager->getAllPlaylists();
    if (playlists.isEmpty()) {
        QMessageBox::information(this, "Нет плейлистов", "Сначала создайте плейлист.");
        return;
    }
    
    QStringList playlistNames;
    for (const PlaylistInfo &playlist : playlists) {
        playlistNames << playlist.name;
    }
    
    bool ok;
    QString playlistName = QInputDialog::getItem(this, "Добавить в плейлист", 
                                                 "Выберите плейлист:", playlistNames, 0, false, &ok);
    if (!ok || playlistName.isEmpty()) {
        return;
    }
    
    int playlistId = -1;
    for (const PlaylistInfo &playlist : playlists) {
        if (playlist.name == playlistName) {
            playlistId = playlist.id;
            break;
        }
    }
    
    if (playlistId >= 0) {
        if (m_dbManager->addTrackToPlaylist(playlistId, track.id)) {
            statusBar()->showMessage(QString("Добавлено '%1' в плейлист '%2'")
                                    .arg(track.title.isEmpty() ? QFileInfo(track.filePath).baseName() : track.title)
                                    .arg(playlistName), 3000);
        }
    }
}

void MainWindow::onRemoveFromPlaylist()
{
    if (m_currentPlaylistId < 0) {
        return;
    }
    
    QModelIndex index = m_trackList->currentIndex();
    if (!index.isValid()) {
        return;
    }
    
    TrackInfo track = m_playlistModel->trackAt(index.row());
    if (track.id < 0) {
        return;
    }
    
    if (m_dbManager->removeTrackFromPlaylist(m_currentPlaylistId, track.id)) {
        m_playlistModel->removeTrack(index.row());
        statusBar()->showMessage("Трек удалён из плейлиста", 2000);
    }
}

void MainWindow::onLoadCoverImage()
{
    QModelIndex index = m_trackList->currentIndex();
    if (!index.isValid()) {
        return;
    }
    
    TrackInfo track = m_playlistModel->trackAt(index.row());
    if (track.id < 0) {
        return;
    }
    
    QString fileName = QFileDialog::getOpenFileName(this, "Выберите изображение обложки",
                                                   "", "Изображения (*.jpg *.jpeg *.png *.bmp)");
    if (fileName.isEmpty()) {
        return;
    }
    
    QString coversDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/covers";
    QDir().mkpath(coversDir);
    
    QString coverFileName = QString::number(track.id) + ".jpg";
    QString coverPath = coversDir + "/" + coverFileName;
    
    QPixmap pixmap(fileName);
    if (pixmap.isNull()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить изображение.");
        return;
    }
    
    if (!pixmap.save(coverPath, "JPG")) {
        QMessageBox::warning(this, "Ошибка", "Не удалось сохранить обложку.");
        return;
    }
    
    TrackInfo fullTrack = m_dbManager->getTrack(track.id);
    m_dbManager->updateTrackInfo(fullTrack.id, fullTrack.title, fullTrack.artist, 
                                 fullTrack.album, fullTrack.duration, coverPath);
    
    TrackInfo updatedTrack = m_dbManager->getTrack(track.id);
    
    TrackInfo currentTrack = m_audioPlayer->currentTrack();
    if (currentTrack.id == track.id) {
        m_audioPlayer->setTrack(updatedTrack);
    }
    
    loadTracks();
    
    updateAlbumCoverForTrack(updatedTrack);
    
    statusBar()->showMessage("Обложка загружена", 2000);
}

void MainWindow::onDeleteTrack()
{
    QModelIndex index = m_trackList->currentIndex();
    if (!index.isValid()) {
        return;
    }
    
    TrackInfo track = m_playlistModel->trackAt(index.row());
    if (track.id < 0) {
        return;
    }
    
    int ret = QMessageBox::question(this, "Удаление композиции",
                                    QString("Вы уверены, что хотите удалить композицию '%1'?\n\n"
                                           "Это действие нельзя отменить.").arg(track.title.isEmpty() ? 
                                           QFileInfo(track.filePath).baseName() : track.title),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        if (m_dbManager->deleteTrack(track.id)) {
            loadTracks();
            statusBar()->showMessage("Композиция удалена", 2000);
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось удалить композицию.");
        }
    }
}

void MainWindow::onAddArtist()
{
    QModelIndex index = m_trackList->currentIndex();
    if (!index.isValid()) {
        return;
    }
    
    TrackInfo track = m_playlistModel->trackAt(index.row());
    if (track.id < 0) {
        return;
    }
    
    QStringList allArtists;
    
    for (int i = 1; i < m_artistFilter->count(); ++i) {
        QString artist = m_artistFilter->itemText(i);
        if (!artist.isEmpty()) {
            allArtists << artist;
        }
    }
    
    if (allArtists.isEmpty()) {
        allArtists = m_dbManager->getAllArtists();
        
        QList<TrackInfo> tracks;
        if (m_currentPlaylistId >= 0) {
            tracks = m_dbManager->getPlaylistTracks(m_currentPlaylistId);
        } else {
            tracks = m_dbManager->getAllTracks();
        }
        
        for (const TrackInfo &t : tracks) {
            if (!t.artist.isEmpty()) {
                QStringList trackArtists = t.artist.split(", ");
                for (const QString &artist : trackArtists) {
                    QString trimmedArtist = artist.trimmed();
                    if (!trimmedArtist.isEmpty() && !allArtists.contains(trimmedArtist)) {
                        allArtists << trimmedArtist;
                    }
                }
            }
        }
    }
    
    allArtists.sort();
    
    bool ok;
    QString artistName;
    
    if (allArtists.isEmpty()) {
        artistName = QInputDialog::getText(this, "Добавить исполнителя", 
                                          "Введите имя исполнителя:",
                                          QLineEdit::Normal, "", &ok);
    } else {
        allArtists << "Другое...";
        
        artistName = QInputDialog::getItem(this, "Добавить исполнителя", 
                                          "Выберите исполнителя:",
                                          allArtists, 0, false, &ok);
        
        if (ok && artistName == "Другое...") {
            artistName = QInputDialog::getText(this, "Добавить исполнителя", 
                                              "Введите имя исполнителя:",
                                              QLineEdit::Normal, "", &ok);
        }
    }
    
    if (ok && !artistName.isEmpty() && artistName != "Другое...") {
        int artistId = m_dbManager->addArtist(artistName);
        if (artistId >= 0) {
            if (m_dbManager->addArtistToTrack(track.id, artistId)) {
                loadTracks();
                statusBar()->showMessage(QString("Исполнитель '%1' добавлен").arg(artistName), 2000);
            }
        }
    }
}

void MainWindow::onRemoveArtist()
{
    QModelIndex index = m_trackList->currentIndex();
    if (!index.isValid()) {
        return;
    }
    
    TrackInfo track = m_playlistModel->trackAt(index.row());
    if (track.id < 0 || track.artist.isEmpty()) {
        QMessageBox::information(this, "Нет исполнителей", "У этого трека нет исполнителей.");
        return;
    }
    
    TrackInfo fullTrack = m_dbManager->getTrack(track.id);
    QStringList artists = fullTrack.artist.split(", ");
    
    if (artists.isEmpty()) {
        QMessageBox::information(this, "Нет исполнителей", "У этого трека нет исполнителей.");
        return;
    }
    
    bool ok;
    QString artistName = QInputDialog::getItem(this, "Удалить исполнителя", "Выберите исполнителя для удаления:",
                                              artists, 0, false, &ok);
    if (ok && !artistName.isEmpty()) {
        int artistId = m_dbManager->getArtistId(artistName);
        if (artistId >= 0) {
            if (m_dbManager->removeArtistFromTrack(track.id, artistId)) {
                loadTracks();
                statusBar()->showMessage(QString("Исполнитель '%1' удалён").arg(artistName), 2000);
            }
        }
    }
}

void MainWindow::onAddAlbum()
{
    QModelIndex index = m_trackList->currentIndex();
    if (!index.isValid()) {
        return;
    }
    
    TrackInfo track = m_playlistModel->trackAt(index.row());
    if (track.id < 0) {
        return;
    }
    
    TrackInfo fullTrack = m_dbManager->getTrack(track.id);
    QStringList suggestedAlbums;
    QStringList otherAlbums;
    
    if (!fullTrack.artist.isEmpty()) {
        QStringList artists = fullTrack.artist.split(", ");
        for (const QString &artistName : artists) {
            int artistId = m_dbManager->getArtistId(artistName);
            if (artistId >= 0) {
                QStringList artistAlbums = m_dbManager->getAlbumsByArtist(artistId);
                for (const QString &album : artistAlbums) {
                    if (!suggestedAlbums.contains(album)) {
                        suggestedAlbums << album;
                    }
                }
            }
        }
    }
    
    QStringList allAlbums = m_dbManager->getAllAlbums();
    for (const QString &album : allAlbums) {
        if (!suggestedAlbums.contains(album)) {
            otherAlbums << album;
        }
    }
    
    suggestedAlbums.sort();
    otherAlbums.sort();
    
    QStringList finalAlbums = suggestedAlbums;
    finalAlbums << otherAlbums;
    
    bool ok;
    QString albumName;
    
    if (finalAlbums.isEmpty()) {
        albumName = QInputDialog::getText(this, "Добавить альбом", 
                                         "Введите название альбома:",
                                         QLineEdit::Normal, "", &ok);
    } else {
        finalAlbums << "Другое...";
        
        albumName = QInputDialog::getItem(this, "Добавить альбом", 
                                          "Выберите альбом:",
                                          finalAlbums, 0, false, &ok);
        
        if (ok && albumName == "Другое...") {
            albumName = QInputDialog::getText(this, "Добавить альбом", 
                                             "Введите название альбома:",
                                             QLineEdit::Normal, "", &ok);
        }
    }
    
    if (ok && !albumName.isEmpty() && albumName != "Другое...") {
        int albumId = m_dbManager->addAlbum(albumName);
        if (albumId >= 0) {
            if (m_dbManager->addAlbumToTrack(track.id, albumId)) {
                loadTracks();
                statusBar()->showMessage(QString("Альбом '%1' добавлен").arg(albumName), 2000);
            }
        }
    }
}

void MainWindow::onRemoveAlbum()
{
    QModelIndex index = m_trackList->currentIndex();
    if (!index.isValid()) {
        return;
    }
    
    TrackInfo track = m_playlistModel->trackAt(index.row());
    if (track.id < 0 || track.album.isEmpty()) {
        QMessageBox::information(this, "Нет альбомов", "У этого трека нет альбомов.");
        return;
    }
    
    TrackInfo fullTrack = m_dbManager->getTrack(track.id);
    QStringList albums = fullTrack.album.split(", ");
    
    if (albums.isEmpty()) {
        QMessageBox::information(this, "Нет альбомов", "У этого трека нет альбомов.");
        return;
    }
    
    bool ok;
    QString albumName = QInputDialog::getItem(this, "Удалить альбом", "Выберите альбом для удаления:",
                                            albums, 0, false, &ok);
    if (ok && !albumName.isEmpty()) {
        int albumId = m_dbManager->getAlbumId(albumName);
        if (albumId >= 0) {
            if (m_dbManager->removeAlbumFromTrack(track.id, albumId)) {
                loadTracks();
                statusBar()->showMessage(QString("Альбом '%1' удалён").arg(albumName), 2000);
            }
        }
    }
}

void MainWindow::onSliderPressed()
{
    m_seeking = true;
}

void MainWindow::onSliderReleased()
{
    m_seeking = false;
    m_audioPlayer->setPosition(m_positionSlider->value());
}

void MainWindow::onSeekPositionChanged(int value)
{
    if (m_seeking) {
        updateTimeLabels();
    }
}

void MainWindow::updatePlayButton()
{
    if (m_audioPlayer->state() == QMediaPlayer::PlayingState) {
        m_playPauseBtn->setText("Пауза");
    } else {
        m_playPauseBtn->setText("Воспроизведение");
    }
}

void MainWindow::updateTimeLabels()
{
    qint64 position = m_seeking ? m_positionSlider->value() : m_audioPlayer->position();
    qint64 duration = m_audioPlayer->duration();
    m_timeLabel->setText(formatTime(position) + " / " + formatTime(duration));
}

void MainWindow::updateAlbumCover()
{
    TrackInfo track = m_audioPlayer->currentTrack();
    
    if (track.id < 0) {
        QModelIndex currentIndex = m_trackList->currentIndex();
        if (currentIndex.isValid()) {
            track = m_playlistModel->trackAt(currentIndex.row());
        }
    }
    
    updateAlbumCoverForTrack(track);
}

void MainWindow::updateAlbumCoverForTrack(const TrackInfo &track)
{
    if (track.id < 0) {
        m_albumCoverLabel->setText("Нет обложки");
        m_albumCoverLabel->setPixmap(QPixmap());
        return;
    }
    
    TrackInfo dbTrack = m_dbManager->getTrack(track.id);
    
    if (!dbTrack.coverPath.isEmpty()) {
        QFileInfo coverFile(dbTrack.coverPath);
        if (coverFile.exists() && coverFile.isFile()) {
            QPixmap pixmap(dbTrack.coverPath);
            if (!pixmap.isNull()) {
                QSize labelSize = m_albumCoverLabel->size();
                if (labelSize.width() <= 0 || labelSize.height() <= 0) {
                    labelSize = QSize(250, 250);
                }
                pixmap = pixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                m_albumCoverLabel->setPixmap(pixmap);
                m_albumCoverLabel->setText("");
                return;
            }
        }
    }
    
    QFileInfo fileInfo(dbTrack.filePath);
    if (fileInfo.exists()) {
        QDir dir = fileInfo.dir();
        QStringList coverNames = {"cover.jpg", "cover.png", "folder.jpg", "folder.png", 
                                  "album.jpg", "album.png", "artwork.jpg", "artwork.png"};
        
        for (const QString &coverName : coverNames) {
            QString coverPath = dir.absoluteFilePath(coverName);
            if (QFileInfo::exists(coverPath)) {
                QPixmap pixmap(coverPath);
                if (!pixmap.isNull()) {
                    QSize labelSize = m_albumCoverLabel->size();
                    if (labelSize.width() <= 0 || labelSize.height() <= 0) {
                        labelSize = QSize(250, 250);
                    }
                    pixmap = pixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    m_albumCoverLabel->setPixmap(pixmap);
                    m_albumCoverLabel->setText("");
                    return;
                }
            }
        }
    }
    
    m_albumCoverLabel->setText("Нет обложки");
    m_albumCoverLabel->setPixmap(QPixmap());
}

void MainWindow::loadPlaylists()
{
    m_playlistCombo->clear();
    m_playlistCombo->addItem("Все треки");
    
    QList<PlaylistInfo> playlists = m_dbManager->getAllPlaylists();
    for (const PlaylistInfo &playlist : playlists) {
        m_playlistCombo->addItem(playlist.name);
    }
}

void MainWindow::loadTracks()
{
    QList<TrackInfo> tracks;
    
    if (m_currentPlaylistId >= 0) {
        tracks = m_dbManager->getPlaylistTracks(m_currentPlaylistId);
    } else {
        tracks = m_dbManager->getAllTracks();
    }
    
    m_playlistModel->setTracks(tracks);
    
    TrackInfo playingTrack = m_audioPlayer->currentTrack();
    if (playingTrack.id < 0) {
        QModelIndex currentIndex = m_trackList->currentIndex();
        if (currentIndex.isValid()) {
            TrackInfo selectedTrack = m_playlistModel->trackAt(currentIndex.row());
            updateAlbumCoverForTrack(selectedTrack);
        } else if (tracks.size() > 0) {
            updateAlbumCoverForTrack(tracks.first());
        }
    }
    
    QStringList artists, albums;
    QStringList allArtists = m_dbManager->getAllArtists();
    artists = allArtists;
    
    QStringList allAlbums = m_dbManager->getAllAlbums();
    albums = allAlbums;
    
    for (const TrackInfo &track : tracks) {
        if (!track.artist.isEmpty()) {
            QStringList trackArtists = track.artist.split(", ");
            for (const QString &artist : trackArtists) {
                if (!artists.contains(artist)) {
                    artists << artist;
                }
            }
        }
        if (!track.album.isEmpty()) {
            QStringList trackAlbums = track.album.split(", ");
            for (const QString &album : trackAlbums) {
                if (!albums.contains(album)) {
                    albums << album;
                }
            }
        }
    }
    
    QString currentArtist = m_artistFilter->currentText();
    QString currentAlbum = m_albumFilter->currentText();
    
    m_artistFilter->clear();
    m_artistFilter->addItem("Все исполнители");
    m_artistFilter->addItems(artists);
    if (artists.contains(currentArtist)) {
        m_artistFilter->setCurrentText(currentArtist);
    }
    
    m_albumFilter->clear();
    m_albumFilter->addItem("Все альбомы");
    m_albumFilter->addItems(albums);
    if (albums.contains(currentAlbum)) {
        m_albumFilter->setCurrentText(currentAlbum);
    }
    
}

void MainWindow::loadHistory()
{
    QList<TrackInfo> history = m_dbManager->getHistory(50);
    m_historyModel->setTracks(history);
}

void MainWindow::applyFilters()
{
    QString searchText = m_searchEdit->text();
    QString artist = m_artistFilter->currentText();
    QString album = m_albumFilter->currentText();
    
    QList<TrackInfo> tracks;
    
    if (!searchText.isEmpty()) {
        tracks = m_dbManager->searchTracks(searchText);
    } else if (artist != "Все исполнители" || album != "Все альбомы") {
        tracks = m_dbManager->filterTracks(
            artist != "Все исполнители" ? artist : "",
            album != "Все альбомы" ? album : "",
            QStringList()
        );
    } else {
        if (m_currentPlaylistId >= 0) {
            tracks = m_dbManager->getPlaylistTracks(m_currentPlaylistId);
        } else {
            tracks = m_dbManager->getAllTracks();
        }
    }
    
    m_playlistModel->setTracks(tracks);
}

QString MainWindow::formatTime(qint64 milliseconds) const
{
    int seconds = milliseconds / 1000;
    int minutes = seconds / 60;
    seconds %= 60;
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0'))
                          .arg(seconds, 2, 10, QChar('0'));
}

QString MainWindow::convertMp4ToMp3(const QString &mp4Path)
{
    QFileInfo fileInfo(mp4Path);
    QString outputDir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation) + "/Converted";
    QDir().mkpath(outputDir);
    
    QString mp3Path = outputDir + "/" + fileInfo.baseName() + ".mp3";
    
    int counter = 1;
    QString originalMp3Path = mp3Path;
    while (QFileInfo::exists(mp3Path)) {
        mp3Path = outputDir + "/" + fileInfo.baseName() + "_" + QString::number(counter) + ".mp3";
        counter++;
    }
    
    QString ffmpegPath = "ffmpeg";
    
    QProcess process;
    QStringList arguments;
    arguments << "-i" << mp4Path
              << "-vn"
              << "-acodec" << "libmp3lame"
              << "-ab" << "192k"
              << "-ar" << "44100"
              << "-y"
              << mp3Path;
    
    process.start(ffmpegPath, arguments);
    
    if (!process.waitForFinished(300000)) {
        return QString();
    }
    
    if (process.exitCode() != 0) {
        QString errorOutput = process.readAllStandardError();
        return QString();
    }
    
    if (QFileInfo::exists(mp3Path)) {
        return mp3Path;
    }
    
    return QString();
}

