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

#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QStackedLayout>

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
    setFixedSize(340, 158);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    refresh();
}

void TorrentCard::refresh()
{
    const QString progressText = QStringLiteral("%1%").arg(m_torrent->progress() * 100.0, 0, 'f', 1);
    setAccessibleName(m_torrent->name());
    setAccessibleDescription(tr("%1. %2 complete.").arg(stateText(), progressText));
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
    const QColor bgColor = m_selected ? QColor(0x18, 0x18, 0x25) : (m_hovered ? QColor(0x31, 0x32, 0x44) : QColor(0x1e, 0x1e, 0x2e));
    const QColor borderColor = (m_selected || hasFocus()) ? QColor(0x89, 0xb4, 0xfa) : QColor(0x31, 0x32, 0x44);
    const QColor textColor(0xcd, 0xd6, 0xf4);
    const QColor dimColor(0xa6, 0xad, 0xc8);
    const QColor state = stateColor();

    // Card body
    QPainterPath path;
    path.addRoundedRect(QRectF(r), 8, 8);
    p.fillPath(path, bgColor);
    p.setPen(QPen(borderColor, (m_selected || hasFocus()) ? 2 : 1));
    p.drawPath(path);

    // State indicator dot
    p.setPen(Qt::NoPen);
    p.setBrush(state);
    p.drawEllipse(QPointF(r.left() + 18, r.top() + 22), 4, 4);

    // Torrent name
    QFont nameFont = p.font();
    nameFont.setPixelSize(13);
    nameFont.setWeight(QFont::DemiBold);
    p.setFont(nameFont);
    p.setPen(textColor);
    const QRect nameRect(r.left() + 32, r.top() + 12, r.width() - 44, 22);
    const QString elidedName = p.fontMetrics().elidedText(m_torrent->name(), Qt::ElideRight, nameRect.width());
    p.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elidedName);

    // State text + category
    QFont smallFont = p.font();
    smallFont.setPixelSize(11);
    smallFont.setWeight(QFont::Normal);
    p.setFont(smallFont);
    p.setPen(state);
    QString info = stateText();
    if (!m_torrent->category().isEmpty())
        info += u"  \xc2\xb7  "_s + m_torrent->category();
    p.drawText(r.adjusted(32, 38, -10, 0), Qt::AlignLeft | Qt::AlignTop, info);

    // Progress bar
    const int barY = r.top() + 68;
    const int barH = 8;
    const QRect barBg(r.left() + 16, barY, r.width() - 32, barH);
    QPainterPath barBgPath;
    barBgPath.addRoundedRect(QRectF(barBg), 4, 4);
    p.fillPath(barBgPath, QColor(0x11, 0x11, 0x1b));

    const double progress = m_torrent->progress();
    if (progress > 0)
    {
        const QRect barFg(barBg.left(), barBg.top(), static_cast<int>(barBg.width() * progress), barH);
        QPainterPath barFgPath;
        barFgPath.addRoundedRect(QRectF(barFg), 4, 4);
        p.fillPath(barFgPath, state);
    }

    // Progress percent + size
    QFont metaFont = p.font();
    metaFont.setPixelSize(11);
    p.setFont(metaFont);
    p.setPen(dimColor);
    p.drawText(r.adjusted(16, barY + barH + 5, -16, 0), Qt::AlignLeft | Qt::AlignTop,
        QStringLiteral("%1%").arg(progress * 100.0, 0, 'f', 1));

    auto fmtSize = [](qint64 bytes) -> QString {
        const QStringList units = {u"B"_s, u"KiB"_s, u"MiB"_s, u"GiB"_s, u"TiB"_s};
        double val = bytes;
        int u = 0;
        while (val >= 1024.0 && u < units.size() - 1) { val /= 1024.0; ++u; }
        return QStringLiteral("%1 %2").arg(val, 0, 'f', u > 0 ? 1 : 0).arg(units[u]);
    };

    p.drawText(r.adjusted(16, barY + barH + 5, -16, 0), Qt::AlignRight | Qt::AlignTop,
        fmtSize(m_torrent->totalSize()));

    // Bottom row: speeds
    p.setPen(dimColor);

    if (m_torrent->isDownloading())
    {
        p.setPen(QColor(0x89, 0xb4, 0xfa));
        p.drawText(r.adjusted(14, 0, 0, -6), Qt::AlignLeft | Qt::AlignBottom,
            tr("\xe2\xac\x87 %1/s").arg(fmtSize(m_torrent->downloadPayloadRate())));
    }
    if (m_torrent->isUploading() || m_torrent->uploadPayloadRate() > 0)
    {
        p.setPen(QColor(0xa6, 0xe3, 0xa1));
        p.drawText(r.adjusted(0, 0, -14, -6), Qt::AlignRight | Qt::AlignBottom,
            tr("\xe2\xac\x86 %1/s").arg(fmtSize(m_torrent->uploadPayloadRate())));
    }
}

void TorrentCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        setFocus(Qt::MouseFocusReason);
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

void TorrentCard::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Space) || (event->key() == Qt::Key_Select))
    {
        m_selected = !m_selected;
        emit clicked(m_torrent);
        update();
        event->accept();
        return;
    }

    if ((event->key() == Qt::Key_Return) || (event->key() == Qt::Key_Enter))
    {
        emit doubleClicked(m_torrent);
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
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
    m_flowLayout = new FlowLayout(m_container, 12, 12, 12);
    m_container->setLayout(m_flowLayout);
    m_scrollArea->setWidget(m_container);

    m_emptyStateLabel = new QLabel(tr("No torrents yet.\nAdd a torrent file or magnet link to get started."), this);
    m_emptyStateLabel->setObjectName(u"torrentCardsEmptyState"_s);
    m_emptyStateLabel->setAlignment(Qt::AlignCenter);
    m_emptyStateLabel->setWordWrap(true);

    m_stackLayout = new QStackedLayout(this);
    m_stackLayout->setContentsMargins(0, 0, 0, 0);
    m_stackLayout->addWidget(m_scrollArea);
    m_stackLayout->addWidget(m_emptyStateLabel);
    m_stackLayout->setCurrentWidget(m_emptyStateLabel);

    connect(&m_refreshTimer, &QTimer::timeout, this, &TorrentCardsWidget::refresh);
    m_refreshTimer.start(1500ms);
}

void TorrentCardsWidget::refresh()
{
    const auto *session = BitTorrent::Session::instance();
    if (!session)
    {
        m_stackLayout->setCurrentWidget(m_emptyStateLabel);
        return;
    }

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

    m_stackLayout->setCurrentWidget(m_cards.isEmpty() ? static_cast<QWidget *>(m_emptyStateLabel) : static_cast<QWidget *>(m_scrollArea));
}
