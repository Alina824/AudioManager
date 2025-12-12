#include "playlistmodel.h"
#include <QFileInfo>

PlaylistModel::PlaylistModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PlaylistModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_tracks.size();
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_tracks.size()) {
        return QVariant();
    }
    
    const TrackInfo &track = m_tracks[index.row()];
    
    switch (role) {
    case TrackIdRole:
        return track.id;
    case FilePathRole:
        return track.filePath;
    case TitleRole:
        return track.title.isEmpty() ? QFileInfo(track.filePath).baseName() : track.title;
    case ArtistRole:
        return track.artist;
    case AlbumRole:
        return track.album;
    case DurationRole:
        return track.duration;
    case CoverPathRole:
        return track.coverPath;
    case TagsRole:
        return track.tags.join(", ");
    case Qt::DisplayRole:
        return track.title.isEmpty() ? QFileInfo(track.filePath).baseName() : track.title;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PlaylistModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TrackIdRole] = "trackId";
    roles[FilePathRole] = "filePath";
    roles[TitleRole] = "title";
    roles[ArtistRole] = "artist";
    roles[AlbumRole] = "album";
    roles[DurationRole] = "duration";
    roles[CoverPathRole] = "coverPath";
    roles[TagsRole] = "tags";
    return roles;
}

void PlaylistModel::setTracks(const QList<TrackInfo> &tracks)
{
    beginResetModel();
    m_tracks = tracks;
    endResetModel();
}

void PlaylistModel::addTrack(const TrackInfo &track)
{
    beginInsertRows(QModelIndex(), m_tracks.size(), m_tracks.size());
    m_tracks.append(track);
    endInsertRows();
}

void PlaylistModel::removeTrack(int index)
{
    if (index < 0 || index >= m_tracks.size()) {
        return;
    }
    
    beginRemoveRows(QModelIndex(), index, index);
    m_tracks.removeAt(index);
    endRemoveRows();
}

void PlaylistModel::clear()
{
    beginResetModel();
    m_tracks.clear();
    endResetModel();
}

TrackInfo PlaylistModel::trackAt(int index) const
{
    if (index >= 0 && index < m_tracks.size()) {
        return m_tracks[index];
    }
    TrackInfo empty;
    empty.id = -1;
    return empty;
}

