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

#include <cmath>

#include <QDebug>

#include "base/global.h"
#include "gui/transferlistsearchquery.h"

namespace
{
    bool fuzzyEqual(const double left, const double right)
    {
        return std::abs(left - right) <= 0.000001;
    }

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
    using Field = TransferListSearchQuery::Field;
    using Operator = TransferListSearchQuery::Operator;
    using ValueType = TransferListSearchQuery::ValueType;

    {
        const TransferListSearchQuery query = TransferListSearchQuery::parse({});
        if (const int result = expect(query.isEmpty(), "empty query should stay empty"))
            return result;
    }

    {
        const TransferListSearchQuery query = TransferListSearchQuery::parse(u"ubuntu iso"_s);
        if (const int result = expect(!query.hasConditions(), "plain query should not create conditions"))
            return result;
        if (const int result = expect(query.plainText() == u"ubuntu iso"_s, "plain query should preserve text"))
            return result;
    }

    {
        const TransferListSearchQuery query = TransferListSearchQuery::parse(u"seeders>10 size<4gb category:movies"_s);
        const QList<TransferListSearchQuery::Condition> &conditions = query.conditions();
        if (const int result = expect(query.plainText().isEmpty(), "fully scoped query should have no plain text"))
            return result;
        if (const int result = expect(conditions.size() == 3, "roadmap example should create three conditions"))
            return result;
        if (const int result = expect((conditions[0].field == Field::Seeds) && (conditions[0].op == Operator::GreaterThan)
                && (conditions[0].valueType == ValueType::Number) && fuzzyEqual(conditions[0].number, 10), "seeders condition mismatch"))
            return result;
        if (const int result = expect((conditions[1].field == Field::Size) && (conditions[1].op == Operator::LessThan)
                && fuzzyEqual(conditions[1].number, 4.0 * 1024 * 1024 * 1024), "size condition mismatch"))
            return result;
        if (const int result = expect((conditions[2].field == Field::Category) && (conditions[2].op == Operator::Contains)
                && (conditions[2].text == u"movies"_s), "category condition mismatch"))
            return result;
    }

    {
        const TransferListSearchQuery query = TransferListSearchQuery::parse(u"ubuntu category:\"TV Shows\" ratio>=1.5"_s);
        const QList<TransferListSearchQuery::Condition> &conditions = query.conditions();
        if (const int result = expect(query.plainText() == u"ubuntu"_s, "mixed query should retain unscoped text"))
            return result;
        if (const int result = expect(conditions.size() == 2, "mixed query should create two conditions"))
            return result;
        if (const int result = expect((conditions[0].field == Field::Category) && (conditions[0].text == u"TV Shows"_s), "quoted category mismatch"))
            return result;
        if (const int result = expect((conditions[1].field == Field::Ratio) && (conditions[1].op == Operator::GreaterOrEqual)
                && fuzzyEqual(conditions[1].number, 1.5), "ratio condition mismatch"))
            return result;
    }

    {
        const TransferListSearchQuery query = TransferListSearchQuery::parse(u"unknown>3 size<soon linux"_s);
        if (const int result = expect(!query.hasConditions(), "invalid scoped tokens should fall back to plain text"))
            return result;
        if (const int result = expect(query.plainText() == u"unknown>3 size<soon linux"_s, "invalid fallback should preserve original text"))
            return result;
    }

    {
        const TransferListSearchQuery query = TransferListSearchQuery::parse(u"category:"_s);
        const QList<TransferListSearchQuery::Condition> &conditions = query.conditions();
        if (const int result = expect(conditions.size() == 1, "empty category token should create a condition"))
            return result;
        if (const int result = expect((conditions[0].field == Field::Category) && (conditions[0].op == Operator::Equal)
                && conditions[0].text.isEmpty(), "empty category should be equality to uncategorized"))
            return result;
    }

    {
        const TransferListSearchQuery query = TransferListSearchQuery::parse(u"dlspeed>500kib/s"_s);
        const QList<TransferListSearchQuery::Condition> &conditions = query.conditions();
        if (const int result = expect(conditions.size() == 1, "speed unit token should create a condition"))
            return result;
        if (const int result = expect((conditions[0].field == Field::DownloadSpeed) && (conditions[0].op == Operator::GreaterThan)
                && fuzzyEqual(conditions[0].number, 500.0 * 1024), "speed unit condition mismatch"))
            return result;
    }

    return 0;
}
