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

#include "portablebackupdialog.h"

#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

#include "base/global.h"
#include "base/path.h"
#include "base/logger.h"
#include "base/profile.h"

PortableBackupDialog::PortableBackupDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Portable Backup"));
    setMinimumWidth(500);
    resize(540, 420);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Info
    auto *infoLabel = new QLabel(tr("Create a timestamped backup folder or restore from one. Backups can include settings, categories, RSS data, and torrent resume files."), this);
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    // Options
    auto *optionsGroup = new QGroupBox(tr("Include in backup"), this);
    auto *optionsLayout = new QVBoxLayout(optionsGroup);

    m_chkSettings = new QCheckBox(tr("Application settings (qBittorrent.ini / qBittorrent-data.conf)"), optionsGroup);
    m_chkSettings->setChecked(true);
    optionsLayout->addWidget(m_chkSettings);

    m_chkCategories = new QCheckBox(tr("Categories"), optionsGroup);
    m_chkCategories->setChecked(true);
    optionsLayout->addWidget(m_chkCategories);

    m_chkRSSFeeds = new QCheckBox(tr("RSS feeds"), optionsGroup);
    m_chkRSSFeeds->setChecked(true);
    optionsLayout->addWidget(m_chkRSSFeeds);

    m_chkRSSRules = new QCheckBox(tr("RSS auto-download rules"), optionsGroup);
    m_chkRSSRules->setChecked(true);
    optionsLayout->addWidget(m_chkRSSRules);

    m_chkTorrentResume = new QCheckBox(tr("Torrent resume data (BT_backup/)"), optionsGroup);
    m_chkTorrentResume->setChecked(true);
    optionsLayout->addWidget(m_chkTorrentResume);

    mainLayout->addWidget(optionsGroup);

    // Progress
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setVisible(false);
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);

    mainLayout->addStretch();

    // Buttons
    auto *buttonLayout = new QHBoxLayout;
    auto *exportBtn = new QPushButton(tr("Export Backup Folder"), this);
    auto *importBtn = new QPushButton(tr("Import Backup Folder"), this);
    auto *closeBtn = new QPushButton(tr("Close"), this);

    connect(exportBtn, &QPushButton::clicked, this, &PortableBackupDialog::exportBackup);
    connect(importBtn, &QPushButton::clicked, this, &PortableBackupDialog::importBackup);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    buttonLayout->addWidget(exportBtn);
    buttonLayout->addWidget(importBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);
    mainLayout->addLayout(buttonLayout);
}

void PortableBackupDialog::exportBackup()
{
    const QString exportRoot = QFileDialog::getExistingDirectory(this, tr("Choose Backup Location"),
        QDir::homePath());
    if (exportRoot.isEmpty()) return;

    const QFileInfo exportRootInfo(exportRoot);
    if (!exportRootInfo.isWritable())
    {
        QMessageBox::warning(this, tr("Location Not Writable"),
            tr("Cannot write to the selected location:\n%1").arg(exportRoot));
        return;
    }

    const QString backupDir = QDir(exportRoot).filePath(u"qBittorrent-Vanced-backup-"_s
        + QDateTime::currentDateTime().toString(u"yyyyMMdd-hhmmss"_s));
    if (!QDir().mkpath(backupDir))
    {
        QMessageBox::warning(this, tr("Backup Folder Not Created"), tr("qBittorrent could not create the backup folder:\n%1").arg(backupDir));
        return;
    }

    m_progressBar->setVisible(true);
    m_statusLabel->setVisible(true);
    m_statusLabel->setText(tr("Exporting backup to %1...").arg(backupDir));
    m_progressBar->setRange(0, 0); // indeterminate

    const Path configDir = specialFolderLocation(SpecialFolder::Config);
    const Path dataDir = specialFolderLocation(SpecialFolder::Data);

    QStringList filesToBackup;

    if (m_chkSettings->isChecked())
    {
        // Main config file
        const QString iniPath = configDir.data() + u"/qBittorrent.ini"_s;
        if (QFile::exists(iniPath))
            filesToBackup << iniPath;
        const QString confPath = configDir.data() + u"/qBittorrent-data.conf"_s;
        if (QFile::exists(confPath))
            filesToBackup << confPath;
    }

    if (m_chkCategories->isChecked())
    {
        const QString catPath = dataDir.data() + u"/categories.json"_s;
        if (QFile::exists(catPath))
            filesToBackup << catPath;
    }

    if (m_chkRSSFeeds->isChecked())
    {
        const QString rssPath = dataDir.data() + u"/rss/feeds.json"_s;
        if (QFile::exists(rssPath))
            filesToBackup << rssPath;
    }

    if (m_chkRSSRules->isChecked())
    {
        const QString rulesPath = dataDir.data() + u"/rss/download_rules.json"_s;
        if (QFile::exists(rulesPath))
            filesToBackup << rulesPath;
    }

    int copied = 0;
    for (const QString &file : filesToBackup)
    {
        const QFileInfo fi(file);
        const QString destFile = backupDir + u"/"_s + fi.fileName();
        if (QFile::copy(file, destFile))
            ++copied;
    }

    if (m_chkTorrentResume->isChecked())
    {
        const QString btBackupSrc = dataDir.data() + u"/BT_backup"_s;
        const QString btBackupDst = backupDir + u"/BT_backup"_s;
        if (QDir(btBackupSrc).exists())
        {
            QDir().mkpath(btBackupDst);
            const QFileInfoList entries = QDir(btBackupSrc).entryInfoList(QDir::Files);
            for (const QFileInfo &entry : entries)
            {
                if (QFile::copy(entry.absoluteFilePath(), btBackupDst + u"/"_s + entry.fileName()))
                    ++copied;
            }
        }
    }

    m_progressBar->setRange(0, 1);
    m_progressBar->setValue(1);
    if (copied > 0)
        m_statusLabel->setText(tr("Exported %1 files to: %2").arg(copied).arg(backupDir));
    else
        m_statusLabel->setText(tr("No matching files were found. Created an empty backup folder at: %1").arg(backupDir));
    LogMsg(tr("Portable backup exported to: %1 (%2 files)").arg(backupDir).arg(copied));
}

void PortableBackupDialog::importBackup()
{
    const QString srcDir = QFileDialog::getExistingDirectory(this, tr("Select Backup Folder"),
        QDir::homePath());
    if (srcDir.isEmpty()) return;

    const int result = QMessageBox::warning(this, tr("Import Backup"),
        tr("Importing a backup replaces matching settings and resume files. Restart qBittorrent after the import completes.\n\nContinue?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (result != QMessageBox::Yes) return;

    m_progressBar->setVisible(true);
    m_statusLabel->setVisible(true);
    m_statusLabel->setText(tr("Importing backup from %1...").arg(srcDir));
    m_progressBar->setRange(0, 0);

    const Path configDir = specialFolderLocation(SpecialFolder::Config);
    const Path dataDir = specialFolderLocation(SpecialFolder::Data);

    int restored = 0;

    // Restore settings files
    const QStringList configFiles = {u"qBittorrent.ini"_s, u"qBittorrent-data.conf"_s};
    for (const QString &name : configFiles)
    {
        const QString src = srcDir + u"/"_s + name;
        if (QFile::exists(src))
        {
            const QString dst = configDir.data() + u"/"_s + name;
            QFile::remove(dst);
            if (QFile::copy(src, dst))
                ++restored;
        }
    }

    // Restore categories
    const QString catSrc = srcDir + u"/categories.json"_s;
    if (QFile::exists(catSrc))
    {
        const QString catDst = dataDir.data() + u"/categories.json"_s;
        QFile::remove(catDst);
        if (QFile::copy(catSrc, catDst))
            ++restored;
    }

    // Restore RSS
    const QString rssFeedsSrc = srcDir + u"/feeds.json"_s;
    if (QFile::exists(rssFeedsSrc))
    {
        QDir().mkpath(dataDir.data() + u"/rss"_s);
        const QString dst = dataDir.data() + u"/rss/feeds.json"_s;
        QFile::remove(dst);
        if (QFile::copy(rssFeedsSrc, dst))
            ++restored;
    }

    const QString rssRulesSrc = srcDir + u"/download_rules.json"_s;
    if (QFile::exists(rssRulesSrc))
    {
        QDir().mkpath(dataDir.data() + u"/rss"_s);
        const QString dst = dataDir.data() + u"/rss/download_rules.json"_s;
        QFile::remove(dst);
        if (QFile::copy(rssRulesSrc, dst))
            ++restored;
    }

    // Restore BT_backup
    const QString btSrc = srcDir + u"/BT_backup"_s;
    if (QDir(btSrc).exists())
    {
        const QString btDst = dataDir.data() + u"/BT_backup"_s;
        QDir().mkpath(btDst);
        const QFileInfoList entries = QDir(btSrc).entryInfoList(QDir::Files);
        for (const QFileInfo &entry : entries)
        {
            const QString dst = btDst + u"/"_s + entry.fileName();
            QFile::remove(dst);
            if (QFile::copy(entry.absoluteFilePath(), dst))
                ++restored;
        }
    }

    m_progressBar->setRange(0, 1);
    m_progressBar->setValue(1);
    if (restored > 0)
        m_statusLabel->setText(tr("Imported %1 files. Restart qBittorrent to finish applying the backup.").arg(restored));
    else
        m_statusLabel->setText(tr("No restorable files were found in: %1").arg(srcDir));
    LogMsg(tr("Portable backup imported from: %1 (%2 files). Restart required.").arg(srcDir).arg(restored));
}
