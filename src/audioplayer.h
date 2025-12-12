#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QUrl>
#include <QString>
#include "databasemanager.h"

class AudioPlayer : public QObject
{
    Q_OBJECT

public:
    explicit AudioPlayer(DatabaseManager *dbManager, QObject *parent = nullptr);
    ~AudioPlayer();
    void setTrack(const TrackInfo &track);
    void play();
    void pause();
    void stop();
    void setPosition(qint64 position);
    void setVolume(float volume);
    QMediaPlayer::PlaybackState state() const;
    qint64 position() const;
    qint64 duration() const;
    float volume() const;
    TrackInfo currentTrack() const { return m_currentTrack; }
    QAudioOutput* audioOutput() const { return m_audioOutput; }

signals:
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void stateChanged(QMediaPlayer::PlaybackState state);
    void trackChanged(const TrackInfo &track);
    void errorOccurred(const QString &error);

private slots:
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onErrorOccurred(QMediaPlayer::Error error, const QString &errorString);
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onStateChanged(QMediaPlayer::PlaybackState state);

private:
    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
    DatabaseManager *m_dbManager;
    TrackInfo m_currentTrack;
    bool m_trackLoaded;
    bool m_autoPlay;
    void extractMetadata(const QUrl &url);
};

#endif

