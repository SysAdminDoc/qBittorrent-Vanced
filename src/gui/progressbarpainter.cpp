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
#include <QPen>
#include <QStyleOptionViewItem>

#include "base/global.h"
#include "uithememanager.h"

namespace
{
    struct ProgressBarThemeColors
    {
        QColor selectedBackground;
        QColor hoverBackground;
        QColor groove;
        QColor text;
        QColor textDisabled;
        QColor focus;
        QColor shimmer;
        QColor downloadingLight;
        QColor downloadingBase;
        QColor downloadingDark;
        QColor downloadingShadow;
        QColor completedLight;
        QColor completedBase;
        QColor completedDark;
        QColor completedShadow;
        QColor stoppedLight;
        QColor stoppedBase;
        QColor stoppedDark;
        QColor stoppedShadow;
    };

    QColor themeColor(const QString &id, const QColor &fallback)
    {
        const UIThemeManager *themeManager = UIThemeManager::instance();
        if (!themeManager)
            return fallback;

        const QColor color = themeManager->getColor(id);
        return color.isValid() ? color : fallback;
    }

    ProgressBarThemeColors progressBarThemeColors()
    {
        return {
            themeColor(u"ProgressBar.SelectedBackground"_s, QColor(0x89, 0xb4, 0xfa, 30)),
            themeColor(u"ProgressBar.HoverBackground"_s, QColor(0x31, 0x32, 0x44, 100)),
            themeColor(u"ProgressBar.Groove"_s, QColor(0x11, 0x11, 0x1b)),
            themeColor(u"ProgressBar.Text"_s, QColor(0xcd, 0xd6, 0xf4)),
            themeColor(u"ProgressBar.TextDisabled"_s, QColor(0x6c, 0x70, 0x86)),
            themeColor(u"ProgressBar.Focus"_s, QColor(0x89, 0xb4, 0xfa)),
            themeColor(u"ProgressBar.Shimmer"_s, QColor(0xcd, 0xd6, 0xf4)),
            themeColor(u"ProgressBar.Downloading.Light"_s, QColor(0x7c, 0xd8, 0x76)),
            themeColor(u"ProgressBar.Downloading.Base"_s, QColor(0x60, 0xc4, 0x5a)),
            themeColor(u"ProgressBar.Downloading.Dark"_s, QColor(0x48, 0xb0, 0x42)),
            themeColor(u"ProgressBar.Downloading.Shadow"_s, QColor(0x40, 0xa8, 0x3a)),
            themeColor(u"ProgressBar.Completed.Light"_s, QColor(0xb8, 0xf0, 0xb0)),
            themeColor(u"ProgressBar.Completed.Base"_s, QColor(0xa6, 0xe3, 0xa1)),
            themeColor(u"ProgressBar.Completed.Dark"_s, QColor(0x80, 0xd0, 0x7a)),
            themeColor(u"ProgressBar.Completed.Shadow"_s, QColor(0x7c, 0xc4, 0x78)),
            themeColor(u"ProgressBar.Stopped.Light"_s, QColor(0x6c, 0x70, 0x86)),
            themeColor(u"ProgressBar.Stopped.Base"_s, QColor(0x58, 0x5b, 0x70)),
            themeColor(u"ProgressBar.Stopped.Dark"_s, QColor(0x45, 0x47, 0x5a)),
            themeColor(u"ProgressBar.Stopped.Shadow"_s, QColor(0x58, 0x5b, 0x70))
        };
    }

    QColor alphaColor(QColor color, const int alpha)
    {
        color.setAlpha(alpha);
        return color;
    }

    void applyStateGradient(QLinearGradient &gradient, const QColor &light, const QColor &base
            , const QColor &dark, const QColor &shadow)
    {
        gradient.setColorAt(0.0, light);
        gradient.setColorAt(0.35, base);
        gradient.setColorAt(0.5, dark);
        gradient.setColorAt(0.65, base);
        gradient.setColorAt(1.0, shadow);
    }

    void drawFocusRing(QPainter *painter, const QRectF &rect, const qreal radius, const QColor &color)
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawRoundedRect(rect.adjusted(1, 1, -1, -1), radius, radius);
        painter->restore();
    }
}

ProgressBarPainter::ProgressBarPainter()
{
    m_shimmerTimer.start();
}

void ProgressBarPainter::setSimpleMode(const bool simple)
{
    m_simpleMode = simple;
}

void ProgressBarPainter::paint(QPainter *painter, const QStyleOptionViewItem &option, const QString &text, const int progress) const
{
    if (m_simpleMode)
        paintSimple(painter, option, text, progress);
    else
        paintFancy(painter, option, text, progress);
}

void ProgressBarPainter::paintSimple(QPainter *painter, const QStyleOptionViewItem &option, const QString &text, const int progress) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRect r = option.rect;
    const bool isEnabled = option.state.testFlag(QStyle::State_Enabled);
    const bool isSelected = option.state.testFlag(QStyle::State_Selected);
    const bool hasFocus = option.state.testFlag(QStyle::State_HasFocus);
    const qreal radius = 3.0;
    const ProgressBarThemeColors colors = progressBarThemeColors();

    if (isSelected)
        painter->fillRect(r, colors.selectedBackground);
    else if (option.state.testFlag(QStyle::State_MouseOver))
        painter->fillRect(r, colors.hoverBackground);

    const int vPad = 4;
    const int hPad = 4;
    const QRectF grooveRect(r.x() + hPad, r.y() + vPad, r.width() - (hPad * 2), r.height() - (vPad * 2));

    QPainterPath groovePath;
    groovePath.addRoundedRect(grooveRect, radius, radius);
    painter->fillPath(groovePath, colors.groove);

    const int clampedProgress = qBound(0, progress, 100);
    if (clampedProgress > 0)
    {
        const qreal chunkWidth = grooveRect.width() * clampedProgress / 100.0;
        const QRectF chunkRect(grooveRect.x(), grooveRect.y(), chunkWidth, grooveRect.height());

        QColor fillColor;
        if (!isEnabled)
            fillColor = colors.stoppedBase;
        else if (clampedProgress >= 100)
            fillColor = colors.completedBase;
        else
            fillColor = colors.downloadingBase;

        painter->setClipPath(groovePath);
        QPainterPath chunkPath;
        chunkPath.addRoundedRect(chunkRect, radius, radius);
        painter->fillPath(chunkPath, fillColor);
        painter->setClipping(false);
    }

    // Always show percent text in simple mode
    const QString displayText = text.isEmpty()
        ? QStringLiteral("%1%").arg(clampedProgress)
        : text;

    const int fontSize = qMax(6, static_cast<int>(grooveRect.height() - 2));
    QFont font = painter->font();
    font.setPixelSize(fontSize);
    painter->setFont(font);

    const QColor textColor = isEnabled ? colors.text : colors.textDisabled;
    painter->setPen(textColor);
    painter->drawText(grooveRect, Qt::AlignCenter, displayText);

    if (hasFocus)
        drawFocusRing(painter, QRectF(r).adjusted(2, 2, -2, -2), 4.0, colors.focus);

    painter->restore();
}

void ProgressBarPainter::paintFancy(QPainter *painter, const QStyleOptionViewItem &option, const QString &text, const int progress) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRect r = option.rect;
    const bool isEnabled = option.state.testFlag(QStyle::State_Enabled);
    const bool isSelected = option.state.testFlag(QStyle::State_Selected);
    const bool hasFocus = option.state.testFlag(QStyle::State_HasFocus);
    const qreal radius = 3.0;
    const ProgressBarThemeColors colors = progressBarThemeColors();

    // Row background
    if (isSelected)
        painter->fillRect(r, colors.selectedBackground);
    else if (option.state.testFlag(QStyle::State_MouseOver))
        painter->fillRect(r, colors.hoverBackground);

    // Groove
    const int vPad = 4;
    const int hPad = 4;
    const QRectF grooveRect(r.x() + hPad, r.y() + vPad, r.width() - (hPad * 2), r.height() - (vPad * 2));

    QPainterPath groovePath;
    groovePath.addRoundedRect(grooveRect, radius, radius);
    painter->fillPath(groovePath, colors.groove);

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
            applyStateGradient(baseGrad, colors.stoppedLight, colors.stoppedBase
                    , colors.stoppedDark, colors.stoppedShadow);
        }
        else if (clampedProgress >= 100)
        {
            // Complete: bright green with 3D banding
            applyStateGradient(baseGrad, colors.completedLight, colors.completedBase
                    , colors.completedDark, colors.completedShadow);
        }
        else
        {
            // Downloading: green with Windows Explorer-style 3D banding
            applyStateGradient(baseGrad, colors.downloadingLight, colors.downloadingBase
                    , colors.downloadingDark, colors.downloadingShadow);
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
            shimmerGrad.setColorAt(0.0, alphaColor(colors.shimmer, 0));
            shimmerGrad.setColorAt(0.3, alphaColor(colors.shimmer, 50));
            shimmerGrad.setColorAt(0.5, alphaColor(colors.shimmer, 75));
            shimmerGrad.setColorAt(0.7, alphaColor(colors.shimmer, 50));
            shimmerGrad.setColorAt(1.0, alphaColor(colors.shimmer, 0));

            painter->fillPath(chunkPath, shimmerGrad);
        }

        painter->setClipping(false);
    }

    // Text
    if (!text.isEmpty())
    {
        const int fontSize = qMax(6, static_cast<int>(grooveRect.height() - 2));
        QFont font = painter->font();
        font.setPixelSize(fontSize);
        painter->setFont(font);

        const QColor textColor = isEnabled ? colors.text : colors.textDisabled;
        painter->setPen(textColor);
        painter->drawText(grooveRect, Qt::AlignCenter, text);
    }

    if (hasFocus)
        drawFocusRing(painter, QRectF(r).adjusted(2, 2, -2, -2), 4.0, colors.focus);

    painter->restore();
}
