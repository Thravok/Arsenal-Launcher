// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PrismLaucher - Minecraft Launcher
 *  Copyright (C) 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "SubTaskProgressBar.h"
#include "ui_SubTaskProgressBar.h"

#include <QFontMetrics>

unique_qobject_ptr<SubTaskProgressBar> SubTaskProgressBar::create(QWidget* parent)
{
    auto progress_bar = new SubTaskProgressBar(parent);
    return unique_qobject_ptr<SubTaskProgressBar>(progress_bar);
}

SubTaskProgressBar::SubTaskProgressBar(QWidget* parent) : QWidget(parent), ui(new Ui::SubTaskProgressBar)
{
    ui->setupUi(this);
    ui->statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui->statusDetailsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
}

SubTaskProgressBar::~SubTaskProgressBar()
{
    delete ui;
}

void SubTaskProgressBar::setRange(int min, int max)
{
    ui->progressBar->setRange(min, max);
}

void SubTaskProgressBar::setValue(int value)
{
    ui->progressBar->setValue(value);
}

void SubTaskProgressBar::setStatus(QString status)
{
    m_fullStatus = status;
    ui->statusLabel->setToolTip(status);
    updateElidedStatus();
}

void SubTaskProgressBar::setDetails(QString details)
{
    // Keep transfer stats on one line for a cleaner meta row.
    const auto singleLine = details.simplified();
    m_fullDetails = singleLine;
    ui->statusDetailsLabel->setToolTip(singleLine);
    ui->statusDetailsLabel->setVisible(!singleLine.isEmpty());
    updateElidedDetails();
}

void SubTaskProgressBar::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateElidedStatus();
    updateElidedDetails();
}

void SubTaskProgressBar::updateElidedStatus()
{
    if (m_fullStatus.isEmpty()) {
        ui->statusLabel->setText({});
        return;
    }
    const QFontMetrics metrics(ui->statusLabel->font());
    ui->statusLabel->setText(metrics.elidedText(m_fullStatus, Qt::ElideMiddle, ui->statusLabel->width()));
}

void SubTaskProgressBar::updateElidedDetails()
{
    if (m_fullDetails.isEmpty()) {
        ui->statusDetailsLabel->setText({});
        return;
    }
    const QFontMetrics metrics(ui->statusDetailsLabel->font());
    ui->statusDetailsLabel->setText(metrics.elidedText(m_fullDetails, Qt::ElideRight, ui->statusDetailsLabel->width()));
}
