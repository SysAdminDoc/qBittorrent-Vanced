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

#include <array>
#include <cstdlib>
#include <utility>

#include <QDebug>
#include <QString>
#include <QUrl>

#include "base/global.h"
#include "base/net/downloadhandlerimpl.h"

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

    void verifyAction(const Net::DownloadRedirectionDecision &decision
            , const Net::DownloadRedirectionAction expectedAction
            , const QString &caseName
            , int &failureCount)
    {
        verify((decision.action == expectedAction)
                , u"%1: unexpected redirection action"_s.arg(caseName)
                , failureCount);
    }
}

int main()
{
    int failureCount = 0;

    const auto httpDecision = Net::classifyDownloadRedirection(
            QUrl {u"https://source.example/original.torrent"_s}
            , QUrl {u"http://mirror.example/download.torrent"_s}
            , 0);
    verifyAction(httpDecision, Net::DownloadRedirectionAction::Follow, u"http"_s, failureCount);
    verify((httpDecision.urlString == u"http://mirror.example/download.torrent"_s), u"http: URL mismatch"_s, failureCount);
    verify((httpDecision.scheme == u"http"_s), u"http: scheme mismatch"_s, failureCount);

    const auto httpsDecision = Net::classifyDownloadRedirection(
            QUrl {u"http://source.example/original.torrent"_s}
            , QUrl {u"https://mirror.example/download.torrent"_s}
            , 0);
    verifyAction(httpsDecision, Net::DownloadRedirectionAction::Follow, u"https"_s, failureCount);
    verify((httpsDecision.urlString == u"https://mirror.example/download.torrent"_s), u"https: URL mismatch"_s, failureCount);
    verify((httpsDecision.scheme == u"https"_s), u"https: scheme mismatch"_s, failureCount);

    const auto relativeDecision = Net::classifyDownloadRedirection(
            QUrl {u"https://source.example/path/original.torrent"_s}
            , QUrl {u"../next.torrent"_s}
            , 0);
    verifyAction(relativeDecision, Net::DownloadRedirectionAction::Follow, u"relative"_s, failureCount);
    verify((relativeDecision.urlString == u"https://source.example/next.torrent"_s), u"relative: URL mismatch"_s, failureCount);
    verify((relativeDecision.scheme == u"https"_s), u"relative: scheme mismatch"_s, failureCount);

    const QString magnet = u"magnet:?xt=urn:btih:0123456789abcdef0123456789abcdef01234567"_s;
    const auto magnetDecision = Net::classifyDownloadRedirection(
            QUrl {u"https://source.example/original.torrent"_s}
            , QUrl {magnet}
            , 0);
    verifyAction(magnetDecision, Net::DownloadRedirectionAction::RedirectToMagnet, u"magnet"_s, failureCount);
    verify((magnetDecision.urlString == magnet), u"magnet: URI mismatch"_s, failureCount);
    verify((magnetDecision.scheme == u"magnet"_s), u"magnet: scheme mismatch"_s, failureCount);

    const QUrl currentUrl {u"https://source.example/original.torrent"_s};
    const std::array<std::pair<QString, QString>, 3> dangerousCases {{
        {u"file:///C:/Windows/win.ini"_s, u"file"_s},
        {u"javascript:alert(1)"_s, u"javascript"_s},
        {u"ftp://mirror.example/download.torrent"_s, u"ftp"_s}
    }};

    for (const auto &[url, scheme] : dangerousCases)
    {
        const auto decision = Net::classifyDownloadRedirection(currentUrl, QUrl {url}, 0);
        verifyAction(decision, Net::DownloadRedirectionAction::RejectDangerousProtocol, url, failureCount);
        verify((decision.urlString == url), u"%1: URL mismatch"_s.arg(url), failureCount);
        verify((decision.scheme == scheme), u"%1: scheme mismatch"_s.arg(url), failureCount);
    }

    const auto magnetUpperDecision = Net::classifyDownloadRedirection(
            QUrl {u"https://source.example/original.torrent"_s}
            , QUrl {u"MAGNET:?xt=urn:btih:abcdef0123456789abcdef0123456789abcdef01"_s}
            , 0);
    verifyAction(magnetUpperDecision, Net::DownloadRedirectionAction::RedirectToMagnet, u"magnet (uppercase)"_s, failureCount);

    const QUrl currentUrl2 {u"https://source.example/original.torrent"_s};
    const std::array<std::pair<QString, QString>, 2> extraDangerousCases {{
        {u"data:text/html,<h1>hi</h1>"_s, u"data"_s},
        {u"gopher://example.com/"_s, u"gopher"_s}
    }};
    for (const auto &[url, scheme] : extraDangerousCases)
    {
        const auto decision = Net::classifyDownloadRedirection(currentUrl2, QUrl {url}, 0);
        verifyAction(decision, Net::DownloadRedirectionAction::RejectDangerousProtocol, url, failureCount);
    }

    const auto belowMaxDecision = Net::classifyDownloadRedirection(
            QUrl {u"https://source.example/original.torrent"_s}
            , QUrl {u"https://mirror.example/download.torrent"_s}
            , Net::MAX_DOWNLOAD_REDIRECTIONS - 1);
    verifyAction(belowMaxDecision, Net::DownloadRedirectionAction::Follow, u"below max redirects"_s, failureCount);

    const auto tooManyRedirectsDecision = Net::classifyDownloadRedirection(
            QUrl {u"https://source.example/original.torrent"_s}
            , QUrl {u"https://mirror.example/download.torrent"_s}
            , Net::MAX_DOWNLOAD_REDIRECTIONS);
    verifyAction(tooManyRedirectsDecision, Net::DownloadRedirectionAction::RejectTooManyRedirects, u"max redirects"_s, failureCount);
    verify(tooManyRedirectsDecision.urlString.isEmpty(), u"max redirects: URL should be empty"_s, failureCount);
    verify(tooManyRedirectsDecision.scheme.isEmpty(), u"max redirects: scheme should be empty"_s, failureCount);

    const auto aboveMaxDecision = Net::classifyDownloadRedirection(
            QUrl {u"https://source.example/original.torrent"_s}
            , QUrl {u"https://mirror.example/download.torrent"_s}
            , Net::MAX_DOWNLOAD_REDIRECTIONS + 5);
    verifyAction(aboveMaxDecision, Net::DownloadRedirectionAction::RejectTooManyRedirects, u"above max redirects"_s, failureCount);

    return (failureCount == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
