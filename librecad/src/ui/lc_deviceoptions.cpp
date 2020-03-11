/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2016 ravas (github.com/r-a-v-a-s)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
**********************************************************************************
*/

#include "lc_deviceoptions.h"
#include "ui_lc_deviceoptions.h"
#include "rs.h"
#include "rs_units.h"

#include <QSettings>

static const RS2::InputDeviceType InputDeviceTypes[] = {
	RS2::InputDeviceMouse, RS2::InputDeviceTablet, RS2::InputDeviceTrackpad, RS2::InputDeviceTouchscreen
};

LC_DeviceOptions::LC_DeviceOptions(QWidget* parent) :
    QFrame(parent),
    ui(new Ui::LC_DeviceOptions)
{
    ui->setupUi(this);

	QSettings settings;
    ui->device_combobox->clear();
	for (const auto e : InputDeviceTypes)
		ui->device_combobox->addItem(RS_Units::inputDeviceTypeToString(e));
	int device_type = settings.value("Hardware/DeviceType", -1).toInt();
	QString device = settings.value("Hardware/Device", "Mouse").toString();
	if (device_type != -1)
		device = RS_Units::inputDeviceTypeToString(static_cast<RS2::InputDeviceType>(device_type));
	int index = ui->device_combobox->findText(device);
	ui->device_combobox->setCurrentIndex(index == -1 ? 0 : index);

    connect(ui->save_button, SIGNAL(pressed()), this, SLOT(save()));
    connect(ui->save_button, SIGNAL(released()), parent, SLOT(close()));
}

LC_DeviceOptions::~LC_DeviceOptions()
{
    delete ui;
}


void LC_DeviceOptions::save()
{
    int index = ui->device_combobox->currentIndex();
    QString device = ui->device_combobox->itemText(index);

    QSettings settings;
    settings.setValue("Hardware/Device", device);
	settings.setValue("Hardware/DeviceType", RS_Units::stringToInputDeviceType(device));
}
