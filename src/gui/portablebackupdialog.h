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

#include <QDialog>

class QCheckBox;
class QLabel;
class QProgressBar;

class PortableBackupDialog final : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(PortableBackupDialog)

public:
    explicit PortableBackupDialog(QWidget *parent = nullptr);

private:
    void exportBackup();
    void importBackup();

    QCheckBox *m_chkSettings = nullptr;
    QCheckBox *m_chkCategories = nullptr;
    QCheckBox *m_chkRSSFeeds = nullptr;
    QCheckBox *m_chkRSSRules = nullptr;
    QCheckBox *m_chkTorrentResume = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_statusLabel = nullptr;
};
