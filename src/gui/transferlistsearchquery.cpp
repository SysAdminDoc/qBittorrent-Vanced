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

#include "transferlistsearchquery.h"

#include <array>
#include <optional>

#include <QRegularExpression>
#include <QStringList>

#include "base/global.h"
#include "base/utils/string.h"

namespace
{
    enum class NumericKind
    {
        Count,
        Bytes,
        BytesPerSecond,
        Percent,
        Ratio,
        Seconds
    };

    struct FieldDescriptor
    {
        const QStringView alias;
        const TransferListSearchQuery::Field field;
        const std::optional<NumericKind> numericKind;
    };

    struct OperatorDescriptor
    {
        const QStringView token;
        const TransferListSearchQuery::Operator op;
    };

    const std::array<FieldDescriptor, 49> FIELD_DESCRIPTORS =
    {{
        {u"name", TransferListSearchQuery::Field::Name, std::nullopt},
        {u"torrent", TransferListSearchQuery::Field::Name, std::nullopt},
        {u"category", TransferListSearchQuery::Field::Category, std::nullopt},
        {u"cat", TransferListSearchQuery::Field::Category, std::nullopt},
        {u"tag", TransferListSearchQuery::Field::Tags, std::nullopt},
        {u"tags", TransferListSearchQuery::Field::Tags, std::nullopt},
        {u"tracker", TransferListSearchQuery::Field::Tracker, std::nullopt},
        {u"status", TransferListSearchQuery::Field::Status, std::nullopt},
        {u"state", TransferListSearchQuery::Field::Status, std::nullopt},
        {u"savepath", TransferListSearchQuery::Field::SavePath, std::nullopt},
        {u"location", TransferListSearchQuery::Field::SavePath, std::nullopt},
        {u"path", TransferListSearchQuery::Field::SavePath, std::nullopt},
        {u"downloadpath", TransferListSearchQuery::Field::DownloadPath, std::nullopt},
        {u"incompletepath", TransferListSearchQuery::Field::DownloadPath, std::nullopt},
        {u"size", TransferListSearchQuery::Field::Size, NumericKind::Bytes},
        {u"wantedsize", TransferListSearchQuery::Field::Size, NumericKind::Bytes},
        {u"totalsize", TransferListSearchQuery::Field::TotalSize, NumericKind::Bytes},
        {u"total", TransferListSearchQuery::Field::TotalSize, NumericKind::Bytes},
        {u"progress", TransferListSearchQuery::Field::Progress, NumericKind::Percent},
        {u"done", TransferListSearchQuery::Field::Progress, NumericKind::Percent},
        {u"seed", TransferListSearchQuery::Field::Seeds, NumericKind::Count},
        {u"seeds", TransferListSearchQuery::Field::Seeds, NumericKind::Count},
        {u"seeder", TransferListSearchQuery::Field::Seeds, NumericKind::Count},
        {u"seeders", TransferListSearchQuery::Field::Seeds, NumericKind::Count},
        {u"peer", TransferListSearchQuery::Field::Peers, NumericKind::Count},
        {u"peers", TransferListSearchQuery::Field::Peers, NumericKind::Count},
        {u"leecher", TransferListSearchQuery::Field::Peers, NumericKind::Count},
        {u"leechers", TransferListSearchQuery::Field::Peers, NumericKind::Count},
        {u"dlspeed", TransferListSearchQuery::Field::DownloadSpeed, NumericKind::BytesPerSecond},
        {u"downloadspeed", TransferListSearchQuery::Field::DownloadSpeed, NumericKind::BytesPerSecond},
        {u"downspeed", TransferListSearchQuery::Field::DownloadSpeed, NumericKind::BytesPerSecond},
        {u"dl", TransferListSearchQuery::Field::DownloadSpeed, NumericKind::BytesPerSecond},
        {u"down", TransferListSearchQuery::Field::DownloadSpeed, NumericKind::BytesPerSecond},
        {u"upspeed", TransferListSearchQuery::Field::UploadSpeed, NumericKind::BytesPerSecond},
        {u"uploadspeed", TransferListSearchQuery::Field::UploadSpeed, NumericKind::BytesPerSecond},
        {u"up", TransferListSearchQuery::Field::UploadSpeed, NumericKind::BytesPerSecond},
        {u"ul", TransferListSearchQuery::Field::UploadSpeed, NumericKind::BytesPerSecond},
        {u"ratio", TransferListSearchQuery::Field::Ratio, NumericKind::Ratio},
        {u"eta", TransferListSearchQuery::Field::ETA, NumericKind::Seconds},
        {u"remaining", TransferListSearchQuery::Field::Remaining, NumericKind::Bytes},
        {u"left", TransferListSearchQuery::Field::Remaining, NumericKind::Bytes},
        {u"downloaded", TransferListSearchQuery::Field::Downloaded, NumericKind::Bytes},
        {u"download", TransferListSearchQuery::Field::Downloaded, NumericKind::Bytes},
        {u"dltotal", TransferListSearchQuery::Field::Downloaded, NumericKind::Bytes},
        {u"uploaded", TransferListSearchQuery::Field::Uploaded, NumericKind::Bytes},
        {u"upload", TransferListSearchQuery::Field::Uploaded, NumericKind::Bytes},
        {u"uptotal", TransferListSearchQuery::Field::Uploaded, NumericKind::Bytes},
        {u"completed", TransferListSearchQuery::Field::Progress, NumericKind::Percent},
        {u"complete", TransferListSearchQuery::Field::Progress, NumericKind::Percent}
    }};

    const std::array<OperatorDescriptor, 7> OPERATOR_DESCRIPTORS =
    {{
        {u">=", TransferListSearchQuery::Operator::GreaterOrEqual},
        {u"<=", TransferListSearchQuery::Operator::LessOrEqual},
        {u"!=", TransferListSearchQuery::Operator::NotEqual},
        {u">", TransferListSearchQuery::Operator::GreaterThan},
        {u"<", TransferListSearchQuery::Operator::LessThan},
        {u":", TransferListSearchQuery::Operator::Contains},
        {u"=", TransferListSearchQuery::Operator::Equal}
    }};

    struct ParsedField
    {
        TransferListSearchQuery::Field field = TransferListSearchQuery::Field::Name;
        std::optional<NumericKind> numericKind;
    };

    struct ParsedOperator
    {
        qsizetype index = -1;
        qsizetype size = 0;
        TransferListSearchQuery::Operator op = TransferListSearchQuery::Operator::Contains;
    };

    std::optional<ParsedField> parseField(QString field)
    {
        field = field.trimmed().toLower();
        field.remove(u'_');
        field.remove(u'-');

        for (const FieldDescriptor &descriptor : FIELD_DESCRIPTORS)
        {
            if (field == descriptor.alias)
                return ParsedField {descriptor.field, descriptor.numericKind};
        }

        return std::nullopt;
    }

    std::optional<ParsedOperator> parseOperator(const QString &token)
    {
        std::optional<ParsedOperator> best;
        for (const OperatorDescriptor &descriptor : OPERATOR_DESCRIPTORS)
        {
            const qsizetype index = token.indexOf(descriptor.token);
            if (index <= 0)
                continue;

            if (!best || (index < best->index) || ((index == best->index) && (descriptor.token.size() > best->size)))
                best = ParsedOperator {index, descriptor.token.size(), descriptor.op};
        }

        return best;
    }

    std::optional<double> unitMultiplier(const QString &suffix, const NumericKind kind)
    {
        if (suffix.isEmpty())
            return 1;

        switch (kind)
        {
        case NumericKind::Bytes:
        case NumericKind::BytesPerSecond:
            if ((suffix == u"b") || (suffix == u"byte") || (suffix == u"bytes"))
                return 1;
            if ((suffix == u"k") || (suffix == u"kb") || (suffix == u"kib"))
                return 1024.0;
            if ((suffix == u"m") || (suffix == u"mb") || (suffix == u"mib"))
                return 1024.0 * 1024;
            if ((suffix == u"g") || (suffix == u"gb") || (suffix == u"gib"))
                return 1024.0 * 1024 * 1024;
            if ((suffix == u"t") || (suffix == u"tb") || (suffix == u"tib"))
                return 1024.0 * 1024 * 1024 * 1024;
            break;
        case NumericKind::Percent:
            if (suffix == u"%")
                return 1;
            break;
        case NumericKind::Seconds:
            if ((suffix == u"s") || (suffix == u"sec") || (suffix == u"secs") || (suffix == u"second") || (suffix == u"seconds"))
                return 1;
            if ((suffix == u"m") || (suffix == u"min") || (suffix == u"mins") || (suffix == u"minute") || (suffix == u"minutes"))
                return 60;
            if ((suffix == u"h") || (suffix == u"hr") || (suffix == u"hrs") || (suffix == u"hour") || (suffix == u"hours"))
                return 60 * 60;
            if ((suffix == u"d") || (suffix == u"day") || (suffix == u"days"))
                return 24 * 60 * 60;
            break;
        case NumericKind::Count:
        case NumericKind::Ratio:
            break;
        }

        return std::nullopt;
    }

    std::optional<double> parseNumericValue(QString value, const NumericKind kind)
    {
        value = Utils::String::unquote(value.trimmed()).toLower();
        value.remove(u',');
        if (value.endsWith(u"/s"))
            value.chop(2);
        if (value.endsWith(u"ps"))
            value.chop(2);

        const QRegularExpression expression {uR"(^([+-]?(?:\d+(?:\.\d*)?|\.\d+))\s*([a-z%]*)$)"_s};
        const QRegularExpressionMatch match = expression.match(value);
        if (!match.hasMatch())
            return std::nullopt;

        bool ok = false;
        const double number = match.captured(1).toDouble(&ok);
        if (!ok)
            return std::nullopt;

        const std::optional<double> multiplier = unitMultiplier(match.captured(2), kind);
        if (!multiplier)
            return std::nullopt;

        return number * *multiplier;
    }

    bool supportsTextOperator(const TransferListSearchQuery::Operator op)
    {
        return (op == TransferListSearchQuery::Operator::Contains)
               || (op == TransferListSearchQuery::Operator::Equal)
               || (op == TransferListSearchQuery::Operator::NotEqual);
    }

    std::optional<TransferListSearchQuery::Condition> parseCondition(const QString &token)
    {
        const std::optional<ParsedOperator> parsedOperator = parseOperator(token);
        if (!parsedOperator)
            return std::nullopt;

        const std::optional<ParsedField> parsedField = parseField(token.left(parsedOperator->index));
        if (!parsedField)
            return std::nullopt;

        QString value = token.mid(parsedOperator->index + parsedOperator->size).trimmed();
        if (parsedField->numericKind)
        {
            const std::optional<double> number = parseNumericValue(value, *parsedField->numericKind);
            if (!number)
                return std::nullopt;

            TransferListSearchQuery::Operator op = parsedOperator->op;
            if (op == TransferListSearchQuery::Operator::Contains)
                op = TransferListSearchQuery::Operator::Equal;

            return TransferListSearchQuery::Condition {parsedField->field, op, TransferListSearchQuery::ValueType::Number, {}, *number};
        }

        if (!supportsTextOperator(parsedOperator->op))
            return std::nullopt;

        TransferListSearchQuery::Operator op = parsedOperator->op;
        value = Utils::String::unquote(value);
        if (value.isEmpty() && (op == TransferListSearchQuery::Operator::Contains))
            op = TransferListSearchQuery::Operator::Equal;

        return TransferListSearchQuery::Condition {parsedField->field, op, TransferListSearchQuery::ValueType::Text, value, 0};
    }
}

TransferListSearchQuery TransferListSearchQuery::parse(const QString &text)
{
    TransferListSearchQuery query;
    const QString trimmedText = text.trimmed();
    if (trimmedText.isEmpty())
        return query;

    QStringList plainTokens;
    const QStringList tokens = Utils::String::splitCommand(trimmedText);
    for (const QString &token : tokens)
    {
        const std::optional<Condition> condition = parseCondition(token);
        if (condition)
            query.m_conditions.append(*condition);
        else
            plainTokens.append(Utils::String::unquote(token));
    }

    query.m_plainText = query.m_conditions.isEmpty()
            ? trimmedText
            : plainTokens.join(u' ');
    return query;
}

const QList<TransferListSearchQuery::Condition> &TransferListSearchQuery::conditions() const
{
    return m_conditions;
}

QString TransferListSearchQuery::plainText() const
{
    return m_plainText;
}

bool TransferListSearchQuery::hasConditions() const
{
    return !m_conditions.isEmpty();
}

bool TransferListSearchQuery::isEmpty() const
{
    return m_conditions.isEmpty() && m_plainText.isEmpty();
}
