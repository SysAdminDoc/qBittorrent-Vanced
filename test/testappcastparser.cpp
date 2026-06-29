/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2026  qBittorrent Vanced contributors
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

#include <QDebug>

#include "base/global.h"
#include "gui/appcastparser.h"

namespace
{
    int fail(const char *message)
    {
        qCritical() << message;
        return 1;
    }

    int expect(const bool condition, const char *message)
    {
        return condition ? 0 : fail(message);
    }
}

int main()
{
    const QByteArray appCast = R"(<?xml version="1.0" encoding="utf-8"?>
<rss version="2.0" xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle">
  <channel>
    <item>
      <title>qBittorrent Vanced v1.2.3</title>
      <sparkle:version>1.2.3</sparkle:version>
      <link>https://example.test/releases/tag/v1.2.3</link>
      <enclosure url="https://example.test/setup.exe" sparkle:version="1.2.3" length="12345" type="application/octet-stream" />
    </item>
  </channel>
</rss>)";

    const std::optional<AppCast::Release> release = AppCast::parseLatestRelease(appCast);
    if (const int result = expect(release.has_value(), "valid appcast should parse"))
        return result;
    if (const int result = expect(release->version == u"1.2.3"_s, "version mismatch"))
        return result;
    if (const int result = expect(release->title == u"qBittorrent Vanced v1.2.3"_s, "title mismatch"))
        return result;
    if (const int result = expect(release->releaseUrl == u"https://example.test/releases/tag/v1.2.3"_s, "release URL mismatch"))
        return result;
    if (const int result = expect(release->downloadUrl == u"https://example.test/setup.exe"_s, "download URL mismatch"))
        return result;
    if (const int result = expect(release->downloadLength == 12345, "download length mismatch"))
        return result;

    if (const int result = expect(!AppCast::parseLatestRelease("<rss/>").has_value(), "empty appcast should not parse a release"))
        return result;

    return 0;
}
