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
#include <QList>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;

namespace BitTorrent
{
    class Torrent;
}

class BatchOperationsDialog final : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(BatchOperationsDialog)

public:
    explicit BatchOperationsDialog(const QList<BitTorrent::Torrent *> &torrents, QWidget *parent = nullptr);

private:
    void applyChanges();

    QList<BitTorrent::Torrent *> m_torrents;

    QCheckBox *m_chkCategory = nullptr;
    QComboBox *m_comboCategory = nullptr;

    QCheckBox *m_chkSavePath = nullptr;
    QLineEdit *m_editSavePath = nullptr;

    QCheckBox *m_chkDownLimit = nullptr;
    QSpinBox *m_spinDownLimit = nullptr;

    QCheckBox *m_chkUpLimit = nullptr;
    QSpinBox *m_spinUpLimit = nullptr;

    QCheckBox *m_chkSequential = nullptr;
    QComboBox *m_comboSequential = nullptr;

    QCheckBox *m_chkSuperSeeding = nullptr;
    QComboBox *m_comboSuperSeeding = nullptr;

    QCheckBox *m_chkAutoTMM = nullptr;
    QComboBox *m_comboAutoTMM = nullptr;
};
