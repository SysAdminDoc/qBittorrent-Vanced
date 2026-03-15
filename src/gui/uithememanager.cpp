/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2023-2024  Vladimir Golovnev <glassez@yandex.ru>
 * Copyright (C) 2019, 2021  Prince Gupta <jagannatharjun11@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link this program with the OpenSSL project's "OpenSSL" library (or with
 * modified versions of it that use the same license as the "OpenSSL" library),
 * and distribute the linked executables. You must obey the GNU General Public
 * License in all respects for all of the code used other than "OpenSSL".  If
 * you modify file(s), you may extend this exception to your version of the
 * file(s), but you are not obligated to do so. If you do not wish to do so,
 * delete this exception statement from your version.
 */

#include "uithememanager.h"

#include <QApplication>
#include <QPalette>
#include <QPixmapCache>
#include <QResource>
#include <QStyle>
#include <QStyleHints>

#include "base/global.h"
#include "base/logger.h"
#include "base/path.h"
#include "base/preferences.h"
#include "uithemecommon.h"

namespace
{
    bool isDarkTheme()
    {
        const QPalette palette = qApp->palette();
        const QColor &color = palette.color(QPalette::Active, QPalette::Base);
        return (color.lightness() < 127);
    }
}

UIThemeManager *UIThemeManager::m_instance = nullptr;

void UIThemeManager::freeInstance()
{
    delete m_instance;
    m_instance = nullptr;
}

void UIThemeManager::initInstance()
{
    if (!m_instance)
        m_instance = new UIThemeManager;
}

UIThemeManager::UIThemeManager()
    : m_useCustomTheme {Preferences::instance()->useCustomUITheme()}
#ifdef QBT_HAS_COLORSCHEME_OPTION
    , m_colorSchemeSetting {u"Appearance/ColorScheme"_s}
#endif
#if (defined(Q_OS_UNIX) && !defined(Q_OS_MACOS))
    , m_useSystemIcons {Preferences::instance()->useSystemIcons()}
#endif
{
#ifdef Q_OS_WIN
    if (const QString styleName = Preferences::instance()->getStyle(); styleName.compare(u"system", Qt::CaseInsensitive) != 0)
    {
        if (!QApplication::setStyle(styleName.isEmpty() ? u"Fusion"_s : styleName))
            LogMsg(tr("Set app style failed. Unknown style: \"%1\"").arg(styleName), Log::WARNING);
    }
#endif

#ifdef QBT_HAS_COLORSCHEME_OPTION
    applyColorScheme();
#endif

    // NOTE: Qt::QueuedConnection can be omitted as soon as support for Qt 6.5 is dropped
    connect(QApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, &UIThemeManager::onColorSchemeChanged, Qt::QueuedConnection);

    if (m_useCustomTheme)
    {
        const Path themePath = Preferences::instance()->customUIThemePath();

        if (themePath.hasExtension(u".qbtheme"_s))
        {
            if (QResource::registerResource(themePath.data(), u"/uitheme"_s))
                m_themeSource = std::make_unique<QRCThemeSource>();
            else
                LogMsg(tr("Failed to load UI theme from file: \"%1\"").arg(themePath.toString()), Log::WARNING);
        }
        else if (themePath.filename() == CONFIG_FILE_NAME)
        {
            m_themeSource = std::make_unique<FolderThemeSource>(themePath.parentPath());
        }
    }

    if (!m_themeSource)
        m_themeSource = std::make_unique<DefaultThemeSource>();

    if (m_useCustomTheme)
    {
        applyPalette();
        applyStyleSheet();
    }
    else
    {
        applyBuiltInDarkTheme();
    }
}

UIThemeManager *UIThemeManager::instance()
{
    return m_instance;
}

#ifdef QBT_HAS_COLORSCHEME_OPTION
ColorScheme UIThemeManager::colorScheme() const
{
    return m_colorSchemeSetting.get(ColorScheme::System);
}

void UIThemeManager::setColorScheme(const ColorScheme value)
{
    if (value == colorScheme())
        return;

    m_colorSchemeSetting = value;
}

void UIThemeManager::applyColorScheme() const
{
    switch (colorScheme())
    {
    case ColorScheme::System:
    default:
        qApp->styleHints()->unsetColorScheme();
        break;
    case ColorScheme::Light:
        qApp->styleHints()->setColorScheme(Qt::ColorScheme::Light);
        break;
    case ColorScheme::Dark:
        qApp->styleHints()->setColorScheme(Qt::ColorScheme::Dark);
        break;
    }
}
#endif

void UIThemeManager::applyStyleSheet() const
{
    qApp->setStyleSheet(QString::fromUtf8(m_themeSource->readStyleSheet()));
}

void UIThemeManager::onColorSchemeChanged()
{
    emit themeChanged();

    // workaround to refresh styled controls once color scheme is changed
    QApplication::setStyle(QApplication::style()->name());
}

QIcon UIThemeManager::getIcon(const QString &iconId, [[maybe_unused]] const QString &fallback) const
{
    const auto colorMode = isDarkTheme() ? ColorMode::Dark : ColorMode::Light;
    auto &icons = (colorMode == ColorMode::Dark) ? m_darkModeIcons : m_icons;

    const auto iter = icons.find(iconId);
    if (iter != icons.end())
        return *iter;

#if (defined(Q_OS_UNIX) && !defined(Q_OS_MACOS))
    // Don't cache system icons because users might change them at run time
    if (m_useSystemIcons)
    {
        auto icon = QIcon::fromTheme(iconId);
        if (icon.isNull() || icon.availableSizes().isEmpty())
            icon = QIcon::fromTheme(fallback, QIcon(m_themeSource->getIconPath(iconId, colorMode).data()));
        return icon;
    }
#endif

    const QIcon icon {m_themeSource->getIconPath(iconId, colorMode).data()};
    icons[iconId] = icon;
    return icon;
}

QIcon UIThemeManager::getFlagIcon(const QString &countryIsoCode) const
{
    if (countryIsoCode.isEmpty())
        return {};

    const QString key = countryIsoCode.toLower();
    const auto iter = m_flags.find(key);
    if (iter != m_flags.end())
        return *iter;

    const QIcon icon {u":/icons/flags/" + key + u".svg"};
    m_flags[key] = icon;
    return icon;
}

QPixmap UIThemeManager::getScaledPixmap(const QString &iconId, const int height) const
{
    // (workaround) svg images require the use of `QIcon()` to load and scale losslessly,
    // otherwise other image classes will convert it to pixmap first and follow-up scaling will become lossy.

    Q_ASSERT(height > 0);

    const QString cacheKey = iconId + u'@' + QString::number(height);

    QPixmap pixmap;
    if (!QPixmapCache::find(cacheKey, &pixmap))
    {
        pixmap = getIcon(iconId).pixmap(height);
        QPixmapCache::insert(cacheKey, pixmap);
    }

    return pixmap;
}

QColor UIThemeManager::getColor(const QString &id) const
{
    const QColor color = m_themeSource->getColor(id, (isDarkTheme() ? ColorMode::Dark : ColorMode::Light));
    return color;
}

void UIThemeManager::applyBuiltInDarkTheme() const
{
    // Catppuccin Mocha palette
    // https://github.com/catppuccin/catppuccin
    const QColor base(0x1e, 0x1e, 0x2e);       // #1e1e2e
    const QColor mantle(0x18, 0x18, 0x25);      // #181825
    const QColor crust(0x11, 0x11, 0x1b);       // #11111b
    const QColor surface0(0x31, 0x32, 0x44);    // #313244
    const QColor surface1(0x45, 0x47, 0x5a);    // #45475a
    const QColor surface2(0x58, 0x5b, 0x70);    // #585b70
    const QColor overlay0(0x6c, 0x70, 0x86);    // #6c7086
    const QColor overlay1(0x7f, 0x84, 0x9c);    // #7f849c
    const QColor text(0xcd, 0xd6, 0xf4);        // #cdd6f4
    const QColor subtext1(0xba, 0xc2, 0xde);    // #bac2de
    const QColor subtext0(0xa6, 0xad, 0xc8);    // #a6adc8
    const QColor blue(0x89, 0xb4, 0xfa);        // #89b4fa
    const QColor lavender(0xb4, 0xbe, 0xfe);    // #b4befe
    const QColor sapphire(0x74, 0xc7, 0xec);    // #74c7ec
    const QColor mauve(0xcb, 0xa6, 0xf7);       // #cba6f7
    const QColor red(0xf3, 0x8b, 0xa8);         // #f38ba8

    // Ensure Fusion style for consistent cross-platform theming
    QApplication::setStyle(u"Fusion"_s);

    QPalette palette;
    palette.setColor(QPalette::Window, base);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, mantle);
    palette.setColor(QPalette::AlternateBase, surface0);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::ToolTipBase, surface0);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::BrightText, red);
    palette.setColor(QPalette::Highlight, blue);
    palette.setColor(QPalette::HighlightedText, crust);
    palette.setColor(QPalette::Button, surface0);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::Link, sapphire);
    palette.setColor(QPalette::LinkVisited, mauve);
    palette.setColor(QPalette::Light, surface1);
    palette.setColor(QPalette::Midlight, surface0);
    palette.setColor(QPalette::Mid, surface1);
    palette.setColor(QPalette::Dark, crust);
    palette.setColor(QPalette::Shadow, crust);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, overlay0);
    palette.setColor(QPalette::Disabled, QPalette::Text, overlay0);
    palette.setColor(QPalette::Disabled, QPalette::ToolTipText, overlay0);
    palette.setColor(QPalette::Disabled, QPalette::BrightText, overlay0);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, overlay0);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, overlay0);
    palette.setColor(QPalette::PlaceholderText, overlay1);

    qApp->setPalette(palette);

    const QString qss = uR"(/* ---- Global Reset ---- */
        QWidget {
            color: #cdd6f4;
        }
        QMainWindow, QDialog, QFrame, QStackedWidget, QScrollArea,
        QAbstractScrollArea {
            background-color: #1e1e2e;
        }
        QMainWindow::separator {
            background-color: #313244;
            width: 1px;
            height: 1px;
        }
        QToolTip {
            background-color: #313244;
            color: #cdd6f4;
            border: 1px solid #45475a;
            border-radius: 4px;
            padding: 4px 8px;
        }

        /* ---- Menu Bar ---- */
        QMenuBar {
            background-color: #11111b;
            color: #bac2de;
            border: none;
            padding: 0;
        }
        QMenuBar::item {
            background: transparent;
            padding: 4px 10px;
            border-radius: 0;
        }
        QMenuBar::item:selected {
            background-color: #313244;
            color: #cdd6f4;
        }
        QMenuBar::item:pressed {
            background-color: #45475a;
        }

        /* ---- Menus ---- */
        QMenu {
            background-color: #1e1e2e;
            color: #cdd6f4;
            border: 1px solid #313244;
            border-radius: 8px;
            padding: 4px 0;
        }
        QMenu::item {
            padding: 6px 32px 6px 12px;
            border-radius: 4px;
            margin: 1px 4px;
        }
        QMenu::item:selected {
            background-color: rgba(137, 180, 250, 0.12);
            color: #89b4fa;
        }
        QMenu::separator {
            height: 1px;
            background-color: #313244;
            margin: 4px 8px;
        }
        QMenu::icon {
            padding-left: 8px;
        }
        QMenu::indicator {
            width: 16px;
            height: 16px;
            margin-left: 6px;
        }

        /* ---- Toolbar ---- */
        QToolBar {
            background-color: #11111b;
            border: none;
            spacing: 1px;
            padding: 2px 4px;
        }
        QToolBar::separator {
            width: 1px;
            background-color: #313244;
            margin: 6px 4px;
        }
        QToolButton {
            background: transparent;
            color: #a6adc8;
            border: none;
            border-radius: 4px;
            padding: 3px 6px;
            margin: 1px;
        }
        QToolButton:hover {
            background-color: #313244;
            color: #cdd6f4;
        }
        QToolButton:pressed {
            background-color: #45475a;
        }
        QToolButton:checked {
            background-color: rgba(137, 180, 250, 0.12);
            color: #89b4fa;
        }

        /* ---- Tab Bar ---- */
        QTabWidget::pane {
            border: 1px solid #313244;
            background-color: #1e1e2e;
            top: -1px;
        }
        QTabBar {
            background: transparent;
        }
        QTabBar::tab {
            background-color: transparent;
            color: #6c7086;
            padding: 5px 12px;
            margin-right: 1px;
            border: none;
            border-bottom: 2px solid transparent;
        }
        QTabBar::tab:hover {
            color: #bac2de;
            background-color: rgba(49, 50, 68, 0.4);
        }
        QTabBar::tab:selected {
            color: #89b4fa;
            border-bottom: 2px solid #89b4fa;
        }

        /* ---- Status Bar ---- */
        QStatusBar {
            background-color: #11111b;
            color: #a6adc8;
            border-top: 1px solid #313244;
            font-size: 12px;
        }
        QStatusBar::item {
            border: none;
        }
        QStatusBar QLabel {
            color: #a6adc8;
            padding: 0 4px;
        }
        QStatusBar QPushButton {
            background: transparent;
            color: #a6adc8;
            border: none;
            padding: 2px 6px;
            font-weight: normal;
            min-height: 0;
        }
        QStatusBar QPushButton:hover {
            color: #cdd6f4;
            background: transparent;
        }

)"_s + uR"(/* ---- Scrollbars ---- */
        QScrollBar:vertical {
            background-color: transparent;
            width: 8px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background-color: #45475a;
            border-radius: 4px;
            min-height: 24px;
            margin: 2px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #585b70;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
            height: 0;
            border: none;
        }
        QScrollBar:horizontal {
            background-color: transparent;
            height: 8px;
            margin: 0;
        }
        QScrollBar::handle:horizontal {
            background-color: #45475a;
            border-radius: 4px;
            min-width: 24px;
            margin: 2px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: #585b70;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background: none;
            width: 0;
            border: none;
        }

        /* ---- Tree / Table / List Views ---- */
        QTreeView, QTableView, QListView {
            background-color: #181825;
            alternate-background-color: #1b1b2c;
            color: #cdd6f4;
            border: none;
            outline: none;
            selection-background-color: rgba(137, 180, 250, 0.12);
            selection-color: #cdd6f4;
        }
        QTreeView::item, QTableView::item, QListView::item {
            padding: 2px 6px;
            border: none;
        }
        QTreeView::item:hover, QTableView::item:hover, QListView::item:hover {
            background-color: rgba(49, 50, 68, 0.4);
        }
        QTreeView::item:selected, QTableView::item:selected, QListView::item:selected {
            background-color: rgba(137, 180, 250, 0.12);
            color: #cdd6f4;
        }
        QTreeView::branch {
            background: transparent;
        }

        /* ---- Header View ---- */
        QHeaderView {
            background-color: #181825;
            border: none;
        }
        QHeaderView::section {
            background-color: #181825;
            color: #6c7086;
            padding: 3px 8px;
            border: none;
            border-right: 1px solid #1e1e2e;
            border-bottom: 1px solid #313244;
            font-weight: 600;
            font-size: 11px;
            text-transform: uppercase;
        }
        QHeaderView::section:hover {
            color: #a6adc8;
        }
        QHeaderView::section:pressed {
            background-color: #1e1e2e;
        }
        QHeaderView::down-arrow {
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 5px solid #6c7086;
            margin-right: 6px;
        }
        QHeaderView::up-arrow {
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-bottom: 5px solid #6c7086;
            margin-right: 6px;
        }

)"_s + uR"(/* ---- Group Box ---- */
        QGroupBox {
            border: 1px solid #313244;
            border-radius: 6px;
            margin-top: 8px;
            padding-top: 16px;
            color: #cdd6f4;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 12px;
            padding: 0 4px;
            color: #89b4fa;
            font-weight: 600;
        }

        /* ---- Push Buttons ---- */
        QPushButton {
            background-color: #313244;
            color: #cdd6f4;
            border: 1px solid #45475a;
            border-radius: 4px;
            padding: 4px 14px;
            font-weight: 600;
            min-height: 16px;
        }
        QPushButton:hover {
            background-color: #45475a;
            border-color: #585b70;
        }
        QPushButton:pressed {
            background-color: #585b70;
        }
        QPushButton:disabled {
            background-color: #1e1e2e;
            color: #585b70;
            border-color: #313244;
        }
        QPushButton:default {
            border-color: #89b4fa;
        }
        QPushButton:flat {
            background: transparent;
            border: none;
        }
        QPushButton:flat:hover {
            background-color: rgba(49, 50, 68, 0.5);
        }

        /* ---- Line Edit / Text Edit / Spin Box ---- */
        QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox {
            background-color: #11111b;
            color: #cdd6f4;
            border: 1px solid #313244;
            border-radius: 4px;
            padding: 4px 8px;
            selection-background-color: rgba(137, 180, 250, 0.25);
            selection-color: #cdd6f4;
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus,
        QSpinBox:focus, QDoubleSpinBox:focus {
            border-color: #89b4fa;
        }
        QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled,
        QSpinBox:disabled, QDoubleSpinBox:disabled {
            background-color: #181825;
            color: #585b70;
        }
        QSpinBox::up-button, QDoubleSpinBox::up-button,
        QSpinBox::down-button, QDoubleSpinBox::down-button {
            background: transparent;
            border: none;
            width: 16px;
        }
        QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-bottom: 5px solid #a6adc8;
        }
        QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 5px solid #a6adc8;
        }

        /* ---- Combo Box ---- */
        QComboBox {
            background-color: #11111b;
            color: #cdd6f4;
            border: 1px solid #313244;
            border-radius: 4px;
            padding: 4px 8px;
            min-height: 18px;
        }
        QComboBox:hover {
            border-color: #45475a;
        }
        QComboBox:focus {
            border-color: #89b4fa;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 5px solid #6c7086;
            margin-right: 6px;
        }
        QComboBox QAbstractItemView {
            background-color: #1e1e2e;
            color: #cdd6f4;
            border: 1px solid #313244;
            selection-background-color: rgba(137, 180, 250, 0.12);
            selection-color: #cdd6f4;
            outline: none;
            padding: 2px;
        }

)"_s + uR"(/* ---- Check Box / Radio Button ---- */
        QCheckBox, QRadioButton {
            color: #cdd6f4;
            spacing: 6px;
        }
        QCheckBox::indicator, QRadioButton::indicator {
            width: 16px;
            height: 16px;
            border: 2px solid #45475a;
            background-color: #11111b;
        }
        QCheckBox::indicator {
            border-radius: 3px;
        }
        QRadioButton::indicator {
            border-radius: 10px;
        }
        QCheckBox::indicator:hover, QRadioButton::indicator:hover {
            border-color: #89b4fa;
        }
        QCheckBox::indicator:checked {
            background-color: #89b4fa;
            border-color: #89b4fa;
            image: none;
        }
        QRadioButton::indicator:checked {
            background-color: #89b4fa;
            border-color: #89b4fa;
        }

        /* ---- Progress Bar ---- */
        QProgressBar {
            background-color: #11111b;
            border: none;
            border-radius: 3px;
            text-align: center;
            color: #bac2de;
            max-height: 16px;
            font-size: 11px;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #89b4fa, stop:1 #74c7ec);
            border-radius: 3px;
        }

        /* ---- Slider ---- */
        QSlider::groove:horizontal {
            height: 4px;
            background-color: #313244;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background-color: #89b4fa;
            width: 14px;
            height: 14px;
            margin: -5px 0;
            border-radius: 7px;
        }
        QSlider::handle:horizontal:hover {
            background-color: #b4befe;
        }

        /* ---- Splitter ---- */
        QSplitter::handle {
            background-color: #11111b;
        }
        QSplitter::handle:horizontal {
            width: 1px;
        }
        QSplitter::handle:vertical {
            height: 1px;
        }
        QSplitter::handle:hover {
            background-color: #89b4fa;
        }

        /* ---- Dialog Buttons ---- */
        QDialogButtonBox QPushButton {
            min-width: 80px;
        }

        /* ---- Dock Widget ---- */
        QDockWidget {
            titlebar-close-icon: none;
            titlebar-normal-icon: none;
            color: #cdd6f4;
        }
        QDockWidget::title {
            background-color: #181825;
            border-bottom: 1px solid #313244;
            padding: 6px;
            text-align: left;
        }

        /* ---- Label ---- */
        QLabel {
            background: transparent;
        }

        /* ---- Properties panel ---- */
        QStackedWidget {
            background-color: #1e1e2e;
        }
        QScrollArea > QWidget > QWidget {
            background-color: #1e1e2e;
        }
    )"_s;

    qApp->setStyleSheet(qss);
}

void UIThemeManager::applyPalette() const
{
    struct ColorDescriptor
    {
        QString id;
        QPalette::ColorRole colorRole;
        QPalette::ColorGroup colorGroup;
    };

    const ColorDescriptor paletteColorDescriptors[] =
    {
        {u"Palette.Window"_s, QPalette::Window, QPalette::Normal},
        {u"Palette.WindowText"_s, QPalette::WindowText, QPalette::Normal},
        {u"Palette.Base"_s, QPalette::Base, QPalette::Normal},
        {u"Palette.AlternateBase"_s, QPalette::AlternateBase, QPalette::Normal},
        {u"Palette.Text"_s, QPalette::Text, QPalette::Normal},
        {u"Palette.ToolTipBase"_s, QPalette::ToolTipBase, QPalette::Normal},
        {u"Palette.ToolTipText"_s, QPalette::ToolTipText, QPalette::Normal},
        {u"Palette.BrightText"_s, QPalette::BrightText, QPalette::Normal},
        {u"Palette.Highlight"_s, QPalette::Highlight, QPalette::Normal},
        {u"Palette.HighlightedText"_s, QPalette::HighlightedText, QPalette::Normal},
        {u"Palette.Button"_s, QPalette::Button, QPalette::Normal},
        {u"Palette.ButtonText"_s, QPalette::ButtonText, QPalette::Normal},
        {u"Palette.Link"_s, QPalette::Link, QPalette::Normal},
        {u"Palette.LinkVisited"_s, QPalette::LinkVisited, QPalette::Normal},
        {u"Palette.Light"_s, QPalette::Light, QPalette::Normal},
        {u"Palette.Midlight"_s, QPalette::Midlight, QPalette::Normal},
        {u"Palette.Mid"_s, QPalette::Mid, QPalette::Normal},
        {u"Palette.Dark"_s, QPalette::Dark, QPalette::Normal},
        {u"Palette.Shadow"_s, QPalette::Shadow, QPalette::Normal},
        {u"Palette.WindowTextDisabled"_s, QPalette::WindowText, QPalette::Disabled},
        {u"Palette.TextDisabled"_s, QPalette::Text, QPalette::Disabled},
        {u"Palette.ToolTipTextDisabled"_s, QPalette::ToolTipText, QPalette::Disabled},
        {u"Palette.BrightTextDisabled"_s, QPalette::BrightText, QPalette::Disabled},
        {u"Palette.HighlightedTextDisabled"_s, QPalette::HighlightedText, QPalette::Disabled},
        {u"Palette.ButtonTextDisabled"_s, QPalette::ButtonText, QPalette::Disabled}
    };

    QPalette palette = qApp->palette();
    for (const ColorDescriptor &colorDescriptor : paletteColorDescriptors)
    {
        // For backward compatibility, the palette color overrides are read from the section of the "light mode" colors
        const QColor newColor = m_themeSource->getColor(colorDescriptor.id, ColorMode::Light);
        if (newColor.isValid())
            palette.setColor(colorDescriptor.colorGroup, colorDescriptor.colorRole, newColor);
    }

    qApp->setPalette(palette);
}
