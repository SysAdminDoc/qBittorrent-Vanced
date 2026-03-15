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

#include <QScrollArea>
#include <QTimer>
#include <QWidget>

class QFlowLayout;
class QVBoxLayout;

namespace BitTorrent
{
    class Torrent;
}

class TorrentCard final : public QWidget
{
    Q_OBJECT

public:
    explicit TorrentCard(BitTorrent::Torrent *torrent, QWidget *parent = nullptr);
    void refresh();
    BitTorrent::Torrent *torrent() const { return m_torrent; }

signals:
    void clicked(BitTorrent::Torrent *torrent);
    void doubleClicked(BitTorrent::Torrent *torrent);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QColor stateColor() const;
    QString stateText() const;

    BitTorrent::Torrent *m_torrent;
    bool m_hovered = false;
    bool m_selected = false;
};

class TorrentCardsWidget final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TorrentCardsWidget)

public:
    explicit TorrentCardsWidget(QWidget *parent = nullptr);
    void refreshCards() { refresh(); }

signals:
    void torrentDoubleClicked(BitTorrent::Torrent *torrent);

private:
    void refresh();
    void rebuildCards();

    QTimer m_refreshTimer;
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_container = nullptr;
    QLayout *m_flowLayout = nullptr;
    QList<TorrentCard *> m_cards;
    int m_lastTorrentCount = -1;
};
