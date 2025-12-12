#include "databasemanager.h"
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dbPath);
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(dbPath + "/audioplayer.db");
    if (!m_database.open()) {
        qWarning() << "Не удалось открыть базу данных:" << m_database.lastError();
    }
}

DatabaseManager::~DatabaseManager()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

bool DatabaseManager::initializeDatabase()
{
    return createTables();
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(m_database);
    query.exec("CREATE TABLE IF NOT EXISTS tracks ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "file_path TEXT UNIQUE NOT NULL,"
               "title TEXT,"
               "artist TEXT,"
               "album TEXT,"
               "duration INTEGER DEFAULT 0,"
               "cover_path TEXT,"
               "last_played DATETIME,"
               "play_count INTEGER DEFAULT 0"
               ")");
    if (query.lastError().isValid()) {
        qWarning() << "Ошибка создания таблицы треков:" << query.lastError();
        return false;
    }

    query.exec("CREATE TABLE IF NOT EXISTS playlists ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "name TEXT NOT NULL,"
               "created DATETIME DEFAULT CURRENT_TIMESTAMP,"
               "modified DATETIME DEFAULT CURRENT_TIMESTAMP"
               ")");
    if (query.lastError().isValid()) {
        qWarning() << "Ошибка создания таблицы плейлистов:" << query.lastError();
        return false;
    }

    query.exec("CREATE TABLE IF NOT EXISTS playlist_tracks ("
               "playlist_id INTEGER NOT NULL,"
               "track_id INTEGER NOT NULL,"
               "position INTEGER NOT NULL,"
               "PRIMARY KEY (playlist_id, track_id),"
               "FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE CASCADE,"
               "FOREIGN KEY (track_id) REFERENCES tracks(id) ON DELETE CASCADE"
               ")");
    if (query.lastError().isValid()) {
        qWarning() << "Ошибка создания таблицы связи плейлистов и треков:" << query.lastError();
        return false;
    }
    
    query.exec("CREATE TABLE IF NOT EXISTS tags ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "name TEXT UNIQUE NOT NULL"
               ")");
    if (query.lastError().isValid()) {
        qWarning() << "Ошибка создания таблицы тегов:" << query.lastError();
        return false;
    }
    
    query.exec("CREATE TABLE IF NOT EXISTS track_tags ("
               "track_id INTEGER NOT NULL,"
               "tag_id INTEGER NOT NULL,"
               "PRIMARY KEY (track_id, tag_id),"
               "FOREIGN KEY (track_id) REFERENCES tracks(id) ON DELETE CASCADE,"
               "FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE"
               ")");
    if (query.lastError().isValid()) {
        qWarning() << "Ошибка создания таблицы связи треков и тегов:" << query.lastError();
        return false;
    }
    
    query.exec("CREATE TABLE IF NOT EXISTS history ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "track_id INTEGER NOT NULL,"
               "played_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
               "FOREIGN KEY (track_id) REFERENCES tracks(id) ON DELETE CASCADE"
               ")");
    if (query.lastError().isValid()) {
        qWarning() << "Ошибка создания таблицы истории:" << query.lastError();
        return false;
    }
    
    query.exec("CREATE TABLE IF NOT EXISTS artists ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "name TEXT UNIQUE NOT NULL"
               ")");
    if (query.lastError().isValid()) {
        qWarning() << "Ошибка создания таблицы исполнителей:" << query.lastError();
        return false;
    }
    query.exec("CREATE TABLE IF NOT EXISTS track_artists ("
               "track_id INTEGER NOT NULL,"
               "artist_id INTEGER NOT NULL,"
               "PRIMARY KEY (track_id, artist_id),"
               "FOREIGN KEY (track_id) REFERENCES tracks(id) ON DELETE CASCADE,"
               "FOREIGN KEY (artist_id) REFERENCES artists(id) ON DELETE CASCADE"
               ")");
    if (query.lastError().isValid()) {
        qWarning() << "Ошибка создания таблицы связи треков и исполнителей:" << query.lastError();
        return false;
    }
    query.exec("CREATE TABLE IF NOT EXISTS albums ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "name TEXT UNIQUE NOT NULL"
               ")");
    if (query.lastError().isValid()) {
        qWarning() << "Ошибка создания таблицы альбомов:" << query.lastError();
        return false;
    }
    
    query.exec("CREATE TABLE IF NOT EXISTS track_albums ("
               "track_id INTEGER NOT NULL,"
               "album_id INTEGER NOT NULL,"
               "PRIMARY KEY (track_id, album_id),"
               "FOREIGN KEY (track_id) REFERENCES tracks(id) ON DELETE CASCADE,"
               "FOREIGN KEY (album_id) REFERENCES albums(id) ON DELETE CASCADE"
               ")");
    if (query.lastError().isValid()) {
        qWarning() << "Ошибка создания таблицы связи треков и альбомов:" << query.lastError();
        return false;
    }
    query.exec("CREATE INDEX IF NOT EXISTS idx_tracks_artist ON tracks(artist)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_tracks_album ON tracks(album)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_history_played_at ON history(played_at DESC)");
    
    return true;
}

int DatabaseManager::addTrack(const QString &filePath, const QString &title, 
                              const QString &artist, const QString &album)
{
    QSqlQuery query(m_database);
    query.prepare("INSERT OR IGNORE INTO tracks (file_path, title, artist, album) "
                  "VALUES (:file_path, :title, :artist, :album)");
    query.bindValue(":file_path", filePath);
    query.bindValue(":title", title);
    query.bindValue(":artist", artist);
    query.bindValue(":album", album);
    if (!query.exec()) {
        qWarning() << "Ошибка добавления трека:" << query.lastError();
        return -1;
    }
    if (query.numRowsAffected() == 0) {
        query.prepare("SELECT id FROM tracks WHERE file_path = :file_path");
        query.bindValue(":file_path", filePath);
        if (query.exec() && query.next()) {
            return query.value(0).toInt();
        }
        return -1;
    }
    return query.lastInsertId().toInt();
}

bool DatabaseManager::updateTrackInfo(int trackId, const QString &title, 
                                      const QString &artist, const QString &album, 
                                      int duration, const QString &coverPath)
{
    QSqlQuery query(m_database);
    query.prepare("UPDATE tracks SET title = :title, artist = :artist, "
                  "album = :album, duration = :duration, cover_path = :cover_path "
                  "WHERE id = :id");
    query.bindValue(":title", title);
    query.bindValue(":artist", artist);
    query.bindValue(":album", album);
    query.bindValue(":duration", duration);
    query.bindValue(":cover_path", coverPath);
    query.bindValue(":id", trackId);
    if (!query.exec()) {
        qWarning() << "Ошибка обновления трека:" << query.lastError();
        return false;
    }
    return true;
}

TrackInfo DatabaseManager::getTrack(int trackId)
{
    TrackInfo track;
    track.id = -1;
    QSqlQuery query(m_database);
    query.prepare("SELECT id, file_path, title, artist, album, duration, "
                  "cover_path, last_played, play_count FROM tracks WHERE id = :id");
    query.bindValue(":id", trackId);
    if (query.exec() && query.next()) {
        track.id = query.value(0).toInt();
        track.filePath = query.value(1).toString();
        track.title = query.value(2).toString();
        track.artist = query.value(3).toString();
        track.album = query.value(4).toString();
        track.duration = query.value(5).toInt();
        track.coverPath = query.value(6).toString();
        track.lastPlayed = query.value(7).toDateTime();
        track.playCount = query.value(8).toInt();
        QSqlQuery tagQuery(m_database);
        tagQuery.prepare("SELECT t.name FROM tags t "
                        "JOIN track_tags tt ON t.id = tt.tag_id "
                        "WHERE tt.track_id = :track_id");
        tagQuery.bindValue(":track_id", trackId);
        if (tagQuery.exec()) {
            while (tagQuery.next()) {
                track.tags << tagQuery.value(0).toString();
            }
        }
        QSqlQuery artistQuery(m_database);
        artistQuery.prepare("SELECT a.name FROM artists a "
                           "JOIN track_artists ta ON a.id = ta.artist_id "
                           "WHERE ta.track_id = :track_id ORDER BY a.name");
        artistQuery.bindValue(":track_id", trackId);
        QStringList artistsFromTable;
        if (artistQuery.exec()) {
            while (artistQuery.next()) {
                artistsFromTable << artistQuery.value(0).toString();
            }
        }
        if (!artistsFromTable.isEmpty()) {
            track.artist = artistsFromTable.join(", ");
        }
        
        QSqlQuery albumQuery(m_database);
        albumQuery.prepare("SELECT a.name FROM albums a "
                          "JOIN track_albums ta ON a.id = ta.album_id "
                          "WHERE ta.track_id = :track_id ORDER BY a.name");
        albumQuery.bindValue(":track_id", trackId);
        QStringList albumsFromTable;
        if (albumQuery.exec()) {
            while (albumQuery.next()) {
                albumsFromTable << albumQuery.value(0).toString();
            }
        }
        if (!albumsFromTable.isEmpty()) {
            track.album = albumsFromTable.join(", ");
        }
    }
    
    return track;
}

QList<TrackInfo> DatabaseManager::getAllTracks()
{
    QList<TrackInfo> tracks;
    QSqlQuery query(m_database);
    query.exec("SELECT id FROM tracks ORDER BY title");
    while (query.next()) {
        tracks << getTrack(query.value(0).toInt());
    }
    return tracks;
}

QList<TrackInfo> DatabaseManager::searchTracks(const QString &searchQuery)
{
    QList<TrackInfo> tracks;
    QSqlQuery query(m_database);
    query.prepare("SELECT id FROM tracks WHERE "
                  "title LIKE :query OR artist LIKE :query OR album LIKE :query "
                  "ORDER BY title");
    query.bindValue(":query", "%" + searchQuery + "%");
    
    if (query.exec()) {
        while (query.next()) {
            tracks << getTrack(query.value(0).toInt());
        }
    }
    return tracks;
}

QList<TrackInfo> DatabaseManager::filterTracks(const QString &artist, 
                                                const QString &album, 
                                                const QStringList &tags)
{
    QList<TrackInfo> tracks;
    QString sql = "SELECT DISTINCT t.id FROM tracks t";
    QStringList conditions;
    QVariantList bindValues;
    if (!artist.isEmpty()) {
        sql += " LEFT JOIN track_artists ta ON t.id = ta.track_id "
               "LEFT JOIN artists a ON ta.artist_id = a.id";
        conditions << "(a.name = ? OR t.artist = ? OR t.artist LIKE ?)";
        bindValues << artist;
        bindValues << artist;
        bindValues << "%" + artist + "%";
    }
    if (!album.isEmpty()) {
        sql += " LEFT JOIN track_albums tal ON t.id = tal.track_id "
               "LEFT JOIN albums al ON tal.album_id = al.id";
        conditions << "(al.name = ? OR t.album = ? OR t.album LIKE ?)";
        bindValues << album;
        bindValues << album;
        bindValues << "%" + album + "%";
    }

    if (!tags.isEmpty()) {
        sql += " JOIN track_tags tt ON t.id = tt.track_id "
               "JOIN tags tag ON tt.tag_id = tag.id";
        QStringList tagPlaceholders;
        for (int i = 0; i < tags.size(); ++i) {
            tagPlaceholders << "?";
            bindValues << tags[i];
        }
        conditions << "tag.name IN (" + tagPlaceholders.join(",") + ")";
    }
    if (!conditions.isEmpty()) {
        sql += " WHERE " + conditions.join(" AND ");
    }
    sql += " ORDER BY t.title";
    QSqlQuery query(m_database);
    query.prepare(sql);
    for (int i = 0; i < bindValues.size(); ++i) {
        query.bindValue(i, bindValues[i]);
    }

    if (query.exec()) {
        while (query.next()) {
            tracks << getTrack(query.value(0).toInt());
        }
    } else {
        qWarning() << "Ошибка фильтрации треков:" << query.lastError().text();
        qWarning() << "SQL:" << sql;
    }
    
    return tracks;
}

bool DatabaseManager::deleteTrack(int trackId)
{
    QSqlQuery query(m_database);
    
    query.prepare("DELETE FROM track_tags WHERE track_id = :track_id");
    query.bindValue(":track_id", trackId);
    query.exec();
    
    query.prepare("DELETE FROM track_artists WHERE track_id = :track_id");
    query.bindValue(":track_id", trackId);
    query.exec();
    
    query.prepare("DELETE FROM track_albums WHERE track_id = :track_id");
    query.bindValue(":track_id", trackId);
    query.exec();
    
    query.prepare("DELETE FROM playlist_tracks WHERE track_id = :track_id");
    query.bindValue(":track_id", trackId);
    query.exec();
    
    query.prepare("DELETE FROM history WHERE track_id = :track_id");
    query.bindValue(":track_id", trackId);
    query.exec();
    
    query.prepare("DELETE FROM tracks WHERE id = :id");
    query.bindValue(":id", trackId);
    if (!query.exec()) {
        qWarning() << "Ошибка удаления трека:" << query.lastError();
        return false;
    }
    
    return true;
}

int DatabaseManager::createPlaylist(const QString &name)
{
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO playlists (name) VALUES (:name)");
    query.bindValue(":name", name);
    if (!query.exec()) {
        qWarning() << "Ошибка создания плейлиста:" << query.lastError();
        return -1;
    }
    return query.lastInsertId().toInt();
}

bool DatabaseManager::deletePlaylist(int playlistId)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM playlists WHERE id = :id");
    query.bindValue(":id", playlistId);
    return query.exec();
}

QList<PlaylistInfo> DatabaseManager::getAllPlaylists()
{
    QList<PlaylistInfo> playlists;
    QSqlQuery query(m_database);
    query.exec("SELECT id, name, created, modified FROM playlists ORDER BY name");
    while (query.next()) {
        PlaylistInfo playlist;
        playlist.id = query.value(0).toInt();
        playlist.name = query.value(1).toString();
        playlist.created = query.value(2).toDateTime();
        playlist.modified = query.value(3).toDateTime();
        playlists << playlist;
    }
    
    return playlists;
}

bool DatabaseManager::addTrackToPlaylist(int playlistId, int trackId, int position)
{
    if (position < 0) {
        QSqlQuery maxQuery(m_database);
        maxQuery.prepare("SELECT COALESCE(MAX(position), -1) FROM playlist_tracks WHERE playlist_id = :id");
        maxQuery.bindValue(":id", playlistId);
        if (maxQuery.exec() && maxQuery.next()) {
            position = maxQuery.value(0).toInt() + 1;
        } else {
            position = 0;
        }
    }
    
    QSqlQuery query(m_database);
    query.prepare("INSERT OR IGNORE INTO playlist_tracks (playlist_id, track_id, position) "
                  "VALUES (:playlist_id, :track_id, :position)");
    query.bindValue(":playlist_id", playlistId);
    query.bindValue(":track_id", trackId);
    query.bindValue(":position", position);
    
    if (!query.exec()) {
        qWarning() << "Ошибка добавления трека в плейлист:" << query.lastError();
        return false;
    }
    QSqlQuery updateQuery(m_database);
    updateQuery.prepare("UPDATE playlists SET modified = CURRENT_TIMESTAMP WHERE id = :id");
    updateQuery.bindValue(":id", playlistId);
    updateQuery.exec();
    return true;
}

bool DatabaseManager::removeTrackFromPlaylist(int playlistId, int trackId)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM playlist_tracks WHERE playlist_id = :playlist_id AND track_id = :track_id");
    query.bindValue(":playlist_id", playlistId);
    query.bindValue(":track_id", trackId);
    return query.exec();
}

QList<TrackInfo> DatabaseManager::getPlaylistTracks(int playlistId)
{
    QList<TrackInfo> tracks;
    QSqlQuery query(m_database);
    query.prepare("SELECT t.id FROM tracks t "
                  "JOIN playlist_tracks pt ON t.id = pt.track_id "
                  "WHERE pt.playlist_id = :playlist_id "
                  "ORDER BY pt.position");
    query.bindValue(":playlist_id", playlistId);
    if (query.exec()) {
        while (query.next()) {
            tracks << getTrack(query.value(0).toInt());
        }
    }
    return tracks;
}

bool DatabaseManager::updatePlaylistName(int playlistId, const QString &name)
{
    QSqlQuery query(m_database);
    query.prepare("UPDATE playlists SET name = :name, modified = CURRENT_TIMESTAMP WHERE id = :id");
    query.bindValue(":name", name);
    query.bindValue(":id", playlistId);
    return query.exec();
}

void DatabaseManager::addToHistory(int trackId)
{
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO history (track_id) VALUES (:track_id)");
    query.bindValue(":track_id", trackId);
    query.exec();
    incrementPlayCount(trackId);
    updateLastPlayed(trackId);
}

QList<TrackInfo> DatabaseManager::getHistory(int limit)
{
    QList<TrackInfo> tracks;
    QSqlQuery query(m_database);
    query.prepare("SELECT DISTINCT t.id FROM tracks t "
                  "JOIN history h ON t.id = h.track_id "
                  "ORDER BY h.played_at DESC LIMIT :limit");
    query.bindValue(":limit", limit);
    if (query.exec()) {
        while (query.next()) {
            tracks << getTrack(query.value(0).toInt());
        }
    }
    return tracks;
}

void DatabaseManager::clearHistory()
{
    QSqlQuery query(m_database);
    query.exec("DELETE FROM history");
}

bool DatabaseManager::addTagToTrack(int trackId, const QString &tag)
{
    QSqlQuery tagQuery(m_database);
    tagQuery.prepare("INSERT OR IGNORE INTO tags (name) VALUES (:name)");
    tagQuery.bindValue(":name", tag);
    tagQuery.exec();
    tagQuery.prepare("SELECT id FROM tags WHERE name = :name");
    tagQuery.bindValue(":name", tag);
    if (!tagQuery.exec() || !tagQuery.next()) {
        return false;
    }
    int tagId = tagQuery.value(0).toInt();
    QSqlQuery query(m_database);
    query.prepare("INSERT OR IGNORE INTO track_tags (track_id, tag_id) VALUES (:track_id, :tag_id)");
    query.bindValue(":track_id", trackId);
    query.bindValue(":tag_id", tagId);
    return query.exec();
}

bool DatabaseManager::removeTagFromTrack(int trackId, const QString &tag)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM track_tags WHERE track_id = :track_id "
                  "AND tag_id = (SELECT id FROM tags WHERE name = :tag)");
    query.bindValue(":track_id", trackId);
    query.bindValue(":tag", tag);
    return query.exec();
}

QStringList DatabaseManager::getAllTags()
{
    QStringList tags;
    QSqlQuery query(m_database);
    query.exec("SELECT name FROM tags ORDER BY name");
    while (query.next()) {
        tags << query.value(0).toString();
    }
    
    return tags;
}

QList<TrackInfo> DatabaseManager::getTracksByTag(const QString &tag)
{
    QList<TrackInfo> tracks;
    QSqlQuery query(m_database);
    query.prepare("SELECT t.id FROM tracks t "
                  "JOIN track_tags tt ON t.id = tt.track_id "
                  "JOIN tags tag ON tt.tag_id = tag.id "
                  "WHERE tag.name = :tag ORDER BY t.title");
    query.bindValue(":tag", tag);
    if (query.exec()) {
        while (query.next()) {
            tracks << getTrack(query.value(0).toInt());
        }
    }
    
    return tracks;
}

void DatabaseManager::incrementPlayCount(int trackId)
{
    QSqlQuery query(m_database);
    query.prepare("UPDATE tracks SET play_count = play_count + 1 WHERE id = :id");
    query.bindValue(":id", trackId);
    query.exec();
}

void DatabaseManager::updateLastPlayed(int trackId)
{
    QSqlQuery query(m_database);
    query.prepare("UPDATE tracks SET last_played = CURRENT_TIMESTAMP WHERE id = :id");
    query.bindValue(":id", trackId);
    query.exec();
}

int DatabaseManager::addArtist(const QString &name)
{
    if (name.isEmpty()) {
        return -1;
    }
    QSqlQuery query(m_database);
    query.prepare("INSERT OR IGNORE INTO artists (name) VALUES (:name)");
    query.bindValue(":name", name);
    if (!query.exec()) {
        qWarning() << "Ошибка добавления исполнителя:" << query.lastError();
        return -1;
    }
    if (query.numRowsAffected() == 0) {
        query.prepare("SELECT id FROM artists WHERE name = :name");
        query.bindValue(":name", name);
        if (query.exec() && query.next()) {
            return query.value(0).toInt();
        }
        return -1;
    }
    return query.lastInsertId().toInt();
}

int DatabaseManager::getArtistId(const QString &name)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT id FROM artists WHERE name = :name");
    query.bindValue(":name", name);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

bool DatabaseManager::addArtistToTrack(int trackId, int artistId)
{
    QSqlQuery query(m_database);
    query.prepare("INSERT OR IGNORE INTO track_artists (track_id, artist_id) VALUES (:track_id, :artist_id)");
    query.bindValue(":track_id", trackId);
    query.bindValue(":artist_id", artistId);
    return query.exec();
}

bool DatabaseManager::removeArtistFromTrack(int trackId, int artistId)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM track_artists WHERE track_id = :track_id AND artist_id = :artist_id");
    query.bindValue(":track_id", trackId);
    query.bindValue(":artist_id", artistId);
    return query.exec();
}

QStringList DatabaseManager::getAllArtists()
{
    QStringList artists;
    QSqlQuery query(m_database);
    query.exec("SELECT name FROM artists ORDER BY name");
    while (query.next()) {
        artists << query.value(0).toString();
    }
    return artists;
}

QList<TrackInfo> DatabaseManager::getTracksByArtist(int artistId)
{
    QList<TrackInfo> tracks;
    QSqlQuery query(m_database);
    query.prepare("SELECT t.id FROM tracks t "
                  "JOIN track_artists ta ON t.id = ta.track_id "
                  "WHERE ta.artist_id = :artist_id ORDER BY t.title");
    query.bindValue(":artist_id", artistId);
    if (query.exec()) {
        while (query.next()) {
            tracks << getTrack(query.value(0).toInt());
        }
    }
    return tracks;
}

int DatabaseManager::addAlbum(const QString &name)
{
    if (name.isEmpty()) {
        return -1;
    }
    QSqlQuery query(m_database);
    query.prepare("INSERT OR IGNORE INTO albums (name) VALUES (:name)");
    query.bindValue(":name", name);
    if (!query.exec()) {
        qWarning() << "Ошибка добавления альбома:" << query.lastError();
        return -1;
    }
    if (query.numRowsAffected() == 0) {
        query.prepare("SELECT id FROM albums WHERE name = :name");
        query.bindValue(":name", name);
        if (query.exec() && query.next()) {
            return query.value(0).toInt();
        }
        return -1;
    }
    
    return query.lastInsertId().toInt();
}

int DatabaseManager::getAlbumId(const QString &name)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT id FROM albums WHERE name = :name");
    query.bindValue(":name", name);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    
    return -1;
}

bool DatabaseManager::addAlbumToTrack(int trackId, int albumId)
{
    QSqlQuery query(m_database);
    query.prepare("INSERT OR IGNORE INTO track_albums (track_id, album_id) VALUES (:track_id, :album_id)");
    query.bindValue(":track_id", trackId);
    query.bindValue(":album_id", albumId);
    return query.exec();
}

bool DatabaseManager::removeAlbumFromTrack(int trackId, int albumId)
{
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM track_albums WHERE track_id = :track_id AND album_id = :album_id");
    query.bindValue(":track_id", trackId);
    query.bindValue(":album_id", albumId);
    return query.exec();
}

QStringList DatabaseManager::getAllAlbums()
{
    QStringList albums;
    QSqlQuery query(m_database);
    query.exec("SELECT name FROM albums ORDER BY name");
    while (query.next()) {
        albums << query.value(0).toString();
    }
    
    return albums;
}

QList<TrackInfo> DatabaseManager::getTracksByAlbum(int albumId)
{
    QList<TrackInfo> tracks;
    QSqlQuery query(m_database);
    query.prepare("SELECT t.id FROM tracks t "
                  "JOIN track_albums ta ON t.id = ta.track_id "
                  "WHERE ta.album_id = :album_id ORDER BY t.title");
    query.bindValue(":album_id", albumId);
    if (query.exec()) {
        while (query.next()) {
            tracks << getTrack(query.value(0).toInt());
        }
    }
    
    return tracks;
}

QStringList DatabaseManager::getAlbumsByArtist(int artistId)
{
    QStringList albums;
    QSqlQuery query(m_database);
    query.prepare("SELECT DISTINCT a.name FROM albums a "
                  "JOIN track_albums ta ON a.id = ta.album_id "
                  "JOIN track_artists tar ON ta.track_id = tar.track_id "
                  "WHERE tar.artist_id = :artist_id ORDER BY a.name");
    query.bindValue(":artist_id", artistId);
    if (query.exec()) {
        while (query.next()) {
            albums << query.value(0).toString();
        }
    }
    
    return albums;
}

