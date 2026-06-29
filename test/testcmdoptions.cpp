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

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QStringList>

#include "app/cmdoptions.h"
#include "base/global.h"

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

    void clearEnvironmentOption(const char *name)
    {
        qputenv(name, QByteArray {});
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app {argc, argv};
    int failureCount = 0;

    clearEnvironmentOption("QBT_PORTABLE");
    clearEnvironmentOption("QBT_PROFILE");
    clearEnvironmentOption("QBT_CONFIGURATION");
    clearEnvironmentOption("QBT_RELATIVE_FASTRESUME");

    const QBtCommandLineParameters portableParams = parseCommandLine({u"qbittorrent"_s, u"--portable"_s});
    verify(portableParams.portableMode, u"--portable should enable portable mode"_s, failureCount);
    verify(portableParams.profileDir.isEmpty(), u"--portable should not set an explicit profile path"_s, failureCount);

    const QBtCommandLineParameters portableWithTorrent = parseCommandLine(
            {u"qbittorrent"_s, u"--portable"_s, u"magnet:?xt=urn:btih:0123456789abcdef0123456789abcdef01234567"_s});
    verify(portableWithTorrent.portableMode, u"--portable should compose with torrent sources"_s, failureCount);
    verify((portableWithTorrent.torrentSources.size() == 1), u"torrent source should be preserved"_s, failureCount);

    const QBtCommandLineParameters relativeParams = parseCommandLine(
            {u"qbittorrent"_s, u"--portable"_s, u"--relative-fastresume"_s});
    verify(relativeParams.portableMode, u"--portable with --relative-fastresume should keep portable mode"_s, failureCount);
    verify(relativeParams.relativeFastresumePaths, u"--relative-fastresume should remain set"_s, failureCount);

    bool conflictThrown = false;
    try
    {
        parseCommandLine({u"qbittorrent"_s, u"--portable"_s, u"--profile=C:/Temp/qbt-profile"_s});
    }
    catch (const CommandLineParameterError &)
    {
        conflictThrown = true;
    }
    verify(conflictThrown, u"--portable and --profile should be rejected together"_s, failureCount);

    qputenv("QBT_PORTABLE", "1");
    const QBtCommandLineParameters profileOverridesEnvPortable = parseCommandLine(
            {u"qbittorrent"_s, u"--profile=C:/Temp/qbt-profile"_s});
    verify(!profileOverridesEnvPortable.portableMode, u"--profile should override QBT_PORTABLE"_s, failureCount);
    verify(!profileOverridesEnvPortable.profileDir.isEmpty(), u"--profile should keep its path when overriding QBT_PORTABLE"_s, failureCount);
    clearEnvironmentOption("QBT_PORTABLE");

    qputenv("QBT_PROFILE", "C:/Temp/qbt-env-profile");
    const QBtCommandLineParameters portableOverridesEnvProfile = parseCommandLine({u"qbittorrent"_s, u"--portable"_s});
    verify(portableOverridesEnvProfile.portableMode, u"--portable should override QBT_PROFILE"_s, failureCount);
    verify(portableOverridesEnvProfile.profileDir.isEmpty(), u"--portable should clear QBT_PROFILE"_s, failureCount);
    clearEnvironmentOption("QBT_PROFILE");

    qputenv("QBT_PORTABLE", "1");
    qputenv("QBT_PROFILE", "C:/Temp/qbt-env-profile");
    bool envConflictThrown = false;
    try
    {
        parseCommandLine({u"qbittorrent"_s});
    }
    catch (const CommandLineParameterError &)
    {
        envConflictThrown = true;
    }
    verify(envConflictThrown, u"QBT_PORTABLE and QBT_PROFILE should be rejected together"_s, failureCount);
    clearEnvironmentOption("QBT_PORTABLE");
    clearEnvironmentOption("QBT_PROFILE");

    return (failureCount == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
