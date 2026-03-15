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

#include "batchoperationsdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "base/bittorrent/session.h"
#include "base/bittorrent/torrent.h"
#include "base/global.h"
#include "base/path.h"

BatchOperationsDialog::BatchOperationsDialog(const QList<BitTorrent::Torrent *> &torrents, QWidget *parent)
    : QDialog(parent)
    , m_torrents(torrents)
{
    setWindowTitle(tr("Batch Operations - %1 torrents selected").arg(torrents.size()));
    setMinimumWidth(500);
    resize(520, 480);

    auto *mainLayout = new QVBoxLayout(this);

    // Info label
    auto *infoLabel = new QLabel(tr("Check the options you want to change. Only checked options will be applied."), this);
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    // Category
    auto *categoryGroup = new QGroupBox(tr("Category"), this);
    auto *categoryLayout = new QHBoxLayout(categoryGroup);
    m_chkCategory = new QCheckBox(tr("Set category:"), categoryGroup);
    m_comboCategory = new QComboBox(categoryGroup);
    m_comboCategory->setEditable(true);
    m_comboCategory->addItem(QString()); // empty = no category
    const auto categories = BitTorrent::Session::instance()->categories();
    for (const auto &category : categories)
        m_comboCategory->addItem(category);
    m_comboCategory->setEnabled(false);
    connect(m_chkCategory, &QCheckBox::toggled, m_comboCategory, &QComboBox::setEnabled);
    categoryLayout->addWidget(m_chkCategory);
    categoryLayout->addWidget(m_comboCategory, 1);
    mainLayout->addWidget(categoryGroup);

    // Save path
    auto *pathGroup = new QGroupBox(tr("Save Path"), this);
    auto *pathLayout = new QHBoxLayout(pathGroup);
    m_chkSavePath = new QCheckBox(tr("Set save path:"), pathGroup);
    m_editSavePath = new QLineEdit(pathGroup);
    m_editSavePath->setEnabled(false);
    connect(m_chkSavePath, &QCheckBox::toggled, m_editSavePath, &QLineEdit::setEnabled);
    pathLayout->addWidget(m_chkSavePath);
    pathLayout->addWidget(m_editSavePath, 1);
    mainLayout->addWidget(pathGroup);

    // Speed limits
    auto *speedGroup = new QGroupBox(tr("Speed Limits"), this);
    auto *speedLayout = new QFormLayout(speedGroup);

    auto *dlRow = new QHBoxLayout;
    m_chkDownLimit = new QCheckBox(speedGroup);
    m_spinDownLimit = new QSpinBox(speedGroup);
    m_spinDownLimit->setRange(-1, 999999);
    m_spinDownLimit->setSuffix(u" KiB/s"_s);
    m_spinDownLimit->setSpecialValueText(tr("Unlimited"));
    m_spinDownLimit->setValue(-1);
    m_spinDownLimit->setEnabled(false);
    connect(m_chkDownLimit, &QCheckBox::toggled, m_spinDownLimit, &QSpinBox::setEnabled);
    dlRow->addWidget(m_chkDownLimit);
    dlRow->addWidget(m_spinDownLimit, 1);
    speedLayout->addRow(tr("Download limit:"), dlRow);

    auto *ulRow = new QHBoxLayout;
    m_chkUpLimit = new QCheckBox(speedGroup);
    m_spinUpLimit = new QSpinBox(speedGroup);
    m_spinUpLimit->setRange(-1, 999999);
    m_spinUpLimit->setSuffix(u" KiB/s"_s);
    m_spinUpLimit->setSpecialValueText(tr("Unlimited"));
    m_spinUpLimit->setValue(-1);
    m_spinUpLimit->setEnabled(false);
    connect(m_chkUpLimit, &QCheckBox::toggled, m_spinUpLimit, &QSpinBox::setEnabled);
    ulRow->addWidget(m_chkUpLimit);
    ulRow->addWidget(m_spinUpLimit, 1);
    speedLayout->addRow(tr("Upload limit:"), ulRow);
    mainLayout->addWidget(speedGroup);

    // Options group
    auto *optionsGroup = new QGroupBox(tr("Options"), this);
    auto *optionsLayout = new QFormLayout(optionsGroup);

    auto *seqRow = new QHBoxLayout;
    m_chkSequential = new QCheckBox(optionsGroup);
    m_comboSequential = new QComboBox(optionsGroup);
    m_comboSequential->addItem(tr("Disable"), false);
    m_comboSequential->addItem(tr("Enable"), true);
    m_comboSequential->setEnabled(false);
    connect(m_chkSequential, &QCheckBox::toggled, m_comboSequential, &QComboBox::setEnabled);
    seqRow->addWidget(m_chkSequential);
    seqRow->addWidget(m_comboSequential, 1);
    optionsLayout->addRow(tr("Sequential download:"), seqRow);

    auto *ssRow = new QHBoxLayout;
    m_chkSuperSeeding = new QCheckBox(optionsGroup);
    m_comboSuperSeeding = new QComboBox(optionsGroup);
    m_comboSuperSeeding->addItem(tr("Disable"), false);
    m_comboSuperSeeding->addItem(tr("Enable"), true);
    m_comboSuperSeeding->setEnabled(false);
    connect(m_chkSuperSeeding, &QCheckBox::toggled, m_comboSuperSeeding, &QComboBox::setEnabled);
    ssRow->addWidget(m_chkSuperSeeding);
    ssRow->addWidget(m_comboSuperSeeding, 1);
    optionsLayout->addRow(tr("Super seeding:"), ssRow);

    auto *tmmRow = new QHBoxLayout;
    m_chkAutoTMM = new QCheckBox(optionsGroup);
    m_comboAutoTMM = new QComboBox(optionsGroup);
    m_comboAutoTMM->addItem(tr("Disable"), false);
    m_comboAutoTMM->addItem(tr("Enable"), true);
    m_comboAutoTMM->setEnabled(false);
    connect(m_chkAutoTMM, &QCheckBox::toggled, m_comboAutoTMM, &QComboBox::setEnabled);
    tmmRow->addWidget(m_chkAutoTMM);
    tmmRow->addWidget(m_comboAutoTMM, 1);
    optionsLayout->addRow(tr("Auto TMM:"), tmmRow);

    mainLayout->addWidget(optionsGroup);
    mainLayout->addStretch();

    // Buttons
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Close, this);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &BatchOperationsDialog::applyChanges);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void BatchOperationsDialog::applyChanges()
{
    for (BitTorrent::Torrent *torrent : m_torrents)
    {
        if (m_chkCategory->isChecked())
            torrent->setCategory(m_comboCategory->currentText());

        if (m_chkSavePath->isChecked())
        {
            torrent->setAutoTMMEnabled(false);
            torrent->setSavePath(Path(m_editSavePath->text()));
        }

        if (m_chkDownLimit->isChecked())
            torrent->setDownloadLimit(m_spinDownLimit->value() * 1024);

        if (m_chkUpLimit->isChecked())
            torrent->setUploadLimit(m_spinUpLimit->value() * 1024);

        if (m_chkSequential->isChecked())
            torrent->setSequentialDownload(m_comboSequential->currentData().toBool());

        if (m_chkSuperSeeding->isChecked())
            torrent->setSuperSeeding(m_comboSuperSeeding->currentData().toBool());

        if (m_chkAutoTMM->isChecked())
            torrent->setAutoTMMEnabled(m_comboAutoTMM->currentData().toBool());
    }

    accept();
}
