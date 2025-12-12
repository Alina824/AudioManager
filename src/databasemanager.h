#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QUrl>

struct TrackInfo {
    int id;
    QString filePath;
    QString title;
    QString artist;
    QString album;
    int duration;
    QStringList tags;
    QString coverPath;
    QDateTime lastPlayed;
    int playCount;
};

struct PlaylistInfo {
    int id;
    QString name;
    QDateTime created;
    QDateTime modified;
};

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    bool initializeDatabase();
    
    int addTrack(const QString &filePath, const QString &title = "", 
                 const QString &artist = "", const QString &album = "");
    bool updateTrackInfo(int trackId, const QString &title, 
                        const QString &artist, const QString &album, 
                        int duration, const QString &coverPath = "");
    TrackInfo getTrack(int trackId);
    QList<TrackInfo> getAllTracks();
    QList<TrackInfo> searchTracks(const QString &query);
    QList<TrackInfo> filterTracks(const QString &artist = "", 
                                  const QString &album = "", 
                                  const QStringList &tags = QStringList());
    bool deleteTrack(int trackId);
    
    int createPlaylist(const QString &name);
    bool deletePlaylist(int playlistId);
    QList<PlaylistInfo> getAllPlaylists();
    bool addTrackToPlaylist(int playlistId, int trackId, int position = -1);
    bool removeTrackFromPlaylist(int playlistId, int trackId);
    QList<TrackInfo> getPlaylistTracks(int playlistId);
    bool updatePlaylistName(int playlistId, const QString &name);
    
    void addToHistory(int trackId);
    QList<TrackInfo> getHistory(int limit = 100);
    void clearHistory();
    
    bool addTagToTrack(int trackId, const QString &tag);
    bool removeTagFromTrack(int trackId, const QString &tag);
    QStringList getAllTags();
    QList<TrackInfo> getTracksByTag(const QString &tag);
    
    void incrementPlayCount(int trackId);
    void updateLastPlayed(int trackId);
    
    int addArtist(const QString &name);
    bool addArtistToTrack(int trackId, int artistId);
    bool removeArtistFromTrack(int trackId, int artistId);
    QStringList getAllArtists();
    QList<TrackInfo> getTracksByArtist(int artistId);
    int getArtistId(const QString &name);
    
    int addAlbum(const QString &name);
    bool addAlbumToTrack(int trackId, int albumId);
    bool removeAlbumFromTrack(int trackId, int albumId);
    QStringList getAllAlbums();
    QList<TrackInfo> getTracksByAlbum(int albumId);
    int getAlbumId(const QString &name);
    QStringList getAlbumsByArtist(int artistId);

private:
    QSqlDatabase m_database;
    bool createTables();
};

#endif

