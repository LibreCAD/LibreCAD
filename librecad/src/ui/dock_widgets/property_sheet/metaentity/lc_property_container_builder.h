/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#ifndef LC_PROPERTYCONTAINERBUILDER_H
#define LC_PROPERTYCONTAINERBUILDER_H

#include <QObject>

#include "lc_actioncontext.h"
#include "lc_property.h"
#include "lc_property_bool.h"

class LC_PropertyRSVector;
class LC_PropertySheetWidget;
class LC_PropertyDouble;
class LC_PropertyQString;

class LC_PropertyContainerBuilder : public QObject{
    Q_OBJECT
public:
    struct CommandLinkInfo {
        struct LinkPartInfo {
            RS2::ActionType actionType;
            QString title;
            QString tooltip;

            LinkPartInfo(){title = "";actionType = RS2::ActionNone, tooltip = "";}
            LinkPartInfo(RS2::ActionType type, const QString& titl, const QString ttip):actionType{type}, title{titl}, tooltip{ttip} {}
        };
        QString description;
        LinkPartInfo leftLink;
        LinkPartInfo rightLink;

        CommandLinkInfo(const QString &desc, const LinkPartInfo &left, const LinkPartInfo &right): description{desc}, leftLink{left}, rightLink{right} {};
        CommandLinkInfo(const QString &desc, const LinkPartInfo &left): description{desc}, leftLink{left}, rightLink{LinkPartInfo()} {};
    };

    LC_PropertyContainerBuilder(LC_ActionContext* actionContext, LC_PropertySheetWidget* widget)
      : m_actionContext{actionContext}, m_widget{widget} {
    }

    LC_PropertyContainer* createSection(LC_PropertyContainer* container, const LC_Property::Names& names) const;

    LC_PropertyRSVector* createVectorProperty(const LC_Property::Names& names, QList<LC_PropertyAtomic*>* props, LC_PropertyContainer* cont,
                                              LC_ActionContext* actionContext = nullptr, LC_LateCompletionRequestor* requestor = nullptr);

    LC_PropertyQString* createReadonlyStringProperty(const LC_Property::Names& names, QList<LC_PropertyAtomic*>* props,
                                                     LC_PropertyContainer* cont, const QString& value);
    LC_PropertyDouble* createDoubleProperty(const LC_Property::Names& names, LC_PropertyContainer* cont,
                                            LC_ActionContext::InteractiveInputInfo::InputType inputType, LC_ActionContext* actionContext,
                                            LC_LateCompletionRequestor* requestor);

    LC_PropertyDouble* createDoubleProperty(const LC_Property::Names& names, QList<LC_PropertyAtomic*>* props, LC_PropertyContainer* cont,
                                            LC_ActionContext::InteractiveInputInfo::InputType inputType, LC_ActionContext* actionContext,
                                            LC_LateCompletionRequestor* requestor);

    LC_PropertyBool* createBoolProperty(const LC_Property::Names& names, LC_PropertyContainer* cont);

    LC_PropertyRSVector* createVectorProperty(const LC_Property::Names& names, LC_PropertyContainer* cont, LC_ActionContext* actionContext,
                                              LC_LateCompletionRequestor* requestor);

    LC_Formatter* getFormatter() const;
    RS_Vector toUCS(const RS_Vector& wcs) const;
    double toUCSBasisAngle(double wcsAngle) const;
    double toWCSAngle(double ucsBasicAngle) const;
    RS_Vector toWCS(const RS_Vector& ucs) const;
    QString formatLinear(double length) const;
    QString formatDouble(double length) const;
    QString formatInt(int length) const;
    QString formatInt(double length) const;
    QString formatInt(qsizetype length) const;
    QString formatWCSAngleDegrees(double wcsAngle) const;
    QString formatRawAngle(double length) const;
    QString formatRawAngle(double length, RS2::AngleFormat format) const;

    RS_Document* getDocument() const {
        return m_actionContext->getDocument();
    }

protected:
    LC_ActionContext* m_actionContext = nullptr;
    LC_PropertySheetWidget* m_widget = nullptr;
};

#endif
