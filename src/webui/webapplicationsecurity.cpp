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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
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

#include "webapplicationsecurity.h"

#include <QRegularExpression>

#include "base/http/types.h"
#include "base/utils/random.h"

QString WebApplicationSecurity::generateContentSecurityPolicyNonce()
{
    return QString::fromLatin1(Utils::Random::bytes(16).toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

bool WebApplicationSecurity::isHtmlResponse(const Path &path, const QString &mimeTypeName)
{
    return mimeTypeName.startsWith(Http::CONTENT_TYPE_HTML)
        || path.toString().endsWith(u".html"_s, Qt::CaseInsensitive);
}

QByteArray WebApplicationSecurity::addNonceToScriptTags(const QByteArray &data, const QString &nonce)
{
    if (nonce.isEmpty())
        return data;

    QString html = QString::fromUtf8(data);
    static const QRegularExpression scriptTagPattern {uR"((<script\b)(?![^>]*\bnonce\s*=))"_s, QRegularExpression::CaseInsensitiveOption};
    html.replace(scriptTagPattern, uR"(\1 nonce="%1")"_s.arg(nonce));
    return html.toUtf8();
}
