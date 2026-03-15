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

#include "stalledtorrentmanager.h"

#include <QDateTime>

#include "base/bittorrent/session.h"
#include "base/bittorrent/torrent.h"
#include "base/global.h"
#include "base/logger.h"
#include "base/preferences.h"

using namespace std::chrono_literals;

StalledTorrentManager::StalledTorrentManager(BitTorrent::Session *session, QObject *parent)
    : QObject(parent)
    , m_session(session)
{
    connect(&m_checkTimer, &QTimer::timeout, this, &StalledTorrentManager::checkStalledTorrents);
    m_checkTimer.start(60s); // check every minute
}

void StalledTorrentManager::checkStalledTorrents()
{
    const Preferences *pref = Preferences::instance();
    if (!pref->isStalledManagementEnabled())
        return;

    const int timeoutMinutes = pref->stalledTimeoutMinutes();
    if (timeoutMinutes <= 0)
        return;

    const auto stalledAction = static_cast<StalledAction>(pref->stalledAction());
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const qint64 timeoutSecs = static_cast<qint64>(timeoutMinutes) * 60;

    QList<BitTorrent::TorrentID> toRemove;

    for (BitTorrent::Torrent *torrent : m_session->torrents())
    {
        if (torrent->isFinished() || torrent->isStopped() || torrent->isErrored())
        {
            m_stalledTimestamps.remove(torrent->id());
            continue;
        }

        const bool isStalled = (torrent->downloadPayloadRate() == 0) && (torrent->progress() < 1.0);

        if (!isStalled)
        {
            m_stalledTimestamps.remove(torrent->id());
            continue;
        }

        if (!m_stalledTimestamps.contains(torrent->id()))
        {
            m_stalledTimestamps[torrent->id()] = now;
            continue;
        }

        const qint64 stalledSince = m_stalledTimestamps[torrent->id()];
        if ((now - stalledSince) < timeoutSecs)
            continue;

        // Torrent has been stalled long enough - take action
        LogMsg(tr("Torrent stalled for %1 minutes: \"%2\"").arg(timeoutMinutes).arg(torrent->name()), Log::WARNING);
        emit torrentStalled(torrent);

        switch (stalledAction)
        {
        case StalledAction::Reannounce:
            LogMsg(tr("Reannouncing stalled torrent: \"%1\"").arg(torrent->name()));
            torrent->forceReannounce();
            m_stalledTimestamps[torrent->id()] = now; // reset timer
            break;

        case StalledAction::Stop:
            LogMsg(tr("Stopping stalled torrent: \"%1\"").arg(torrent->name()));
            torrent->stop();
            m_stalledTimestamps.remove(torrent->id());
            break;

        case StalledAction::Remove:
            LogMsg(tr("Removing stalled torrent: \"%1\"").arg(torrent->name()), Log::WARNING);
            toRemove.append(torrent->id());
            m_stalledTimestamps.remove(torrent->id());
            break;

        case StalledAction::MoveCategory:
        {
            const QString category = pref->stalledCategory();
            if (!category.isEmpty())
            {
                LogMsg(tr("Moving stalled torrent to category '%1': \"%2\"").arg(category, torrent->name()));
                torrent->setCategory(category);
            }
            m_stalledTimestamps.remove(torrent->id());
            break;
        }
        }
    }

    // Remove torrents outside the iteration loop
    for (const BitTorrent::TorrentID &id : toRemove)
        m_session->removeTorrent(id);
}
