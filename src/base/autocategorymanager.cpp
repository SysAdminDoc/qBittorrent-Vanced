/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2024  qBittorrent-Enhanced-Edition contributors
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
 */

#include "autocategorymanager.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "base/bittorrent/torrent.h"
#include "base/global.h"
#include "base/logger.h"
#include "base/preferences.h"

AutoCategoryManager::AutoCategoryManager(QObject *parent)
    : QObject(parent)
{
    loadRules();
}

void AutoCategoryManager::processTorrent(BitTorrent::Torrent *torrent) const
{
    if (!Preferences::instance()->isAutoCategoryEnabled())
        return;

    if (!torrent->category().isEmpty())
        return; // already categorized

    const QString currentTracker = torrent->currentTracker();
    if (currentTracker.isEmpty())
        return;

    for (const AutoCategoryRule &rule : m_rules)
    {
        if (rule.regex.isValid() && rule.regex.match(currentTracker).hasMatch())
        {
            torrent->setCategory(rule.category);
            LogMsg(tr("Auto-categorized torrent '%1' to '%2' (matched tracker: %3)")
                .arg(torrent->name(), rule.category, rule.trackerPattern));
            return;
        }
    }
}

QList<AutoCategoryRule> AutoCategoryManager::rules() const
{
    return m_rules;
}

void AutoCategoryManager::setRules(const QList<AutoCategoryRule> &rules)
{
    m_rules = rules;
    saveRules();
}

void AutoCategoryManager::addRule(const QString &trackerPattern, const QString &category)
{
    AutoCategoryRule rule;
    rule.trackerPattern = trackerPattern;
    rule.category = category;
    rule.regex = QRegularExpression(trackerPattern, QRegularExpression::CaseInsensitiveOption);
    m_rules.append(rule);
    saveRules();
}

void AutoCategoryManager::removeRule(const int index)
{
    if ((index >= 0) && (index < m_rules.size()))
    {
        m_rules.removeAt(index);
        saveRules();
    }
}

void AutoCategoryManager::loadRules()
{
    m_rules.clear();

    const QString rulesJson = Preferences::instance()->autoCategoryRulesJson();
    if (rulesJson.isEmpty())
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(rulesJson.toUtf8());
    if (!doc.isArray())
        return;

    const QJsonArray array = doc.array();
    for (const QJsonValue &val : array)
    {
        const QJsonObject obj = val.toObject();
        AutoCategoryRule rule;
        rule.trackerPattern = obj[u"pattern"_s].toString();
        rule.category = obj[u"category"_s].toString();
        rule.regex = QRegularExpression(rule.trackerPattern, QRegularExpression::CaseInsensitiveOption);
        if (rule.regex.isValid() && !rule.trackerPattern.isEmpty() && !rule.category.isEmpty())
            m_rules.append(rule);
    }
}

void AutoCategoryManager::saveRules() const
{
    QJsonArray array;
    for (const AutoCategoryRule &rule : m_rules)
    {
        QJsonObject obj;
        obj[u"pattern"_s] = rule.trackerPattern;
        obj[u"category"_s] = rule.category;
        array.append(obj);
    }

    Preferences::instance()->setAutoCategoryRulesJson(
        QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact)));
}
