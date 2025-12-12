#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "databasemanager.h"

class PlaylistModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        TrackIdRole = Qt::UserRole + 1,
        FilePathRole,
        TitleRole,
        ArtistRole,
        AlbumRole,
        DurationRole,
        CoverPathRole,
        TagsRole
    };

    explicit PlaylistModel(QObject *parent = nullptr);
    
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    
    void setTracks(const QList<TrackInfo> &tracks);
    void addTrack(const TrackInfo &track);
    void removeTrack(int index);
    void clear();
    TrackInfo trackAt(int index) const;
    int trackCount() const { return m_tracks.size(); }

private:
    QList<TrackInfo> m_tracks;
};

#endif




