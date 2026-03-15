/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2020  Prince Gupta <jagannatharjun11@gmail.com>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link this program with the OpenSSL project's "OpenSSL" library (or with
 * modified versions of it that use the same license as the "OpenSSL" library),
 * and distribute the linked executables. You must obey the GNU General Public
 * License in all respects for all of the code used other than "OpenSSL".  If you
 * modify file(s), you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete this
 * exception statement from your version.
 */

#include "progressbarpainter.h"

#include <QElapsedTimer>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QStyleOptionViewItem>

#include "base/global.h"

ProgressBarPainter::ProgressBarPainter()
{
    m_shimmerTimer.start();
}

void ProgressBarPainter::paint(QPainter *painter, const QStyleOptionViewItem &option, const QString &text, const int progress) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRect r = option.rect;
    const bool isEnabled = option.state.testFlag(QStyle::State_Enabled);
    const bool isSelected = option.state.testFlag(QStyle::State_Selected);
    const qreal radius = 3.0;

    // Row background
    if (isSelected)
        painter->fillRect(r, QColor(0x89, 0xb4, 0xfa, 30));
    else if (option.state.testFlag(QStyle::State_MouseOver))
        painter->fillRect(r, QColor(0x31, 0x32, 0x44, 100));

    // Groove
    const int vPad = 4;
    const int hPad = 4;
    const QRectF grooveRect(r.x() + hPad, r.y() + vPad, r.width() - (hPad * 2), r.height() - (vPad * 2));

    QPainterPath groovePath;
    groovePath.addRoundedRect(grooveRect, radius, radius);
    painter->fillPath(groovePath, QColor(0x11, 0x11, 0x1b));

    // Chunk
    const int clampedProgress = qBound(0, progress, 100);
    if (clampedProgress > 0)
    {
        const qreal chunkWidth = grooveRect.width() * clampedProgress / 100.0;
        const QRectF chunkRect(grooveRect.x(), grooveRect.y(), chunkWidth, grooveRect.height());

        // Base gradient (vertical for 3D depth like Windows Explorer)
        QLinearGradient baseGrad(chunkRect.topLeft(), chunkRect.bottomLeft());

        if (!isEnabled)
        {
            // Stopped/error: muted gray
            baseGrad.setColorAt(0.0, QColor(0x6c, 0x70, 0x86));
            baseGrad.setColorAt(0.4, QColor(0x58, 0x5b, 0x70));
            baseGrad.setColorAt(0.6, QColor(0x45, 0x47, 0x5a));
            baseGrad.setColorAt(1.0, QColor(0x58, 0x5b, 0x70));
        }
        else if (clampedProgress >= 100)
        {
            // Complete: bright green with 3D banding
            baseGrad.setColorAt(0.0, QColor(0xb8, 0xf0, 0xb0));
            baseGrad.setColorAt(0.35, QColor(0xa6, 0xe3, 0xa1));
            baseGrad.setColorAt(0.5, QColor(0x80, 0xd0, 0x7a));
            baseGrad.setColorAt(0.65, QColor(0xa6, 0xe3, 0xa1));
            baseGrad.setColorAt(1.0, QColor(0x7c, 0xc4, 0x78));
        }
        else
        {
            // Downloading: green with Windows Explorer-style 3D banding
            baseGrad.setColorAt(0.0, QColor(0x7c, 0xd8, 0x76));
            baseGrad.setColorAt(0.35, QColor(0x60, 0xc4, 0x5a));
            baseGrad.setColorAt(0.5, QColor(0x48, 0xb0, 0x42));
            baseGrad.setColorAt(0.65, QColor(0x60, 0xc4, 0x5a));
            baseGrad.setColorAt(1.0, QColor(0x40, 0xa8, 0x3a));
        }

        // Clip to groove and draw base
        painter->setClipPath(groovePath);

        QPainterPath chunkPath;
        chunkPath.addRoundedRect(chunkRect, radius, radius);
        painter->fillPath(chunkPath, baseGrad);

        // Windows Explorer shimmer effect - a bright highlight that sweeps across
        if (isEnabled && (clampedProgress < 100) && (chunkWidth > 10))
        {
            const qreal shimmerWidth = 60.0;
            const qreal cycleMs = 2000.0;
            const qreal elapsed = static_cast<qreal>(m_shimmerTimer.elapsed());
            const qreal phase = std::fmod(elapsed, cycleMs) / cycleMs;
            // Sweep from left to right across the chunk
            const qreal sweepRange = chunkWidth + shimmerWidth;
            const qreal shimmerX = chunkRect.x() + (phase * sweepRange) - shimmerWidth;

            QLinearGradient shimmerGrad(QPointF(shimmerX, 0), QPointF(shimmerX + shimmerWidth, 0));
            shimmerGrad.setColorAt(0.0, QColor(255, 255, 255, 0));
            shimmerGrad.setColorAt(0.3, QColor(255, 255, 255, 60));
            shimmerGrad.setColorAt(0.5, QColor(255, 255, 255, 90));
            shimmerGrad.setColorAt(0.7, QColor(255, 255, 255, 60));
            shimmerGrad.setColorAt(1.0, QColor(255, 255, 255, 0));

            painter->fillPath(chunkPath, shimmerGrad);
        }

        painter->setClipping(false);
    }

    // Text
    if (!text.isEmpty())
    {
        QFont font = painter->font();
        font.setPixelSize(static_cast<int>(grooveRect.height() - 2));
        painter->setFont(font);

        const QColor textColor = isEnabled ? QColor(0xcd, 0xd6, 0xf4) : QColor(0x6c, 0x70, 0x86);
        painter->setPen(textColor);
        painter->drawText(grooveRect, Qt::AlignCenter, text);
    }

    painter->restore();
}
