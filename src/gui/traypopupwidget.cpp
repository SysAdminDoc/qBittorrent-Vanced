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

#include "traypopupwidget.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QSystemTrayIcon>

#include "base/bittorrent/session.h"
#include "base/bittorrent/sessionstatus.h"
#include "base/bittorrent/torrent.h"
#include "base/global.h"

using namespace std::chrono_literals;

TrayPopupWidget::TrayPopupWidget(QWidget *parent)
    : QWidget(parent, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedSize(300, 180);

    connect(&m_refreshTimer, &QTimer::timeout, this, &TrayPopupWidget::refresh);

    m_hideTimer.setSingleShot(true);
    connect(&m_hideTimer, &QTimer::timeout, this, &QWidget::hide);
}

void TrayPopupWidget::showAtTrayIcon()
{
    refresh();

    // Position near the system tray (bottom-right of screen)
    const QScreen *screen = QApplication::primaryScreen();
    if (!screen) return;

    const QRect screenGeometry = screen->availableGeometry();
    const int x = screenGeometry.right() - width() - 8;
    const int y = screenGeometry.bottom() - height() - 8;
    move(x, y);

    show();
    raise();

    m_refreshTimer.start(1s);
    m_hideTimer.start(5s);
}

void TrayPopupWidget::leaveEvent(QEvent *event)
{
    m_hideTimer.start(500ms);
    QWidget::leaveEvent(event);
}

void TrayPopupWidget::refresh()
{
    const auto *session = BitTorrent::Session::instance();
    if (!session) return;

    const auto status = session->status();
    m_downloadRate = status.payloadDownloadRate;
    m_uploadRate = status.payloadUploadRate;

    m_activeTorrents = 0;
    m_downloadingCount = 0;
    m_seedingCount = 0;

    for (const auto *torrent : session->torrents())
    {
        if (torrent->isStopped() || torrent->isErrored()) continue;
        ++m_activeTorrents;
        if (torrent->isDownloading()) ++m_downloadingCount;
        if (torrent->isUploading()) ++m_seedingCount;
    }

    update();
}

void TrayPopupWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRect r = rect().adjusted(6, 6, -6, -6);

    // Background with subtle shadow
    QPainterPath bg;
    bg.addRoundedRect(QRectF(r), 14, 14);
    p.fillPath(bg, QColor(0x1e, 0x1e, 0x2e, 245));
    p.setPen(QPen(QColor(0x45, 0x47, 0x5a, 80), 1));
    p.drawPath(bg);

    const QColor textColor(0xcd, 0xd6, 0xf4);
    const QColor dimColor(0xa6, 0xad, 0xc8);
    const QColor dlColor(0x89, 0xb4, 0xfa);
    const QColor ulColor(0xa6, 0xe3, 0xa1);

    // Title
    QFont titleFont = p.font();
    titleFont.setPixelSize(15);
    titleFont.setWeight(QFont::DemiBold);
    p.setFont(titleFont);
    p.setPen(textColor);
    p.drawText(r.adjusted(20, 16, -20, 0), Qt::AlignLeft | Qt::AlignTop, u"qBittorrent Vanced"_s);

    // Separator
    p.setPen(QPen(QColor(0x31, 0x32, 0x44), 1));
    p.drawLine(r.left() + 20, r.top() + 42, r.right() - 20, r.top() + 42);

    // Data rows
    QFont dataFont = p.font();
    dataFont.setPixelSize(13);
    dataFont.setWeight(QFont::Normal);
    p.setFont(dataFont);

    const int row1Y = r.top() + 56;
    const int row2Y = row1Y + 28;
    const int row3Y = row2Y + 28;

    // Download
    p.setPen(dlColor);
    p.drawText(r.adjusted(20, row1Y - r.top(), -20, 0), Qt::AlignLeft | Qt::AlignTop,
        tr("\xe2\xac\x87  %1").arg(formatSpeed(m_downloadRate)));

    // Upload
    p.setPen(ulColor);
    p.drawText(r.adjusted(20, row2Y - r.top(), -20, 0), Qt::AlignLeft | Qt::AlignTop,
        tr("\xe2\xac\x86  %1").arg(formatSpeed(m_uploadRate)));

    // Active torrents
    p.setPen(dimColor);
    QFont smallFont = dataFont;
    smallFont.setPixelSize(11);
    p.setFont(smallFont);
    p.drawText(r.adjusted(20, row3Y - r.top(), -20, 0), Qt::AlignLeft | Qt::AlignTop,
        tr("%1 active  \xc2\xb7  %2 downloading  \xc2\xb7  %3 seeding").arg(m_activeTorrents).arg(m_downloadingCount).arg(m_seedingCount));
}

QString TrayPopupWidget::formatSpeed(qint64 bytesPerSec) const
{
    return formatSize(bytesPerSec) + u"/s"_s;
}

QString TrayPopupWidget::formatSize(qint64 bytes) const
{
    const QStringList units = {u"B"_s, u"KiB"_s, u"MiB"_s, u"GiB"_s};
    double val = bytes;
    int u = 0;
    while (val >= 1024.0 && u < units.size() - 1) { val /= 1024.0; ++u; }
    return QStringLiteral("%1 %2").arg(val, 0, 'f', u > 0 ? 1 : 0).arg(units[u]);
}
