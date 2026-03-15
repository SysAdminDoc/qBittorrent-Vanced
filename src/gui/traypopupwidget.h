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

#include <QTimer>
#include <QWidget>

class TrayPopupWidget final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TrayPopupWidget)

public:
    explicit TrayPopupWidget(QWidget *parent = nullptr);

    void showAtTrayIcon();

protected:
    void paintEvent(QPaintEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void refresh();
    QString formatSpeed(qint64 bytesPerSec) const;
    QString formatSize(qint64 bytes) const;

    QTimer m_refreshTimer;
    QTimer m_hideTimer;

    qint64 m_downloadRate = 0;
    qint64 m_uploadRate = 0;
    int m_activeTorrents = 0;
    int m_downloadingCount = 0;
    int m_seedingCount = 0;
};
