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

#include "torrentcardswidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QVBoxLayout>

#include "base/bittorrent/session.h"
#include "base/bittorrent/torrent.h"
#include "base/global.h"
#include "flowlayout.h"

using namespace std::chrono_literals;

// --- TorrentCard ---

TorrentCard::TorrentCard(BitTorrent::Torrent *torrent, QWidget *parent)
    : QWidget(parent)
    , m_torrent(torrent)
{
    setFixedSize(320, 140);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
}

void TorrentCard::refresh()
{
    update();
}

QColor TorrentCard::stateColor() const
{
    if (m_torrent->isErrored()) return QColor(0xf3, 0x8b, 0xa8); // red
    if (m_torrent->isStopped()) return QColor(0x6c, 0x70, 0x86);  // overlay0
    if (m_torrent->isDownloading()) return QColor(0x89, 0xb4, 0xfa); // blue
    if (m_torrent->isUploading()) return QColor(0xa6, 0xe3, 0xa1);  // green
    return QColor(0xa6, 0xad, 0xc8); // subtext0
}

QString TorrentCard::stateText() const
{
    if (m_torrent->isErrored()) return tr("Error");
    if (m_torrent->isStopped()) return tr("Stopped");
    if (m_torrent->isDownloading()) return tr("Downloading");
    if (m_torrent->isUploading()) return tr("Seeding");
    if (m_torrent->isFinished()) return tr("Completed");
    return tr("Unknown");
}

void TorrentCard::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRect r = rect().adjusted(2, 2, -2, -2);
    const QColor bgColor = m_hovered ? QColor(0x31, 0x32, 0x44) : QColor(0x1e, 0x1e, 0x2e);
    const QColor borderColor = m_selected ? QColor(0x89, 0xb4, 0xfa) : QColor(0x31, 0x32, 0x44);
    const QColor textColor(0xcd, 0xd6, 0xf4);
    const QColor dimColor(0xa6, 0xad, 0xc8);
    const QColor state = stateColor();

    // Card body
    QPainterPath path;
    path.addRoundedRect(QRectF(r), 10, 10);
    p.fillPath(path, bgColor);
    p.setPen(QPen(borderColor, m_selected ? 2 : 1));
    p.drawPath(path);

    // State indicator dot
    p.setPen(Qt::NoPen);
    p.setBrush(state);
    p.drawEllipse(QPointF(r.left() + 16, r.top() + 20), 5, 5);

    // Torrent name (truncated)
    QFont nameFont = p.font();
    nameFont.setPixelSize(13);
    nameFont.setBold(true);
    p.setFont(nameFont);
    p.setPen(textColor);
    const QRect nameRect(r.left() + 28, r.top() + 10, r.width() - 36, 22);
    const QString elidedName = p.fontMetrics().elidedText(m_torrent->name(), Qt::ElideRight, nameRect.width());
    p.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elidedName);

    // State text + category
    QFont smallFont = p.font();
    smallFont.setPixelSize(10);
    smallFont.setBold(false);
    p.setFont(smallFont);
    p.setPen(state);
    QString info = stateText();
    if (!m_torrent->category().isEmpty())
        info += u" | "_s + m_torrent->category();
    p.drawText(r.adjusted(28, 34, -8, 0), Qt::AlignLeft | Qt::AlignTop, info);

    // Progress bar
    const int barY = r.top() + 58;
    const int barH = 8;
    const QRect barBg(r.left() + 12, barY, r.width() - 24, barH);
    QPainterPath barBgPath;
    barBgPath.addRoundedRect(QRectF(barBg), 4, 4);
    p.fillPath(barBgPath, QColor(0x31, 0x32, 0x44));

    const double progress = m_torrent->progress();
    if (progress > 0)
    {
        const QRect barFg(barBg.left(), barBg.top(), static_cast<int>(barBg.width() * progress), barH);
        QPainterPath barFgPath;
        barFgPath.addRoundedRect(QRectF(barFg), 4, 4);
        p.fillPath(barFgPath, state);
    }

    // Progress percent
    p.setPen(textColor);
    p.drawText(r.adjusted(12, barY + barH + 4, -12, 0), Qt::AlignLeft | Qt::AlignTop,
        QStringLiteral("%1%").arg(progress * 100.0, 0, 'f', 1));

    // Size
    auto fmtSize = [](qint64 bytes) -> QString {
        const QStringList units = {u"B"_s, u"KiB"_s, u"MiB"_s, u"GiB"_s, u"TiB"_s};
        double val = bytes;
        int u = 0;
        while (val >= 1024.0 && u < units.size() - 1) { val /= 1024.0; ++u; }
        return QStringLiteral("%1 %2").arg(val, 0, 'f', u > 0 ? 1 : 0).arg(units[u]);
    };

    p.setPen(dimColor);
    p.drawText(r.adjusted(12, barY + barH + 4, -12, 0), Qt::AlignRight | Qt::AlignTop,
        fmtSize(m_torrent->totalSize()));

    // Bottom row: speeds + seeds/peers
    const int bottomY = r.bottom() - 18;
    p.setPen(dimColor);

    if (m_torrent->isDownloading())
    {
        p.setPen(QColor(0x89, 0xb4, 0xfa));
        p.drawText(r.adjusted(12, 0, 0, 0), Qt::AlignLeft | Qt::AlignBottom,
            QStringLiteral("DL: %1/s").arg(fmtSize(m_torrent->downloadPayloadRate())));
    }
    if (m_torrent->isUploading() || m_torrent->uploadPayloadRate() > 0)
    {
        p.setPen(QColor(0xa6, 0xe3, 0xa1));
        p.drawText(r.adjusted(0, 0, -12, 0), Qt::AlignRight | Qt::AlignBottom,
            QStringLiteral("UL: %1/s").arg(fmtSize(m_torrent->uploadPayloadRate())));
    }
}

void TorrentCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_selected = !m_selected;
        emit clicked(m_torrent);
        update();
    }
    QWidget::mousePressEvent(event);
}

void TorrentCard::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit doubleClicked(m_torrent);
    QWidget::mouseDoubleClickEvent(event);
}

void TorrentCard::enterEvent(QEnterEvent *event)
{
    m_hovered = true;
    update();
    QWidget::enterEvent(event);
}

void TorrentCard::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QWidget::leaveEvent(event);
}

// --- TorrentCardsWidget ---

TorrentCardsWidget::TorrentCardsWidget(QWidget *parent)
    : QWidget(parent)
{
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_container = new QWidget(m_scrollArea);
    m_flowLayout = new FlowLayout(m_container, 10, 10, 10);
    m_container->setLayout(m_flowLayout);
    m_scrollArea->setWidget(m_container);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_scrollArea);

    connect(&m_refreshTimer, &QTimer::timeout, this, &TorrentCardsWidget::refresh);
    m_refreshTimer.start(1500ms);
}

void TorrentCardsWidget::refresh()
{
    const auto *session = BitTorrent::Session::instance();
    if (!session) return;

    const int currentCount = session->torrents().size();
    if (currentCount != m_lastTorrentCount)
    {
        rebuildCards();
        m_lastTorrentCount = currentCount;
        return;
    }

    for (auto *card : m_cards)
        card->refresh();
}

void TorrentCardsWidget::rebuildCards()
{
    // Clear existing
    for (auto *card : m_cards)
        card->deleteLater();
    m_cards.clear();

    const auto *session = BitTorrent::Session::instance();
    if (!session) return;

    for (auto *torrent : session->torrents())
    {
        auto *card = new TorrentCard(torrent, m_container);
        connect(card, &TorrentCard::doubleClicked, this, &TorrentCardsWidget::torrentDoubleClicked);
        m_flowLayout->addWidget(card);
        m_cards.append(card);
    }
}
