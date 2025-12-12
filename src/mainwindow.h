#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListView>
#include <QTreeView>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QTabWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QGroupBox>
#include <QScrollArea>
#include "audioplayer.h"
#include "databasemanager.h"
#include "playlistmodel.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddFiles();
    void onAddFolder();
    void onPlayPause();
    void onStop();
    void onPrevious();
    void onNext();
    void onShuffle();
    void onRepeat();
    void onVolumeChanged(int value);
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onStateChanged(QMediaPlayer::PlaybackState state);
    void onTrackChanged(const TrackInfo &track);
    void onTrackDoubleClicked(const QModelIndex &index);
    void onSearchTextChanged(const QString &text);
    void onFilterChanged();
    void onPlaylistSelected(int index);
    void onCreatePlaylist();
    void onDeletePlaylist();
    void onAddToPlaylist();
    void onRemoveFromPlaylist();
    void onShowHistory();
    void onAddArtist();
    void onRemoveArtist();
    void onAddAlbum();
    void onRemoveAlbum();
    void onLoadCoverImage();
    void onDeleteTrack();
    void onSliderPressed();
    void onSliderReleased();
    void onSeekPositionChanged(int value);

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupConnections();
    void updatePlayButton();
    void updateTimeLabels();
    void updateAlbumCover();
    void updateAlbumCoverForTrack(const TrackInfo &track);
    void loadPlaylists();
    void loadTracks();
    void loadHistory();
    void applyFilters();
    QString formatTime(qint64 milliseconds) const;
    QString convertMp4ToMp3(const QString &mp4Path);
    QWidget *m_centralWidget;
    QSplitter *m_mainSplitter;
    QSplitter *m_leftSplitter;
    QWidget *m_leftPanel;
    QTabWidget *m_leftTabs;
    QTreeView *m_playlistTree;
    QListView *m_historyList;
    QComboBox *m_playlistCombo;
    QPushButton *m_createPlaylistBtn;
    QPushButton *m_deletePlaylistBtn;
    QWidget *m_centerPanel;
    QListView *m_trackList;
    QLineEdit *m_searchEdit;
    QComboBox *m_artistFilter;
    QComboBox *m_albumFilter;
    QPushButton *m_clearFiltersBtn;
    QPushButton *m_addToPlaylistBtn;
    QWidget *m_rightPanel;
    QLabel *m_albumCoverLabel;
    QLabel *m_trackInfoLabel;
    QWidget *m_controlsPanel;
    QPushButton *m_playPauseBtn;
    QPushButton *m_stopBtn;
    QPushButton *m_previousBtn;
    QPushButton *m_nextBtn;
    QPushButton *m_shuffleBtn;
    QPushButton *m_repeatBtn;
    QSlider *m_positionSlider;
    QSlider *m_volumeSlider;
    QLabel *m_timeLabel;
    QLabel *m_volumeLabel;
    DatabaseManager *m_dbManager;
    AudioPlayer *m_audioPlayer;
    PlaylistModel *m_playlistModel;
    PlaylistModel *m_historyModel;
    int m_currentPlaylistId;
    int m_currentTrackIndex;
    bool m_shuffleEnabled;
    bool m_repeatEnabled;
    bool m_seeking;
    QAction *m_addFilesAction;
    QAction *m_addFolderAction;
    QAction *m_exitAction;
    QAction *m_showHistoryAction;
};

#endif

