/*******************************************************************************
*
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/
#ifndef LC_PLUGININVOKER_H
#define LC_PLUGININVOKER_H

#include <QObject>

#include "lc_actioncontext.h"

class QC_PluginInterface;
class QC_ApplicationWindow;
class LC_ActionContext;

class LC_PluginInvoker: public QObject{
    Q_OBJECT
public:
    explicit LC_PluginInvoker(QC_ApplicationWindow* appWindow, LC_ActionContext* ctx);
    ~LC_PluginInvoker() override;
    void loadPlugins();
public slots:
    void execPlug();
private:
    QC_ApplicationWindow* m_appWindow = nullptr;
    QList<QC_PluginInterface*> m_loadedPluginList;
    LC_ActionContext* m_actionContext = nullptr;
};

#endif // LC_PLUGININVOKER_H
