/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2026  SysAdminDoc
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

#include <cstdlib>

#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QWidget>

#include "base/global.h"
#include "base/logger.h"
#include "base/path.h"
#include "base/preferences.h"
#include "base/profile.h"
#include "base/settingsstorage.h"
#include "gui/uithemedialog.h"
#include "gui/uithememanager.h"

namespace
{
    void verify(const bool condition, const QString &message, int &failureCount)
    {
        if (!condition)
        {
            qCritical().noquote() << message;
            ++failureCount;
        }
    }

    QComboBox *flavorCombo(UIThemeDialog &dialog)
    {
        return dialog.findChild<QComboBox *>(u"builtInThemeFlavorComboBox"_s);
    }
}

int main(int argc, char **argv)
{
    QApplication app {argc, argv};
    app.setApplicationName(u"qBittorrent"_s);
    app.setOrganizationDomain(u"qbittorrent.org"_s);

    QTemporaryDir profileRoot;
    if (!profileRoot.isValid())
    {
        qCritical().noquote() << u"Could not create temporary profile root"_s;
        return EXIT_FAILURE;
    }

    Logger::initInstance();
    Profile::initInstance(Path {profileRoot.path()}, u"uithemedialog-test"_s, false);
    SettingsStorage::initInstance();
    Preferences::initInstance();
    Preferences::instance()->setBuiltInUIThemeFlavor(u"mocha"_s);
    UIThemeManager::initInstance();

    int failureCount = 0;

    {
        UIThemeDialog dialog;

        auto *tabs = dialog.findChild<QTabWidget *>(u"tabWidget"_s);
        verify((tabs != nullptr), u"Theme dialog should expose its tab widget"_s, failureCount);
        verify((tabs && (tabs->tabText(0) == u"Built-in"_s)), u"Built-in theme picker should be the first tab"_s, failureCount);

        auto *combo = flavorCombo(dialog);
        verify((combo != nullptr), u"Theme dialog should expose the built-in flavor combo"_s, failureCount);
        verify((combo && (combo->count() == UIThemeManager::builtInThemeFlavorIds().size()))
                , u"Flavor combo should include every built-in Catppuccin flavor"_s, failureCount);
        verify((combo && (combo->findData(u"latte"_s) >= 0)), u"Flavor combo should include Latte"_s, failureCount);
        verify((combo && (combo->findData(u"mocha"_s) >= 0)), u"Flavor combo should include Mocha"_s, failureCount);

        auto *previewLabel = dialog.findChild<QLabel *>(u"builtInThemePreviewLabel"_s);
        verify((previewLabel && (previewLabel->text() == u"Torrent list preview"_s))
                , u"Theme dialog should label the torrent list preview"_s, failureCount);

        auto *previewHost = dialog.findChild<QWidget *>(u"builtInThemePreviewHost"_s);
        verify((previewHost && previewHost->layout() && (previewHost->layout()->count() == 1))
                , u"Theme dialog should mount one live preview widget"_s, failureCount);

        combo->setCurrentIndex(combo->findData(u"latte"_s));
        QApplication::processEvents();
        verify((UIThemeManager::instance()->builtInThemeFlavor() == u"latte"_s)
                , u"Changing the flavor combo should apply a live preview"_s, failureCount);

        dialog.reject();
        QApplication::processEvents();
        verify((UIThemeManager::instance()->builtInThemeFlavor() == u"mocha"_s)
                , u"Rejecting the dialog should restore the original flavor"_s, failureCount);
    }

    {
        UIThemeDialog dialog;
        auto *combo = flavorCombo(dialog);
        verify((combo != nullptr), u"Theme dialog should expose the flavor combo for accept smoke"_s, failureCount);

        combo->setCurrentIndex(combo->findData(u"frappe"_s));
        QApplication::processEvents();
        dialog.QDialog::accept();
        QApplication::processEvents();
        verify((UIThemeManager::instance()->builtInThemeFlavor() == u"frappe"_s)
                , u"Accepting the dialog should keep the selected flavor"_s, failureCount);
    }

    UIThemeManager::freeInstance();
    Preferences::freeInstance();
    SettingsStorage::freeInstance();
    Logger::freeInstance();
    Profile::freeInstance();

    return (failureCount == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
