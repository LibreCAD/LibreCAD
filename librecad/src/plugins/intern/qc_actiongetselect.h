/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
**
**
** This file is free software; you can redistribute it and/or modify
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

#ifndef QC_ACTIONGETSELECT_H
#define QC_ACTIONGETSELECT_H

#include <memory>
#include "rs_actioninterface.h"

class Doc_plugin_interface;
class Plug_Entity;
class QString;


/**
 * This action class can handle user events to select entities from plugin.
 *
 * @author  Rallaz
 */
class QC_ActionGetSelect : public RS_ActionInterface {
    Q_OBJECT
public:
    QC_ActionGetSelect(RS_EntityContainer& container,
                       RS_GraphicView& graphicView);

    QC_ActionGetSelect(RS2::EntityType typeToSelect, RS_EntityContainer& container,
                       RS_GraphicView& graphicView);

    ~QC_ActionGetSelect() override;
    void init(int status) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void setMessage(QString msg);
    bool isCompleted() const{return completed;}
    void getSelected(QList<Plug_Entity *> *se, Doc_plugin_interface* d) const;
    void unselectEntities();
protected:
    /**
     * Action States.
     */
    enum Status {
        Select
    };

    RS2::CursorType doGetMouseCursor(int status) override;
    void updateMouseButtonHints() override;
private:
    bool completed = false;
    std::unique_ptr<QString> message;
    RS2::EntityType typeToSelect = RS2::EntityType::EntityUnknown;
};
#endif
