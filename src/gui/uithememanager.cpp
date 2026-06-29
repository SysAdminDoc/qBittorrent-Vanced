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

#include <array>

#include <QApplication>
#include <QEvent>
#include <QPalette>
#include <QPixmapCache>
#include <QResource>
#include <QStyle>
#include <QStyleHints>
#include <QWidget>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "base/global.h"
#include "base/logger.h"
#include "base/path.h"
#include "base/preferences.h"
#include "uithemecommon.h"

namespace
{
    struct CatppuccinPalette
    {
        QStringView id;
        QStringView name;
        bool isDark;
        QColor base;
        QColor mantle;
        QColor crust;
        QColor surface0;
        QColor surface1;
        QColor surface2;
        QColor overlay0;
        QColor overlay1;
        QColor text;
        QColor subtext1;
        QColor subtext0;
        QColor blue;
        QColor lavender;
        QColor sapphire;
        QColor mauve;
        QColor red;
        QColor green;
    };

    QColor rgb(const int value)
    {
        return QColor::fromRgb((value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff);
    }

    const std::array<CatppuccinPalette, 4> &catppuccinPalettes()
    {
        static const std::array<CatppuccinPalette, 4> palettes =
        {{
            {u"latte", u"Latte", false, rgb(0xeff1f5), rgb(0xe6e9ef), rgb(0xdce0e8), rgb(0xccd0da), rgb(0xbcc0cc), rgb(0xacb0be), rgb(0x9ca0b0), rgb(0x8c8fa1), rgb(0x4c4f69), rgb(0x5c5f77), rgb(0x6c6f85), rgb(0x1e66f5), rgb(0x7287fd), rgb(0x209fb5), rgb(0x8839ef), rgb(0xd20f39), rgb(0x40a02b)},
            {u"frappe", u"Frappe", true, rgb(0x303446), rgb(0x292c3c), rgb(0x232634), rgb(0x414559), rgb(0x51576d), rgb(0x626880), rgb(0x737994), rgb(0x838ba7), rgb(0xc6d0f5), rgb(0xb5bfe2), rgb(0xa5adce), rgb(0x8caaee), rgb(0xbabbf1), rgb(0x85c1dc), rgb(0xca9ee6), rgb(0xe78284), rgb(0xa6d189)},
            {u"macchiato", u"Macchiato", true, rgb(0x24273a), rgb(0x1e2030), rgb(0x181926), rgb(0x363a4f), rgb(0x494d64), rgb(0x5b6078), rgb(0x6e738d), rgb(0x8087a2), rgb(0xcad3f5), rgb(0xb8c0e0), rgb(0xa5adcb), rgb(0x8aadf4), rgb(0xb7bdf8), rgb(0x7dc4e4), rgb(0xc6a0f6), rgb(0xed8796), rgb(0xa6da95)},
            {u"mocha", u"Mocha", true, rgb(0x1e1e2e), rgb(0x181825), rgb(0x11111b), rgb(0x313244), rgb(0x45475a), rgb(0x585b70), rgb(0x6c7086), rgb(0x7f849c), rgb(0xcdd6f4), rgb(0xbac2de), rgb(0xa6adc8), rgb(0x89b4fa), rgb(0xb4befe), rgb(0x74c7ec), rgb(0xcba6f7), rgb(0xf38ba8), rgb(0xa6e3a1)}
        }};
        return palettes;
    }

    const CatppuccinPalette &catppuccinPalette(const QString &id)
    {
        const QString normalized = id.toLower();
        for (const CatppuccinPalette &palette : catppuccinPalettes())
        {
            if (palette.id.toString() == normalized)
                return palette;
        }

        return catppuccinPalettes().back();
    }

    QString normalizedCatppuccinFlavor(const QString &id, const QString &fallback = u"mocha"_s)
    {
        const CatppuccinPalette &palette = id.isEmpty() ? catppuccinPalette(fallback) : catppuccinPalette(id);
        return palette.id.toString();
    }

    QString defaultCatppuccinFlavorForSystem()
    {
        return (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Light) ? u"latte"_s : u"mocha"_s;
    }

    QString cssColor(const QColor &color)
    {
        return color.name(QColor::HexRgb);
    }

    QString rgbaColor(const QColor &color, const QString &alpha)
    {
        return u"rgba(%1, %2, %3, %4)"_s
                .arg(color.red())
                .arg(color.green())
                .arg(color.blue())
                .arg(alpha);
    }

    void replaceColor(QString &qss, const QString &mochaColor, const QColor &replacement)
    {
        qss.replace(mochaColor, cssColor(replacement), Qt::CaseInsensitive);
    }

    void applyCatppuccinPalette(QString &qss, const CatppuccinPalette &palette)
    {
        replaceColor(qss, u"#1e1e2e"_s, palette.base);
        replaceColor(qss, u"#181825"_s, palette.mantle);
        replaceColor(qss, u"#11111b"_s, palette.crust);
        replaceColor(qss, u"#313244"_s, palette.surface0);
        replaceColor(qss, u"#45475a"_s, palette.surface1);
        replaceColor(qss, u"#585b70"_s, palette.surface2);
        replaceColor(qss, u"#6c7086"_s, palette.overlay0);
        replaceColor(qss, u"#7f849c"_s, palette.overlay1);
        replaceColor(qss, u"#cdd6f4"_s, palette.text);
        replaceColor(qss, u"#bac2de"_s, palette.subtext1);
        replaceColor(qss, u"#a6adc8"_s, palette.subtext0);
        replaceColor(qss, u"#89b4fa"_s, palette.blue);
        replaceColor(qss, u"#b4befe"_s, palette.lavender);
        replaceColor(qss, u"#74c7ec"_s, palette.sapphire);
        replaceColor(qss, u"#cba6f7"_s, palette.mauve);
        replaceColor(qss, u"#f38ba8"_s, palette.red);
        replaceColor(qss, u"#a6e3a1"_s, palette.green);

        qss.replace(u"rgba(137, 180, 250, 0.10)"_s, rgbaColor(palette.blue, u"0.10"_s));
        qss.replace(u"rgba(137, 180, 250, 0.12)"_s, rgbaColor(palette.blue, u"0.12"_s));
        qss.replace(u"rgba(137, 180, 250, 0.16)"_s, rgbaColor(palette.blue, u"0.16"_s));
        qss.replace(u"rgba(137, 180, 250, 0.18)"_s, rgbaColor(palette.blue, u"0.18"_s));
        qss.replace(u"rgba(137, 180, 250, 0.25)"_s, rgbaColor(palette.blue, u"0.25"_s));
        qss.replace(u"rgba(24, 24, 37, 0.42)"_s, rgbaColor(palette.mantle, u"0.42"_s));
        qss.replace(u"rgba(49, 50, 68, 0.4)"_s, rgbaColor(palette.surface0, u"0.4"_s));
        qss.replace(u"rgba(49, 50, 68, 0.5)"_s, rgbaColor(palette.surface0, u"0.5"_s));
        qss.replace(u"rgba(49, 50, 68, 0.55)"_s, rgbaColor(palette.surface0, u"0.55"_s));
    }

#ifdef Q_OS_WIN
    COLORREF toColorRef(const QColor &color)
    {
        return RGB(color.red(), color.green(), color.blue());
    }
#endif

    bool isDarkTheme()
    {
        const QPalette palette = qApp->palette();
        const QColor &color = palette.color(QPalette::Active, QPalette::Base);
        return (color.lightness() < 127);
    }

    bool shouldPolishNativeTitleBar(const QWidget *widget)
    {
        return widget && widget->isWindow() && !widget->testAttribute(Qt::WA_DontShowOnScreen);
    }

#ifdef Q_OS_WIN
    using DwmSetWindowAttributeFunc = HRESULT(WINAPI *)(HWND, DWORD, LPCVOID, DWORD);

    DwmSetWindowAttributeFunc resolveDwmSetWindowAttribute()
    {
        static const HMODULE dwmApi = ::LoadLibraryW(L"dwmapi.dll");
        if (!dwmApi)
            return nullptr;

        static const auto dwmSetWindowAttribute = reinterpret_cast<DwmSetWindowAttributeFunc>(
                ::GetProcAddress(dwmApi, "DwmSetWindowAttribute"));
        return dwmSetWindowAttribute;
    }
#endif
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

    m_builtInThemeFlavor = normalizedCatppuccinFlavor(
            Preferences::instance()->builtInUIThemeFlavor(), defaultCatppuccinFlavorForSystem());

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

#ifdef Q_OS_WIN
    qApp->installEventFilter(this);
    for (QWidget *widget : QApplication::topLevelWidgets())
        applyNativeDarkTitleBar(widget);
#endif
}

UIThemeManager *UIThemeManager::instance()
{
    return m_instance;
}

QStringList UIThemeManager::builtInThemeFlavorIds()
{
    QStringList ids;
    for (const CatppuccinPalette &palette : catppuccinPalettes())
        ids.append(palette.id.toString());
    return ids;
}

QString UIThemeManager::builtInThemeFlavorDisplayName(const QString &flavor)
{
    return tr("Catppuccin %1").arg(catppuccinPalette(flavor).name.toString());
}

QString UIThemeManager::builtInThemeFlavor() const
{
    return m_builtInThemeFlavor;
}

void UIThemeManager::setBuiltInThemeFlavor(const QString &flavor)
{
    const QString normalizedFlavor = normalizedCatppuccinFlavor(flavor);
    if (normalizedFlavor == m_builtInThemeFlavor)
        return;

    m_builtInThemeFlavor = normalizedFlavor;
    Preferences::instance()->setBuiltInUIThemeFlavor(normalizedFlavor);

    if (m_useCustomTheme)
        return;

    applyBuiltInDarkTheme();

#ifdef Q_OS_WIN
    for (QWidget *widget : QApplication::topLevelWidgets())
        applyNativeDarkTitleBar(widget);
#endif

    emit themeChanged();
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

bool UIThemeManager::eventFilter(QObject *watched, QEvent *event)
{
#ifdef Q_OS_WIN
    if ((event->type() == QEvent::Show) || (event->type() == QEvent::WinIdChange))
    {
        if (auto *widget = qobject_cast<QWidget *>(watched))
            applyNativeDarkTitleBar(widget);
    }
#endif

    return QObject::eventFilter(watched, event);
}

#ifdef Q_OS_WIN
void UIThemeManager::applyNativeDarkTitleBar(QWidget *widget) const
{
    if (!shouldPolishNativeTitleBar(widget))
        return;

    const auto dwmSetWindowAttribute = resolveDwmSetWindowAttribute();
    if (!dwmSetWindowAttribute)
        return;

    const CatppuccinPalette &palette = catppuccinPalette(m_builtInThemeFlavor);

    const HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    const BOOL darkModeEnabled = palette.isDark ? TRUE : FALSE;
    // 20 is current DWMWA_USE_IMMERSIVE_DARK_MODE; 19 covers older Windows 10 builds.
    (void)dwmSetWindowAttribute(hwnd, 20, &darkModeEnabled, sizeof(darkModeEnabled));
    (void)dwmSetWindowAttribute(hwnd, 19, &darkModeEnabled, sizeof(darkModeEnabled));

    const COLORREF captionColor = toColorRef(palette.crust);
    const COLORREF borderColor = toColorRef(palette.surface0);
    const COLORREF textColor = toColorRef(palette.text);
    (void)dwmSetWindowAttribute(hwnd, 35, &captionColor, sizeof(captionColor));
    (void)dwmSetWindowAttribute(hwnd, 34, &borderColor, sizeof(borderColor));
    (void)dwmSetWindowAttribute(hwnd, 36, &textColor, sizeof(textColor));
}
#endif

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
    const CatppuccinPalette &theme = catppuccinPalette(m_builtInThemeFlavor);

    // Catppuccin palette
    // https://github.com/catppuccin/catppuccin
    const QColor highlightedText = theme.isDark ? theme.crust : theme.base;

    // Ensure Fusion style for consistent cross-platform theming
    QApplication::setStyle(u"Fusion"_s);

    QPalette palette;
    palette.setColor(QPalette::Window, theme.base);
    palette.setColor(QPalette::WindowText, theme.text);
    palette.setColor(QPalette::Base, theme.mantle);
    palette.setColor(QPalette::AlternateBase, theme.surface0);
    palette.setColor(QPalette::Text, theme.text);
    palette.setColor(QPalette::ToolTipBase, theme.surface0);
    palette.setColor(QPalette::ToolTipText, theme.text);
    palette.setColor(QPalette::BrightText, theme.red);
    palette.setColor(QPalette::Highlight, theme.blue);
    palette.setColor(QPalette::HighlightedText, highlightedText);
    palette.setColor(QPalette::Button, theme.surface0);
    palette.setColor(QPalette::ButtonText, theme.text);
    palette.setColor(QPalette::Link, theme.sapphire);
    palette.setColor(QPalette::LinkVisited, theme.mauve);
    palette.setColor(QPalette::Light, theme.surface1);
    palette.setColor(QPalette::Midlight, theme.surface0);
    palette.setColor(QPalette::Mid, theme.surface1);
    palette.setColor(QPalette::Dark, theme.crust);
    palette.setColor(QPalette::Shadow, theme.crust);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, theme.overlay0);
    palette.setColor(QPalette::Disabled, QPalette::Text, theme.overlay0);
    palette.setColor(QPalette::Disabled, QPalette::ToolTipText, theme.overlay0);
    palette.setColor(QPalette::Disabled, QPalette::BrightText, theme.overlay0);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, theme.overlay0);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, theme.overlay0);
    palette.setColor(QPalette::Disabled, QPalette::Base, theme.crust);
    palette.setColor(QPalette::Disabled, QPalette::Button, theme.mantle);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, theme.surface1);
    palette.setColor(QPalette::PlaceholderText, theme.overlay1);

    qApp->setPalette(palette);

    QString qss = uR"(/* ---- Global Reset ---- */
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
            padding: 6px 8px;
        }
        QWidget:focus {
            outline: none;
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
        QMenu::item:disabled {
            color: #6c7086;
            background-color: transparent;
        }
        QMenu::separator {
            height: 1px;
            background-color: #313244;
            margin: 5px 10px;
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
            border-bottom: 1px solid #313244;
            spacing: 3px;
            padding: 4px 6px;
        }
        QToolBar::separator {
            width: 1px;
            background-color: #313244;
            margin: 6px 5px;
        }
        QToolButton {
            background: transparent;
            color: #a6adc8;
            border: none;
            border-radius: 6px;
            padding: 4px 7px;
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
        QToolButton:focus {
            border: 1px solid #89b4fa;
            padding: 3px 6px;
        }
        QToolButton:disabled {
            color: #585b70;
            background: transparent;
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
            padding: 7px 14px;
            margin-right: 2px;
            border: none;
            border-bottom: 2px solid transparent;
            min-height: 20px;
        }
        QTabBar::tab:hover {
            color: #bac2de;
            background-color: rgba(49, 50, 68, 0.4);
        }
        QTabBar::tab:selected {
            color: #89b4fa;
            border-bottom: 2px solid #89b4fa;
        }
        QTabBar::tab:focus {
            color: #cdd6f4;
            background-color: rgba(137, 180, 250, 0.10);
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
            border-radius: 4px;
            padding: 3px 7px;
            font-weight: normal;
            min-height: 0;
        }
        QStatusBar QPushButton:hover {
            color: #cdd6f4;
            background: rgba(49, 50, 68, 0.55);
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
            alternate-background-color: #1e1e2e;
            color: #cdd6f4;
            border: none;
            gridline-color: #313244;
            outline: none;
            selection-background-color: rgba(137, 180, 250, 0.12);
            selection-color: #cdd6f4;
        }
        QTreeView::item, QTableView::item, QListView::item {
            padding: 4px 8px;
            border: none;
        }
        QTreeView::item:hover, QTableView::item:hover, QListView::item:hover {
            background-color: rgba(49, 50, 68, 0.4);
        }
        QTreeView::item:selected, QTableView::item:selected, QListView::item:selected {
            background-color: rgba(137, 180, 250, 0.18);
            color: #cdd6f4;
        }
        QTreeView::item:selected:active, QTableView::item:selected:active,
        QListView::item:selected:active {
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
            padding: 5px 8px;
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
            margin-top: 10px;
            padding-top: 18px;
            background-color: rgba(24, 24, 37, 0.42);
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
            border-radius: 6px;
            padding: 5px 14px;
            font-weight: 600;
            min-height: 22px;
        }
        QPushButton:hover {
            background-color: #45475a;
            border-color: #585b70;
        }
        QPushButton:pressed {
            background-color: #585b70;
        }
        QPushButton:focus {
            border-color: #89b4fa;
            background-color: #313244;
        }
        QPushButton:disabled {
            background-color: #1e1e2e;
            color: #585b70;
            border-color: #313244;
        }
        QPushButton:default {
            border-color: #89b4fa;
            background-color: rgba(137, 180, 250, 0.16);
        }
        QPushButton:flat {
            background: transparent;
            border: none;
        }
        QPushButton:flat:hover {
            background-color: rgba(49, 50, 68, 0.5);
        }
        QPushButton#inlineDownloadSpeedButton, QPushButton#inlineUploadSpeedButton {
            background-color: #181825;
            border: 1px solid #313244;
            border-radius: 6px;
            font-size: 11px;
            font-weight: 600;
            min-height: 18px;
            padding: 3px 9px;
        }
        QPushButton#inlineDownloadSpeedButton {
            color: #89b4fa;
        }
        QPushButton#inlineUploadSpeedButton {
            color: #a6e3a1;
        }
        QPushButton#inlineDownloadSpeedButton:hover, QPushButton#inlineUploadSpeedButton:hover {
            background-color: #313244;
            border-color: #45475a;
        }

        /* ---- Line Edit / Text Edit / Spin Box ---- */
        QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox {
            background-color: #11111b;
            color: #cdd6f4;
            border: 1px solid #313244;
            border-radius: 6px;
            padding: 5px 8px;
            min-height: 20px;
            selection-background-color: rgba(137, 180, 250, 0.25);
            selection-color: #cdd6f4;
        }
        QLineEdit:hover, QTextEdit:hover, QPlainTextEdit:hover,
        QSpinBox:hover, QDoubleSpinBox:hover {
            border-color: #45475a;
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus,
        QSpinBox:focus, QDoubleSpinBox:focus {
            border-color: #89b4fa;
        }
        QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled,
        QSpinBox:disabled, QDoubleSpinBox:disabled {
            background-color: #181825;
            color: #585b70;
            border-color: #313244;
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
            border-radius: 6px;
            padding: 5px 8px;
            min-height: 20px;
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
        QComboBox:disabled {
            background-color: #181825;
            color: #585b70;
            border-color: #313244;
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
        QCheckBox:focus, QRadioButton:focus {
            outline: none;
        }
        QCheckBox::indicator:focus, QRadioButton::indicator:focus {
            border-color: #89b4fa;
        }
        QCheckBox::indicator:pressed, QRadioButton::indicator:pressed {
            background-color: #313244;
        }
        QCheckBox:disabled, QRadioButton:disabled {
            color: #6c7086;
        }
        QCheckBox::indicator:disabled, QRadioButton::indicator:disabled {
            background-color: #181825;
            border-color: #313244;
        }

        /* ---- Progress Bar ---- */
        QProgressBar {
            background-color: #11111b;
            border: none;
            border-radius: 3px;
            text-align: center;
            color: #bac2de;
            max-height: 18px;
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
        QSlider::handle:horizontal:pressed {
            background-color: #74c7ec;
        }
        QSlider:disabled::groove:horizontal {
            background-color: #1e1e2e;
        }
        QSlider:disabled::handle:horizontal {
            background-color: #45475a;
        }

        /* ---- Splitter ---- */
        QSplitter::handle {
            background-color: #11111b;
        }
        QSplitter::handle:horizontal {
            width: 3px;
        }
        QSplitter::handle:vertical {
            height: 3px;
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
        QLabel#torrentCardsEmptyState {
            color: #a6adc8;
            font-size: 13px;
            padding: 28px;
        }
        QLabel#transferListEmptyState {
            color: #a6adc8;
            font-size: 14px;
            line-height: 150%;
            padding: 36px;
        }

        /* ---- Properties panel ---- */
        QStackedWidget {
            background-color: #1e1e2e;
        }
        QScrollArea > QWidget > QWidget {
            background-color: #1e1e2e;
        }
    )"_s;

    applyCatppuccinPalette(qss, theme);
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
