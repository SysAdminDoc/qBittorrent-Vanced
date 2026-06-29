/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2023-2024  Vladimir Golovnev <glassez@yandex.ru>
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

#include "uithemedialog.h"

#include <array>
#include <cstddef>

#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QFileDialog>
#include <QFontMetrics>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPaintEvent>
#include <QSizePolicy>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QTimer>

#include "base/3rdparty/expected.hpp"
#include "base/global.h"
#include "base/logger.h"
#include "base/path.h"
#include "base/profile.h"
#include "base/utils/fs.h"
#include "base/utils/io.h"
#include "progressbarpainter.h"
#include "uithemecommon.h"
#include "uithememanager.h"
#include "utils.h"

#include "ui_uithemedialog.h"

#define SETTINGS_KEY(name) u"GUI/UIThemeDialog/" name

namespace
{
    Path userConfigPath()
    {
        return specialFolderLocation(SpecialFolder::Config) / Path(u"themes/default"_s);
    }

    Path defaultIconPath(const QString &iconID, [[maybe_unused]] const ColorMode colorMode)
    {
        return Path(u":icons"_s) / Path(iconID + u".svg");
    }

    QString previewText(const char *text)
    {
        return QCoreApplication::translate("BuiltInThemePreviewWidget", text);
    }
}

class BuiltInThemePreviewWidget final : public QWidget
{
    Q_DISABLE_COPY_MOVE(BuiltInThemePreviewWidget)

public:
    explicit BuiltInThemePreviewWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(260);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        auto *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, qOverload<>(&QWidget::update));
        timer->start(125);
    }

    QSize sizeHint() const override
    {
        return {560, 280};
    }

protected:
    void changeEvent(QEvent *event) override
    {
        QWidget::changeEvent(event);

        if ((event->type() == QEvent::PaletteChange) || (event->type() == QEvent::StyleChange))
            update();
    }

    void paintEvent([[maybe_unused]] QPaintEvent *event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QPalette palette = this->palette();
        const QColor windowColor = palette.color(QPalette::Window);
        const QColor baseColor = palette.color(QPalette::Base);
        const QColor alternateColor = palette.color(QPalette::AlternateBase);
        const QColor borderColor = palette.color(QPalette::Mid);
        const QColor textColor = palette.color(QPalette::Text);
        const QColor mutedColor = palette.color(QPalette::PlaceholderText);
        const QColor highlightColor = palette.color(QPalette::Highlight);
        const QColor highlightedTextColor = palette.color(QPalette::HighlightedText);

        painter.fillRect(rect(), windowColor);

        const QRect panelRect = rect().adjusted(1, 1, -1, -1);
        painter.setPen(borderColor);
        painter.setBrush(baseColor);
        painter.drawRoundedRect(panelRect, 6, 6);

        const int margin = 14;
        const int headerHeight = 30;
        const int rowHeight = 36;
        const int gap = 8;
        const QRect contentRect = panelRect.adjusted(margin, margin, -margin, -margin);
        const int nameWidth = contentRect.width() * 38 / 100;
        const int sizeWidth = contentRect.width() * 15 / 100;
        const int progressWidth = contentRect.width() * 28 / 100;
        const int statusWidth = contentRect.width() - nameWidth - sizeWidth - progressWidth;

        const auto drawCellText = [&painter](const QRect &cellRect, const QString &text, const QColor &color, const int alignment = Qt::AlignVCenter | Qt::AlignLeft)
        {
            painter.setPen(color);
            const QString elided = painter.fontMetrics().elidedText(text, Qt::ElideRight, cellRect.width());
            painter.drawText(cellRect, alignment, elided);
        };

        QFont headerFont = font();
        headerFont.setBold(true);
        painter.setFont(headerFont);

        QRect cellRect(contentRect.left(), contentRect.top(), nameWidth, headerHeight);
        drawCellText(cellRect, previewText("Name"), mutedColor);
        cellRect.translate(nameWidth, 0);
        cellRect.setWidth(sizeWidth);
        drawCellText(cellRect, previewText("Size"), mutedColor, Qt::AlignVCenter | Qt::AlignRight);
        cellRect.translate(sizeWidth, 0);
        cellRect.setWidth(progressWidth);
        drawCellText(cellRect, previewText("Progress"), mutedColor, Qt::AlignVCenter | Qt::AlignHCenter);
        cellRect.translate(progressWidth, 0);
        cellRect.setWidth(statusWidth);
        drawCellText(cellRect, previewText("Status"), mutedColor);

        painter.setFont(font());

        struct PreviewRow
        {
            QString name;
            QString size;
            QString status;
            int progress;
            bool selected;
            bool enabled;
            ProgressBarStateGlyph glyph;
        };

        const std::array<PreviewRow, 5> rows =
        {{
            {previewText("Linux ISO Collection"), u"4.2 GiB"_s, previewText("Downloading"), 63, true, true
                    , ProgressBarStateGlyph::None},
            {previewText("Studio Footage"), u"18.7 GiB"_s, previewText("Queued"), 12, false, true
                    , ProgressBarStateGlyph::Queued},
            {previewText("Media Verify"), u"9.1 GiB"_s, previewText("Checking"), 88, false, true
                    , ProgressBarStateGlyph::Checking},
            {previewText("Release Archive"), u"2.8 GiB"_s, previewText("Seeding"), 100, false, true
                    , ProgressBarStateGlyph::None},
            {previewText("Backup Snapshot"), u"7.5 GiB"_s, previewText("Stalled"), 44, false, true
                    , ProgressBarStateGlyph::Stalled}
        }};

        int rowTop = contentRect.top() + headerHeight + gap;
        for (std::size_t row = 0; row < rows.size(); ++row)
        {
            const PreviewRow &sample = rows[row];
            const QRect rowRect(contentRect.left(), rowTop, contentRect.width(), rowHeight);

            if (sample.selected)
            {
                painter.setPen(Qt::NoPen);
                painter.setBrush(highlightColor);
                painter.drawRoundedRect(rowRect, 5, 5);
            }
            else
            {
                painter.fillRect(rowRect, ((row % 2) == 0) ? alternateColor : baseColor);
            }

            const QColor rowTextColor = sample.selected ? highlightedTextColor : (sample.enabled ? textColor : mutedColor);
            QRect textRect = rowRect.adjusted(10, 0, -8, 0);
            textRect.setWidth(nameWidth - 12);
            drawCellText(textRect, sample.name, rowTextColor);

            textRect.translate(nameWidth, 0);
            textRect.setWidth(sizeWidth - 12);
            drawCellText(textRect, sample.size, rowTextColor, Qt::AlignVCenter | Qt::AlignRight);

            QStyleOptionViewItem option;
            option.rect = QRect(rowRect.left() + nameWidth + sizeWidth + 6, rowRect.top() + 4, progressWidth - 12, rowRect.height() - 8);
            option.state = QStyle::State_None;
            if (sample.enabled)
                option.state |= QStyle::State_Enabled;
            if (sample.selected)
                option.state |= QStyle::State_Selected;

            m_progressBarPainter.paint(&painter, option, QString::number(sample.progress) + u'%', sample.progress
                    , sample.glyph);

            textRect = QRect(rowRect.left() + nameWidth + sizeWidth + progressWidth + 10
                    , rowRect.top(), statusWidth - 18, rowRect.height());
            drawCellText(textRect, sample.status, rowTextColor);

            rowTop += rowHeight;
        }
    }

private:
    ProgressBarPainter m_progressBarPainter;
};

class ColorWidget final : public QLabel
{
    Q_DISABLE_COPY_MOVE(ColorWidget)
    Q_DECLARE_TR_FUNCTIONS(ColorWidget)

public:
    explicit ColorWidget(const QColor &currentColor, const QColor &defaultColor, QWidget *parent = nullptr)
        : QLabel(parent)
        , m_defaultColor {defaultColor}
        , m_currentColor {currentColor}
    {
        setObjectName("colorWidget");
        setFrameShape(QFrame::Box);
        setFrameShadow(QFrame::Plain);
        setAlignment(Qt::AlignCenter);

        applyColor(currentColor);
    }

    QColor currentColor() const
    {
        return m_currentColor;
    }

private:
    void mouseDoubleClickEvent([[maybe_unused]] QMouseEvent *event) override
    {
        showColorDialog();
    }

    void contextMenuEvent([[maybe_unused]] QContextMenuEvent *event) override
    {
        QMenu *menu = new QMenu(this);
        menu->setAttribute(Qt::WA_DeleteOnClose);

        menu->addAction(tr("Edit..."), this, &ColorWidget::showColorDialog);
        menu->addAction(tr("Reset"), this, &ColorWidget::resetColor);

        menu->popup(QCursor::pos());
    }

    void setCurrentColor(const QColor &color)
    {
        if (m_currentColor == color)
            return;

        m_currentColor = color;
        applyColor(m_currentColor);
    }

    void resetColor()
    {
        setCurrentColor(m_defaultColor);
    }

    void applyColor(const QColor &color)
    {
        if (color.isValid())
        {
            setStyleSheet(u"#colorWidget { background-color: rgba(%1, %2, %3, %4); }"_s
                    .arg(color.red())
                    .arg(color.green())
                    .arg(color.blue())
                    .arg(color.alphaF()));
            setText({});
        }
        else
        {
            setStyleSheet({});
            setText(tr("System"));
        }
    }

    void showColorDialog()
    {
        auto *dialog = new QColorDialog(m_currentColor, this);
        dialog->setOption(QColorDialog::ShowAlphaChannel, true);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(dialog, &QDialog::accepted, this, [this, dialog]
        {
            setCurrentColor(dialog->currentColor());
        });

        dialog->open();
    }

    const QColor m_defaultColor;
    QColor m_currentColor;
};

class IconWidget final : public QLabel
{
    Q_DISABLE_COPY_MOVE(IconWidget)
    Q_DECLARE_TR_FUNCTIONS(IconWidget)

public:
    explicit IconWidget(const Path &currentPath, const Path &defaultPath, QWidget *parent = nullptr)
        : QLabel(parent)
        , m_defaultPath {defaultPath}
    {
        setObjectName("iconWidget");
        setAlignment(Qt::AlignCenter);

        setCurrentPath(currentPath);
    }

    Path currentPath() const
    {
        return m_currentPath;
    }

private:
    void mouseDoubleClickEvent([[maybe_unused]] QMouseEvent *event) override
    {
        showFileDialog();
    }

    void contextMenuEvent([[maybe_unused]] QContextMenuEvent *event) override
    {
        QMenu *menu = new QMenu(this);
        menu->setAttribute(Qt::WA_DeleteOnClose);

        menu->addAction(tr("Browse..."), this, &IconWidget::showFileDialog);
        menu->addAction(tr("Reset"), this, &IconWidget::resetIcon);

        menu->popup(QCursor::pos());
    }

    void setCurrentPath(const Path &path)
    {
        if (m_currentPath == path)
            return;

        m_currentPath = path;
        showIcon(m_currentPath);
    }

    void resetIcon()
    {
        setCurrentPath(m_defaultPath);
    }

    void showIcon(const Path &iconPath)
    {
        const QIcon icon {iconPath.data()};
        setPixmap(icon.pixmap(Utils::Gui::smallIconSize()));
    }

    void showFileDialog()
    {
        auto *dialog = new QFileDialog(this, tr("Select icon")
                , QDir::homePath(), (tr("Supported image files") + u" (*.svg *.png)"));
        dialog->setFileMode(QFileDialog::ExistingFile);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(dialog, &QDialog::accepted, this, [this, dialog]
        {
            const Path iconPath {dialog->selectedFiles().value(0)};
            setCurrentPath(iconPath);
        });

        dialog->open();
    }

    const Path m_defaultPath;
    Path m_currentPath;
};

UIThemeDialog::UIThemeDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui {new Ui::UIThemeDialog}
    , m_storeDialogSize {SETTINGS_KEY(u"Size"_s)}
    , m_initialBuiltInThemeFlavor {UIThemeManager::instance()->builtInThemeFlavor()}
{
    m_ui->setupUi(this);

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_builtInThemePreview = new BuiltInThemePreviewWidget(this);
    m_ui->builtInThemePreviewLayout->addWidget(m_builtInThemePreview);

    loadBuiltInThemeFlavors();
    loadColors();
    loadIcons();

    if (const QSize dialogSize = m_storeDialogSize; dialogSize.isValid())
        resize(dialogSize);
}

UIThemeDialog::~UIThemeDialog()
{
    m_storeDialogSize = size();
    delete m_ui;
}

void UIThemeDialog::accept()
{
    QDialog::accept();

    bool hasError = false;
    if (!storeColors())
        hasError = true;
    if (!storeIcons())
        hasError = true;

    if (hasError)
    {
        QMessageBox::critical(this, tr("UI Theme Configuration.")
                , tr("The UI Theme changes could not be fully applied. The details can be found in the Log."));
    }
}

void UIThemeDialog::done(const int result)
{
    if (result != QDialog::Accepted)
        restoreBuiltInThemePreview();

    QDialog::done(result);
}

void UIThemeDialog::loadBuiltInThemeFlavors()
{
    m_ui->builtInThemeFlavorComboBox->blockSignals(true);
    m_ui->builtInThemeFlavorComboBox->clear();

    for (const QString &flavor : UIThemeManager::builtInThemeFlavorIds())
        m_ui->builtInThemeFlavorComboBox->addItem(UIThemeManager::builtInThemeFlavorDisplayName(flavor), flavor);

    const int flavorIndex = m_ui->builtInThemeFlavorComboBox->findData(m_initialBuiltInThemeFlavor);
    m_ui->builtInThemeFlavorComboBox->setCurrentIndex((flavorIndex >= 0) ? flavorIndex : 0);
    m_ui->builtInThemeFlavorComboBox->blockSignals(false);

    connect(m_ui->builtInThemeFlavorComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]
    {
        applyBuiltInThemePreview(m_ui->builtInThemeFlavorComboBox->currentData().toString());
    });
}

void UIThemeDialog::applyBuiltInThemePreview(const QString &flavor)
{
    if (flavor.isEmpty())
        return;

    UIThemeManager::instance()->setBuiltInThemeFlavor(flavor);
    m_builtInThemePreview->update();
}

void UIThemeDialog::restoreBuiltInThemePreview()
{
    if (m_initialBuiltInThemeFlavor.isEmpty())
        return;

    UIThemeManager::instance()->setBuiltInThemeFlavor(m_initialBuiltInThemeFlavor);
    m_builtInThemePreview->update();
}

void UIThemeDialog::loadColors()
{
    const QHash<QString, UIThemeColor> defaultColors = defaultUIThemeColors();
    const QList<QString> colorIDs = std::invoke([](auto &&list) { list.sort(); return list; }, defaultColors.keys());
    int row = 2;
    for (const QString &id : colorIDs)
    {
        m_ui->colorsLayout->addWidget(new QLabel(id), row, 0);

        const UIThemeColor &defaultColor = defaultColors.value(id);

        auto *lightColorWidget = new ColorWidget(m_defaultThemeSource.getColor(id, ColorMode::Light), defaultColor.light, this);
        m_lightColorWidgets.insert(id, lightColorWidget);
        m_ui->colorsLayout->addWidget(lightColorWidget, row, 2);

        auto *darkColorWidget = new ColorWidget(m_defaultThemeSource.getColor(id, ColorMode::Dark), defaultColor.dark, this);
        m_darkColorWidgets.insert(id, darkColorWidget);
        m_ui->colorsLayout->addWidget(darkColorWidget, row, 4);

        ++row;
    }
}

void UIThemeDialog::loadIcons()
{
    const QSet<QString> defaultIcons = defaultUIThemeIcons();
    const QList<QString> iconIDs = std::invoke([](auto &&list) { list.sort(); return list; }
            , QList<QString>(defaultIcons.cbegin(), defaultIcons.cend()));
    int row = 2;
    for (const QString &id : iconIDs)
    {
        m_ui->iconsLayout->addWidget(new QLabel(id), row, 0);

        auto *lightIconWidget = new IconWidget(m_defaultThemeSource.getIconPath(id, ColorMode::Light)
                , defaultIconPath(id, ColorMode::Light), this);
        m_lightIconWidgets.insert(id, lightIconWidget);
        m_ui->iconsLayout->addWidget(lightIconWidget, row, 2);

        auto *darkIconWidget = new IconWidget(m_defaultThemeSource.getIconPath(id, ColorMode::Dark)
                , defaultIconPath(id, ColorMode::Dark), this);
        m_darkIconWidgets.insert(id, darkIconWidget);
        m_ui->iconsLayout->addWidget(darkIconWidget, row, 4);

        ++row;
    }
}

bool UIThemeDialog::storeColors()
{
    QJsonObject userConfig;
    userConfig.insert(u"version", 2);
    const auto colorName = [](const QColor &color)
    {
        return color.name((color.alpha() < 255) ? QColor::HexArgb : QColor::HexRgb);
    };

    const QHash<QString, UIThemeColor> defaultColors = defaultUIThemeColors();
    const auto addColorOverrides = [this, &defaultColors, &userConfig, &colorName](const ColorMode colorMode)
    {
        const QHash<QString, ColorWidget *> &colorWidgets = (colorMode == ColorMode::Light)
                ? m_lightColorWidgets : m_darkColorWidgets;

        QJsonObject colors;
        for (auto it = colorWidgets.cbegin(); it != colorWidgets.cend(); ++it)
        {
            const QString &colorID = it.key();
            const QColor &defaultColor = (colorMode == ColorMode::Light)
                    ? defaultColors.value(colorID).light : defaultColors.value(colorID).dark;
            const QColor &color = it.value()->currentColor();
            if (color != defaultColor)
                colors.insert(it.key(), colorName(color));
        }

        if (!colors.isEmpty())
            userConfig.insert(((colorMode == ColorMode::Light) ? KEY_COLORS_LIGHT : KEY_COLORS_DARK), colors);
    };

    addColorOverrides(ColorMode::Light);
    addColorOverrides(ColorMode::Dark);

    const QByteArray configData = QJsonDocument(userConfig).toJson();
    const nonstd::expected<void, QString> result = Utils::IO::saveToFile((userConfigPath() / Path(CONFIG_FILE_NAME)), configData);
    if (!result)
    {
        const QString error = tr("Couldn't save UI Theme configuration. Reason: %1").arg(result.error());
        LogMsg(error, Log::WARNING);
        return false;
    }

    return true;
}

bool UIThemeDialog::storeIcons()
{
    bool hasError = false;

    const auto updateIcons = [this, &hasError](const ColorMode colorMode)
    {
        const QHash<QString, IconWidget *> &iconWidgets = (colorMode == ColorMode::Light)
                ? m_lightIconWidgets : m_darkIconWidgets;
        const Path subdirPath = (colorMode == ColorMode::Light)
                ? Path(u"icons/light"_s) : Path(u"icons/dark"_s);

        for (auto it = iconWidgets.cbegin(); it != iconWidgets.cend(); ++it)
        {
            const QString &id = it.key();
            const Path &path = it.value()->currentPath();
            if (path == m_defaultThemeSource.getIconPath(id, colorMode))
                continue;

            const Path &userIconPathBase = userConfigPath() / subdirPath / Path(id);

            if (const Path oldIconPath = userIconPathBase + u".svg"
                    ; path.exists() && !Utils::Fs::removeFile(oldIconPath))
            {
                const QString error = tr("Couldn't remove icon file. File: %1.").arg(oldIconPath.toString());
                LogMsg(error, Log::WARNING);
                hasError = true;
                continue;
            }

            if (const Path oldIconPath = userIconPathBase + u".png"
                    ; path.exists() && !Utils::Fs::removeFile(oldIconPath))
            {
                const QString error = tr("Couldn't remove icon file. File: %1.").arg(oldIconPath.toString());
                LogMsg(error, Log::WARNING);
                hasError = true;
                continue;
            }

            if (const Path targetPath = userIconPathBase + path.extension()
                    ; !Utils::Fs::copyFile(path, targetPath))
            {
                const QString error = tr("Couldn't copy icon file. Source: %1. Destination: %2.")
                        .arg(path.toString(), targetPath.toString());
                LogMsg(error, Log::WARNING);
                hasError = true;
            }
        }
    };

    updateIcons(ColorMode::Light);
    updateIcons(ColorMode::Dark);

    return !hasError;
}
