#include "audioplayer.h"
#include <QMediaMetaData>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QImage>

AudioPlayer::AudioPlayer(DatabaseManager *dbManager, QObject *parent)
    : QObject(parent)
    , m_dbManager(dbManager)
    , m_trackLoaded(false)
    , m_autoPlay(false)
{
    m_audioOutput = new QAudioOutput(QMediaDevices::defaultAudioOutput(), this);
    m_player = new QMediaPlayer(this);
    m_player->setAudioOutput(m_audioOutput);
    
    connect(m_player, &QMediaPlayer::mediaStatusChanged, 
            this, &AudioPlayer::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::errorOccurred, 
            this, &AudioPlayer::onErrorOccurred);
    connect(m_player, &QMediaPlayer::positionChanged, 
            this, &AudioPlayer::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, 
            this, &AudioPlayer::onDurationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, 
            this, &AudioPlayer::onStateChanged);
}

AudioPlayer::~AudioPlayer()
{
    stop();
}

void AudioPlayer::setTrack(const TrackInfo &track)
{
    if (track.id < 0) {
        return;
    }
    
    stop();
    m_currentTrack = track;
    m_trackLoaded = false;
    m_autoPlay = false;
    
    if (!QFileInfo::exists(track.filePath)) {
        emit errorOccurred(QString("Файл не найден: %1").arg(track.filePath));
        return;
    }
    
    QUrl url = QUrl::fromLocalFile(track.filePath);
    m_player->setSource(url);
    
    emit trackChanged(track);
}

void AudioPlayer::play()
{
    if (m_trackLoaded) {
        m_player->play();
    } else {
        m_autoPlay = true;
        m_player->play();
    }
}

void AudioPlayer::pause()
{
    m_player->pause();
}

void AudioPlayer::stop()
{
    m_player->stop();
}

void AudioPlayer::setPosition(qint64 position)
{
    m_player->setPosition(position);
}

void AudioPlayer::setVolume(float volume)
{
    m_audioOutput->setVolume(volume);
}

QMediaPlayer::PlaybackState AudioPlayer::state() const
{
    return m_player->playbackState();
}

qint64 AudioPlayer::position() const
{
    return m_player->position();
}

qint64 AudioPlayer::duration() const
{
    return m_player->duration();
}

float AudioPlayer::volume() const
{
    return m_audioOutput->volume();
}

void AudioPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
        m_trackLoaded = true;
        extractMetadata(m_player->source());
        emit durationChanged(m_player->duration());
        
        if (m_autoPlay) {
            m_autoPlay = false;
            m_player->play();
        }
    } else if (status == QMediaPlayer::InvalidMedia) {
        emit errorOccurred("Неверный медиа файл или формат не поддерживается");
        m_trackLoaded = false;
        m_autoPlay = false;
    } else if (status == QMediaPlayer::NoMedia) {
        m_trackLoaded = false;
    }
}

void AudioPlayer::onErrorOccurred(QMediaPlayer::Error error, const QString &errorString)
{
    QString fullError = QString("Ошибка воспроизведения: %1 (код: %2)").arg(errorString).arg(error);
    emit errorOccurred(fullError);
    qWarning() << "Ошибка аудио плеера:" << fullError;
    m_trackLoaded = false;
    m_autoPlay = false;
}

void AudioPlayer::onPositionChanged(qint64 position)
{
    emit positionChanged(position);
}

void AudioPlayer::onDurationChanged(qint64 duration)
{
    emit durationChanged(duration);
}

void AudioPlayer::onStateChanged(QMediaPlayer::PlaybackState state)
{
    emit stateChanged(state);
}

void AudioPlayer::extractMetadata(const QUrl &url)
{
    if (!m_dbManager || m_currentTrack.id < 0) {
        return;
    }
    
    QString title = m_player->metaData().value(QMediaMetaData::Title).toString();
    QString artist = m_player->metaData().value(QMediaMetaData::AlbumArtist).toString();
    if (artist.isEmpty()) {
        artist = m_player->metaData().value(QMediaMetaData::ContributingArtist).toString();
    }
    QString album = m_player->metaData().value(QMediaMetaData::AlbumTitle).toString();
    
    if (title.isEmpty()) {
        QFileInfo fileInfo(url.toLocalFile());
        title = fileInfo.baseName();
    }
    
    int duration = m_player->duration() / 1000;
    
    TrackInfo dbTrack = m_dbManager->getTrack(m_currentTrack.id);
    QString existingCoverPath = dbTrack.coverPath;
    
    bool hasUserCover = !existingCoverPath.isEmpty() && QFileInfo::exists(existingCoverPath);
    
    QString coverPath = existingCoverPath;
    QVariant coverVariant = m_player->metaData().value(QMediaMetaData::CoverArtImage);
    if (coverVariant.isValid() && !hasUserCover) {
        QImage coverImage = coverVariant.value<QImage>();
        if (!coverImage.isNull()) {
            QString coversDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/covers";
            QDir().mkpath(coversDir);
            
            QString fileName = QString::number(m_currentTrack.id) + ".jpg";
            QString newCoverPath = coversDir + "/" + fileName;
            
            if (coverImage.save(newCoverPath, "JPG")) {
                coverPath = newCoverPath;
            }
        }
    }
    
    m_dbManager->updateTrackInfo(m_currentTrack.id, title, artist, album, duration, coverPath);
    
    m_currentTrack = m_dbManager->getTrack(m_currentTrack.id);
    emit trackChanged(m_currentTrack);
}

