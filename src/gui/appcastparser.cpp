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

#include "appcastparser.h"

#include <QXmlStreamReader>

namespace
{
    QString attributeValue(const QXmlStreamAttributes &attributes, const QStringView name)
    {
        for (const QXmlStreamAttribute &attribute : attributes)
        {
            if (attribute.name() == name)
                return attribute.value().toString();
        }

        return {};
    }
}

bool AppCast::Release::isValid() const
{
    return !version.isEmpty() && !releaseUrl.isEmpty();
}

std::optional<AppCast::Release> AppCast::parseLatestRelease(const QByteArray &data)
{
    QXmlStreamReader xml {data};
    Release release;
    bool inItem = false;

    while (!xml.atEnd())
    {
        xml.readNext();
        if (xml.isStartElement())
        {
            const QStringView name = xml.name();
            if (name == u"item")
            {
                inItem = true;
                release = {};
            }
            else if (inItem && (name == u"title"))
            {
                release.title = xml.readElementText().trimmed();
            }
            else if (inItem && (name == u"link"))
            {
                release.releaseUrl = xml.readElementText().trimmed();
            }
            else if (inItem && ((name == u"version") || (name == u"shortVersionString")))
            {
                if (release.version.isEmpty())
                    release.version = xml.readElementText().trimmed();
                else
                    xml.skipCurrentElement();
            }
            else if (inItem && (name == u"enclosure"))
            {
                const QXmlStreamAttributes attributes = xml.attributes();
                release.downloadUrl = attributeValue(attributes, u"url");
                if (release.version.isEmpty())
                    release.version = attributeValue(attributes, u"version");

                bool ok = false;
                const qint64 length = attributeValue(attributes, u"length").toLongLong(&ok);
                if (ok)
                    release.downloadLength = length;
            }
        }
        else if (xml.isEndElement() && (xml.name() == u"item") && inItem)
        {
            return release.isValid() ? std::optional<Release> {release} : std::nullopt;
        }
    }

    return std::nullopt;
}
