/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

#ifndef QC_PLUGININTERFACE_H
#define QC_PLUGININTERFACE_H

#include <QtPlugin>

class Document_Interface;

class PluginMenuLocation
{
    public:
    PluginMenuLocation(QString menuEntryPoint, QString menuEntryActionName) {
        this->menuEntryActionName=menuEntryActionName;
        this->menuEntryPoint=menuEntryPoint;
    }

    QString menuEntryPoint;
    QString menuEntryActionName;
};

/**
 * Interface for create plugins.
 *
 * @author Rallaz
 */
class QC_PluginInterface
{
public:
    virtual ~QC_PluginInterface() {}
    virtual QString name() const = 0;
    virtual QList<PluginMenuLocation> menu() const = 0;
    virtual void execComm(Document_Interface *doc, QWidget *parent) = 0;

};

Q_DECLARE_INTERFACE(QC_PluginInterface,  "org.librecad.PluginInterface/1.0");


#endif

