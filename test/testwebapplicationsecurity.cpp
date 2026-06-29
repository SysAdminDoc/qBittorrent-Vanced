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

#include <cstdlib>

#include <QCoreApplication>
#include <QDebug>
#include <QRegularExpression>

#include "base/http/types.h"
#include "webui/webapplicationsecurity.h"

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
}

int main(int argc, char **argv)
{
    QCoreApplication app {argc, argv};
    int failureCount = 0;

    const QString firstNonce = WebApplicationSecurity::generateContentSecurityPolicyNonce();
    const QString secondNonce = WebApplicationSecurity::generateContentSecurityPolicyNonce();
    const QRegularExpression noncePattern {u"^[A-Za-z0-9_-]+$"_s};

    verify(noncePattern.match(firstNonce).hasMatch(), u"generated nonce should use URL-safe base64 characters"_s, failureCount);
    verify((firstNonce.size() >= 20), u"generated nonce should carry at least 128 bits of entropy"_s, failureCount);
    verify((firstNonce != secondNonce), u"generated nonce should not be reused"_s, failureCount);

    const QByteArray html = QByteArrayLiteral(
        "<!doctype html><html><head>"
        "<script src=\"login.js\"></script>"
        "<script nonce=\"existing\" src=\"already-safe.js\"></script>"
        "<SCRIPT type=\"module\"></SCRIPT>"
        "</head></html>");
    const QByteArray rewritten = WebApplicationSecurity::addNonceToScriptTags(html, firstNonce);
    const QByteArray plainScriptWithNonce = QByteArrayLiteral("<script nonce=\"")
            + firstNonce.toLatin1() + QByteArrayLiteral("\" src=\"login.js\">");
    const QByteArray uppercaseScriptWithNonce = QByteArrayLiteral("<SCRIPT nonce=\"")
            + firstNonce.toLatin1() + QByteArrayLiteral("\" type=\"module\">");

    verify(rewritten.contains(plainScriptWithNonce)
            , u"plain script tag should receive the CSP nonce"_s, failureCount);
    verify(rewritten.contains("<script nonce=\"existing\" src=\"already-safe.js\">")
            , u"existing script nonce should be preserved"_s, failureCount);
    verify(rewritten.contains(uppercaseScriptWithNonce)
            , u"uppercase script tag should receive the CSP nonce"_s, failureCount);
    verify((WebApplicationSecurity::addNonceToScriptTags(html, {}) == html)
            , u"empty nonce should leave HTML unchanged"_s, failureCount);

    verify(WebApplicationSecurity::isHtmlResponse(Path(u":/www/public/index.html"_s), u"text/html; charset=UTF-8"_s)
            , u"text/html MIME type should be treated as HTML"_s, failureCount);
    verify(WebApplicationSecurity::isHtmlResponse(Path(u":/www/public/index.html"_s), Http::CONTENT_TYPE_TXT)
            , u".html path should be treated as HTML even with a generic MIME type"_s, failureCount);
    verify(!WebApplicationSecurity::isHtmlResponse(Path(u":/www/public/scripts/login.js"_s), Http::CONTENT_TYPE_JS)
            , u"JavaScript response should not be treated as HTML"_s, failureCount);

    return (failureCount == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
