/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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

#ifndef QG_LINETYPEBOX_H
#define QG_LINETYPEBOX_H

#include <QComboBox>

#include "rs.h"
#include "rs_pen.h"

class LC_LineTypeList;

/**
 * A combobox for choosing a line type.
 */
class QG_LineTypeBox : public QComboBox {
    Q_OBJECT
public:
    explicit QG_LineTypeBox(QWidget* parent = nullptr);
    QG_LineTypeBox(bool showByLayer, bool showUnchanged, QWidget* parent = nullptr, const char* name = nullptr);
    ~QG_LineTypeBox() override;

    RS2::LineType getLineType() const {
        return m_currentLineType;
    }

    void setLineType(RS2::LineType t);
    void setLayerLineType(RS2::LineType t);

    void init(bool showByLayer, bool showUnchanged, bool showNoPen = false);
    /// Rows for a drawing: the built-in rows, then the line types its table adds.
    /// A null list gives the built-in combo.
    void init(const LC_LineTypeList* list, bool showByLayer, bool showUnchanged, bool showNoPen = false);
    /// Refills the rows for another drawing (null: built-ins only), keeps the
    /// selection, emits nothing.
    void rebuild(const LC_LineTypeList* list);
    /// Selects the row of a pen's line type; a name without a row gets a
    /// temporary one.
    void setLineTypeOf(const RS_Pen& pen);
    /// Writes the selected row's line type into a pen.
    void applyLineTypeTo(RS_Pen& pen) const;

    bool isUnchanged() const {
        return m_unchanged;
    }

private
    slots :
    void slotLineTypeChanged(int index);
    signals :
    void lineTypeChanged(RS2::LineType);

private:
    /// Per-row data: the enum, the spelling the row writes, and the row kind.
    enum ItemRole {
        LineTypeRole = Qt::UserRole,
        LineTypeNameRole = Qt::UserRole + 1,
        LineTypeRowKindRole = Qt::UserRole + 2,
    };
    /// The kinds start at 1: a row without one (the separator) reads as 0.
    enum RowKind {
        SpecialRow = 1,
        BuiltInRow,
        TableRow,
        TemporaryRow,
    };

    void fillRows(const LC_LineTypeList* list);
    void readRow(int index);
    int indexForName(const QString& name) const;
    int indexForEnum(RS2::LineType type) const;
    int indexForRowOfName(const QString& name, RS2::LineType type) const;
    bool isCanonicalName(const QString& name, RS2::LineType type) const;
    void removeTemporaryRow();
    int appendTemporaryRow(const QString& name, RS2::LineType type);
    void ensureSeparator();

    RS2::LineType m_currentLineType = RS2::SolidLine;
    bool m_showByLayer = false;
    bool m_showUnchanged = false;
    bool m_showNoPen = false;
    bool m_unchanged = false;
    bool m_hasList = false;
    bool m_initialized = false;
    bool m_hasSeparator = false;
    RS_Pen m_sourcePen;
    bool m_hasSource = false;
    int m_sourceRowIndex = -1;
};

#endif
