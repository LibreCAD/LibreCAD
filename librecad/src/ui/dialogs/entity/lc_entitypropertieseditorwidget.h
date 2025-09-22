/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_ENTITYPROPERTIESEDITORWIDGET_H
#define LC_ENTITYPROPERTIESEDITORWIDGET_H
#include <QWidget>
#include <QToolButton>

#include "lc_actioncontext.h"
#include "lc_entitypropertieseditorsupport.h"
#include "rs_entity.h"

class LC_PointPickButton;

class LC_EntityPropertiesEditorWidget: public QWidget,
                                       public LC_EntityPropertiesEditorSupport{
    Q_OBJECT
public:
    explicit LC_EntityPropertiesEditorWidget(QWidget* parent);
    virtual void setEntity(RS_Entity* entity) = 0;
    virtual void updateEntityData() {};
    void interactiveInputUpdate(LC_ActionContext::InteractiveInputInfo::InputType inputType,
        const QString &tag, double valueOne, double valueTwo);
    virtual void setupInteractiveInputWidgets() {};
signals:
    void interactiveInputRequested(LC_ActionContext::InteractiveInputInfo::InputType inputType, QString tag);
protected slots:
    void onInteractiveInputButtonClicked(bool checked);
    void pickDistanceSetup(QToolButton* button, const QString &tag, QLineEdit* lineEditOne, QLineEdit* lineEditTwo = nullptr);
    void pickAngleSetup(QToolButton* button, const QString &tag, QLineEdit* lineEditOne, QLineEdit* lineEditTwo = nullptr);
    void pickPointSetup(LC_PointPickButton* button, const QString &tag, QLineEdit* lineEditOne, QLineEdit* lineEditTwo = nullptr);
protected:
    void setupInteractiveInputControls(QToolButton* button, LC_ActionContext::InteractiveInputInfo::InputType inputType, const QString &tag,
         QLineEdit* lineEditOne, QLineEdit* lineEditTwo = nullptr);
};

#endif // LC_ENTITYPROPERTIESEDITORWIDGET_H
