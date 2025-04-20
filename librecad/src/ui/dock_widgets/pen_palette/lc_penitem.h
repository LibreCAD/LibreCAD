/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 sand1024
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
#ifndef LIBRECAD_LC_PENITEM_H
#define LIBRECAD_LC_PENITEM_H
#include <QIcon>

#include "lc_peninforegistry.h"
#include "rs.h"
#include "rs_color.h"

class RS_Pen;
/**
 * Named pen - holds information about pen attributes and presentation of them in UI. Used by table model
 */
class LC_PenItem {
public:
    LC_PenItem(QString name, const RS_Pen&);
    explicit LC_PenItem(QString name);
    QString getName()  const {return m_name;}

    void setPen(const RS_Pen& pen);

    QIcon getColorIcon() {return m_iconColor;};
    QIcon getLineTypeIcon() {return m_iconLineType;};
    QIcon getLineWidthIcon() {return m_iconLineWidth;};

    QString getColorName() {return m_colorName;};
    void setColorName(QString &value){m_colorName = value;};
    QString getLineTypeName() {return m_lineTypeName;};
    QString getLineWidthName() {return m_lineWidthName;};
    RS2::LineType getLineType();

    bool isMatched() const {return m_matched;};
    void setMatched(bool value) {m_matched = value;};
    RS2::LineWidth getLineWidth();

    RS_Color getColor();
    void setLineTypeIcon(QIcon &icon);
    void setLineTypeName(QString name);
    void setLineWidthIcon(QIcon &icon);
    void setLineWidthName(QString name);
    void setColorIcon(QIcon &icon);
    void setLineType(RS2::LineType type);
    void setLineWidth(RS2::LineWidth width);
    void setColor(const RS_Color& col);
private:
    /**
     * Pen's name
     */
    QString m_name;

    /**
     * pens attribute (same as in RS_Pen)
     */
    RS2::LineType m_lineType = RS2::LineType::LineTypeUnchanged;
    RS2::LineWidth m_lineWidth = RS2::LineWidth::WidthUnchanged;
    RS_Color m_color = LC_PenInfoRegistry::createUnchangedColor();

    /**
     * Display attributes for UI
     */
    QIcon m_iconColor;
    QIcon m_iconLineType;
    QIcon m_iconLineWidth;

    QString m_colorName;
    QString m_lineTypeName;
    QString m_lineWidthName;

    /**
     * Indicates whether this pen is matched by filter or not
     */
    bool m_matched{false};
};

#endif //LIBRECAD_LC_PENITEM_H
