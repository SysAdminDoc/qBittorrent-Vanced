/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2024  qBittorrent-Enhanced-Edition contributors
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#pragma once

#include <QHash>
#include <QObject>
#include <QTimer>

#include "base/bittorrent/infohash.h"

namespace BitTorrent
{
    class Session;
    class Torrent;
}

enum class StalledAction
{
    Reannounce = 0,
    Stop = 1,
    Remove = 2,
    MoveCategory = 3
};

class StalledTorrentManager final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(StalledTorrentManager)

public:
    explicit StalledTorrentManager(BitTorrent::Session *session, QObject *parent = nullptr);

signals:
    void torrentStalled(BitTorrent::Torrent *torrent);

private:
    void checkStalledTorrents();

    BitTorrent::Session *m_session = nullptr;
    QTimer m_checkTimer;
    QHash<BitTorrent::TorrentID, qint64> m_stalledTimestamps;
};
