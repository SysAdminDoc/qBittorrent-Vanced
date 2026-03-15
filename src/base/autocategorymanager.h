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

#pragma once

#include <QList>
#include <QObject>
#include <QRegularExpression>
#include <QString>

namespace BitTorrent
{
    class Torrent;
}

struct AutoCategoryRule
{
    QString trackerPattern;
    QString category;
    QRegularExpression regex;
};

class AutoCategoryManager final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(AutoCategoryManager)

public:
    explicit AutoCategoryManager(QObject *parent = nullptr);

    void processTorrent(BitTorrent::Torrent *torrent) const;

    QList<AutoCategoryRule> rules() const;
    void setRules(const QList<AutoCategoryRule> &rules);
    void addRule(const QString &trackerPattern, const QString &category);
    void removeRule(int index);

private:
    void loadRules();
    void saveRules() const;

    QList<AutoCategoryRule> m_rules;
};
