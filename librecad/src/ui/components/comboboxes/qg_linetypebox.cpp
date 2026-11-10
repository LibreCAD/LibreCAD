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

#include "qg_linetypebox.h"

#include <QSignalBlocker>

#include <algorithm>
#include <vector>

#include "lc_linetype.h"
#include "lc_linetypelist.h"
#include "lc_linetypenames.h"
#include "rs_debug.h"

/**
 * Default Constructor. You must call init manually if you choose
 * to use this constructor.
 */
QG_LineTypeBox::QG_LineTypeBox(QWidget* parent)
    : QComboBox(parent) {
    m_showByLayer = false;
    m_showUnchanged = false;
    m_unchanged = false;
}

/**
 * Constructor that calls init and provides a fully functional 
 * combobox for choosing linetypes.
 *
 * @param showByLayer true: Show attribute ByLayer, ByBlock.
 * @param showUnchanged
 * @param parent
 * @param name
 */
QG_LineTypeBox::QG_LineTypeBox(const bool showByLayer, const bool showUnchanged, QWidget* parent, const char* name)
    : QComboBox(parent) {
    setObjectName(name);
    m_unchanged = false;
    init(showByLayer, showUnchanged);
}

/**
 * Destructor
 */
QG_LineTypeBox::~QG_LineTypeBox() = default;

/**
 * Initialisation (called from constructor or manually but only
 * once).
 *
 * @param showByLayer true: Show attribute ByLayer, ByBlock.
 * @param showUnchanged
 * @param showNoPen
 */
void QG_LineTypeBox::init(const LC_LineTypeList* list, const bool showByLayer, const bool showUnchanged, const bool showNoPen) {
    LC_LOG << __func__ << "(): QG_LineTypeBox, begin";
    this->m_showByLayer = showByLayer;
    this->m_showUnchanged = showUnchanged;
    this->m_showNoPen = showNoPen;
    this->m_hasList = list != nullptr;
    fillRows(list);
    // A combo carries no pen: a source from an earlier init() must not survive.
    this->m_hasSource = false;
    this->m_sourceRowIndex = -1;

    // init() runs once per combo; the flag keeps a second call from connecting
    // activated twice.
    if (!m_initialized) {
        connect(this, &QG_LineTypeBox::activated, this, &QG_LineTypeBox::slotLineTypeChanged);
    }

    setCurrentIndex(0);
    slotLineTypeChanged(currentIndex());
    m_initialized = true;
    LC_LOG << __func__ << "(): QG_LineTypeBox, done";
}

void QG_LineTypeBox::init(const bool showByLayer, const bool showUnchanged, const bool showNoPen) {
    init(nullptr, showByLayer, showUnchanged, showNoPen);
}

/**
 * Fills the rows: the special rows, the built-ins, then the table's own line
 * types behind one separator.
 */
void QG_LineTypeBox::fillRows(const LC_LineTypeList* list) {
    clear();
    m_hasSeparator = false;

    if (m_showUnchanged) {
        addItem(QIcon(":linetypes/linetype00.lci"), tr("- Unchanged -"), RS2::LineTypeUnchanged);
    }

    if (m_showByLayer) {
        // addItem(QIcon(":linetypes/linetype00.lci"), tr("By Layer"), RS2::LineByLayer);
        // addItem(QIcon(":linetypes/linetype00.lci"), tr("By Block"), RS2::LineByBlock);
        addItem(QIcon(":/icons/item_by_layer.lci"), tr("By Layer"), RS2::LineByLayer);
        addItem(QIcon(":/icons/point_plus_square.lci"), tr("By Block"),RS2::LineByBlock);
    }
    if (m_showNoPen) {
        addItem(QIcon(":linetypes/linetype00.lci"), tr("No Pen"), RS2::NoPen);
    }
    addItem(QIcon(":linetypes/linetype01.lci"), tr("Continuous"), RS2::SolidLine);
    addItem(QIcon(":linetypes/linetype02.lci"), tr("Dot"), RS2::DotLine);
    addItem(QIcon(":linetypes/linetype02.lci"), tr("Dot (tiny)"), RS2::DotLineTiny);
    addItem(QIcon(":linetypes/linetype02.lci"), tr("Dot (small)"), RS2::DotLine2);
    addItem(QIcon(":linetypes/linetype02.lci"), tr("Dot (large)"), RS2::DotLineX2);
    addItem(QIcon(":linetypes/linetype03.lci"), tr("Dash"), RS2::DashLine);
    addItem(QIcon(":linetypes/linetype03.lci"), tr("Dash (tiny)"), RS2::DashLineTiny);
    addItem(QIcon(":linetypes/linetype03.lci"), tr("Dash (small)"), RS2::DashLine2);
    addItem(QIcon(":linetypes/linetype03.lci"), tr("Dash (large)"), RS2::DashLineX2);
    addItem(QIcon(":linetypes/linetype08.lci"), tr("Hidden"), RS2::HiddenLine);
    addItem(QIcon(":linetypes/linetype08.lci"), tr("Hidden (tiny)"), RS2::HiddenLineTiny);
    addItem(QIcon(":linetypes/linetype08.lci"), tr("Hidden (small)"), RS2::HiddenLine2);
    addItem(QIcon(":linetypes/linetype08.lci"), tr("Hidden (large)"), RS2::HiddenLineX2);
    addItem(QIcon(":linetypes/linetype04.lci"), tr("Dash Dot"), RS2::DashDotLine);
    addItem(QIcon(":linetypes/linetype04.lci"), tr("Dash Dot (tiny)"), RS2::DashDotLineTiny);
    addItem(QIcon(":linetypes/linetype04.lci"), tr("Dash Dot (small)"), RS2::DashDotLine2);
    addItem(QIcon(":linetypes/linetype04.lci"), tr("Dash Dot (large)"), RS2::DashDotLineX2);
    addItem(QIcon(":linetypes/linetype05.lci"), tr("Divide"), RS2::DivideLine);
    addItem(QIcon(":linetypes/linetype05.lci"), tr("Divide (tiny)"), RS2::DivideLineTiny);
    addItem(QIcon(":linetypes/linetype05.lci"), tr("Divide (small)"), RS2::DivideLine2);
    addItem(QIcon(":linetypes/linetype05.lci"), tr("Divide (large)"), RS2::DivideLineX2);
    addItem(QIcon(":linetypes/linetype06.lci"), tr("Center"), RS2::CenterLine);
    addItem(QIcon(":linetypes/linetype06.lci"), tr("Center (tiny)"), RS2::CenterLineTiny);
    addItem(QIcon(":linetypes/linetype06.lci"), tr("Center (small)"), RS2::CenterLine2);
    addItem(QIcon(":linetypes/linetype06.lci"), tr("Center (large)"), RS2::CenterLineX2);
    addItem(QIcon(":linetypes/linetype09.lci"), tr("Phantom"), RS2::PhantomLine);
    addItem(QIcon(":linetypes/linetype09.lci"), tr("Phantom (tiny)"), RS2::PhantomLineTiny);
    addItem(QIcon(":linetypes/linetype09.lci"), tr("Phantom (small)"), RS2::PhantomLine2);
    addItem(QIcon(":linetypes/linetype09.lci"), tr("Phantom (large)"), RS2::PhantomLineX2);
    addItem(QIcon(":linetypes/linetype07.lci"), tr("Border"), RS2::BorderLine);
    addItem(QIcon(":linetypes/linetype07.lci"), tr("Border (tiny)"), RS2::BorderLineTiny);
    addItem(QIcon(":linetypes/linetype07.lci"), tr("Border (small)"), RS2::BorderLine2);
    addItem(QIcon(":linetypes/linetype07.lci"), tr("Border (large)"), RS2::BorderLineX2);

    // The special rows write no name; a built-in row writes its canonical name
    // only when the drawing paints that built-in from its table, so the two
    // cannot disagree.
    const int specialRows = (m_showUnchanged ? 1 : 0) + (m_showByLayer ? 2 : 0) +
                            (m_showNoPen ? 1 : 0);
    for (int i = 0; i < count(); ++i) {
        if (i < specialRows) {
            setItemData(i, QString(), LineTypeNameRole);
            setItemData(i, SpecialRow, LineTypeRowKindRole);
            continue;
        }
        const auto type = static_cast<RS2::LineType>(itemData(i).toInt());
        QString name;
        if (list != nullptr) {
            const QString canonical = LC_LineTypeNames::lineTypeToName(type);
            const LC_LineType* entry = list->find(canonical);
            if (entry != nullptr && LC_LineTypeList::redefinesBuiltIn(*entry)) {
                name = canonical;
            }
        }
        setItemData(i, name, LineTypeNameRole);
        setItemData(i, BuiltInRow, LineTypeRowKindRole);
    }

    if (list == nullptr) {
        return;
    }
    // The seeds are the rows above. A spelling that is not its enum's canonical
    // name is a line type only this drawing holds.
    std::vector<const LC_LineType*> rows;
    for (unsigned i = 0; i < list->count(); ++i) {
        const LC_LineType* entry = list->at(i);
        if (entry == nullptr) {
            continue;
        }
        const QString canonical = LC_LineTypeNames::lineTypeToName(
            LC_LineTypeNames::nameToLineType(entry->name));
        // key() is already the fold of the name.
        if (entry->key() == LC_LineTypeNames::foldName(canonical)) {
            continue;
        }
        rows.push_back(entry);
    }
    if (rows.empty()) {
        return;
    }
    std::sort(rows.begin(), rows.end(),
              [](const LC_LineType* a, const LC_LineType* b) { return a->key() < b->key(); });
    insertSeparator(count());
    m_hasSeparator = true;
    for (const LC_LineType* entry : rows) {
        const int index = count();
        addItem(entry->name);
        setItemData(index, static_cast<int>(LC_LineTypeNames::nameToLineType(entry->name)), LineTypeRole);
        setItemData(index, entry->name, LineTypeNameRole);
        setItemData(index, TableRow, LineTypeRowKindRole);
        if (!entry->description.isEmpty()) {
            setItemData(index, entry->description, Qt::ToolTipRole);
        }
    }
}

/**
 * Refills the rows for another drawing, keeps the selection and emits nothing.
 */
void QG_LineTypeBox::rebuild(const LC_LineTypeList* list) {
    if (!m_initialized) {
        return;
    }
    const QSignalBlocker blocker(this);
    const int previous = currentIndex();
    const QString selectedName =
        previous >= 0 ? itemData(previous, LineTypeNameRole).toString() : QString();
    const auto selectedType = previous >= 0
        ? static_cast<RS2::LineType>(itemData(previous, LineTypeRole).toInt())
        : m_currentLineType;
    // Only a selection that still sits on the source row keeps that source; a
    // pen the user replaced must not come back.
    const bool wasSource = m_hasSource && previous >= 0 && previous == m_sourceRowIndex;
    m_hasList = list != nullptr;
    fillRows(list);
    int index = -1;
    if (!selectedName.isEmpty() && m_hasList) {
        index = indexForRowOfName(selectedName, selectedType);
        if (index < 0 && !isCanonicalName(selectedName, selectedType)) {
            index = appendTemporaryRow(selectedName, selectedType);
        }
    }
    if (index < 0) {
        index = indexForEnum(selectedType);
    }
    if (index < 0) {
        // Neither the name nor the enum finds a row: start at row 0, as init()
        // does, and forget the pen behind the old selection.
        index = 0;
        m_hasSource = false;
    }
    setCurrentIndex(index);
    readRow(index);
    if (wasSource && m_hasSource) {
        m_sourceRowIndex = currentIndex();
    }
    else {
        m_hasSource = false;
    }
}

/**
 * Selects the row of a pen's line type; a name without a row gets a temporary
 * row, so the combo always shows what the pen says.
 */
void QG_LineTypeBox::setLineTypeOf(const RS_Pen& pen) {
    if (m_hasList) {
        removeTemporaryRow();
    }
    int index = -1;
    if (m_hasList && pen.hasLineTypeName()) {
        const QString name = pen.getLineTypeName();
        index = indexForRowOfName(name, pen.getLineType());
        if (index < 0 && !isCanonicalName(name, pen.getLineType())) {
            index = appendTemporaryRow(name, pen.getLineType());
        }
    }
    // Record the pen before anything emits: a host reading the pen inside the
    // signal must get this pen, not the spelling of the row it selected.
    m_sourcePen = pen;
    m_hasSource = true;
    if (index < 0) {
        // The row setLineType(enum) is about to select; none when the combo has
        // no row for the enum, so the recorded row stays none.
        m_sourceRowIndex = indexForEnum(pen.getLineType());
        // A pen without a name, or a canonical spelling the combo does not show,
        // behaves as setLineType() does today.
        setLineType(pen.getLineType());
        return;
    }
    m_sourceRowIndex = index;
    setCurrentIndex(index);
    slotLineTypeChanged(currentIndex());
}

/**
 * Writes the selected row's line type into a pen. An untouched selection copies
 * the pen it was set from, so the pen's own spelling survives.
 */
void QG_LineTypeBox::applyLineTypeTo(RS_Pen& pen) const {
    if (m_hasSource && currentIndex() >= 0 && currentIndex() == m_sourceRowIndex) {
        pen.setLineTypeFromPen(m_sourcePen);
        return;
    }
    const int index = currentIndex();
    const QString name = index >= 0 ? itemData(index, LineTypeNameRole).toString() : QString();
    if (!name.isEmpty()) {
        pen.setLineTypeName(name);
    }
    else if (index < 0 || isUnchanged()) {
        // No row or "- Unchanged -": keep the value the combo last reported.
        pen.setLineType(getLineType());
    }
    else {
        // The row shown decides, even when a host moved the index itself.
        pen.setLineType(static_cast<RS2::LineType>(itemData(index, LineTypeRole).toInt()));
    }
}

std::optional<LC_LineType> QG_LineTypeBox::definitionOf(const RS_Pen&) const {
    return std::nullopt;
}

void QG_LineTypeBox::setLayerLineTypeOf(const RS_Pen& layerPen) {
    setLayerLineType(layerPen.getLineType());
}

int QG_LineTypeBox::indexForName(const QString& name) const {
    // By LC_LineType::key(): a record keeps the blanks around its name, a pen
    // trims them, and both are one name.
    const QString key = LC_LineTypeNames::foldName(name.trimmed());
    for (int i = 0; i < count(); ++i) {
        const QString rowName = itemData(i, LineTypeNameRole).toString();
        if (!rowName.isEmpty() && LC_LineTypeNames::foldName(rowName.trimmed()) == key) {
            return i;
        }
    }
    return -1;
}

int QG_LineTypeBox::indexForEnum(const RS2::LineType type) const {
    switch (type) {
        case RS2::LineByLayer:
            return m_showByLayer ? (m_showUnchanged ? 1 : 0) : -1;
        case RS2::LineByBlock:
            return m_showByLayer ? (m_showUnchanged ? 2 : 1) : -1;
        default:
            return findData(type);
    }
}

/**
 * The row a name selects: its row by folded key, else, for the canonical
 * spelling of an enum, the built-in row setLineType(enum) would select.
 */
int QG_LineTypeBox::indexForRowOfName(const QString& name, const RS2::LineType type) const {
    const int byName = indexForName(name);
    if (byName >= 0) {
        return byName;
    }
    if (isCanonicalName(name, type)) {
        return indexForEnum(type);
    }
    return -1;
}

bool QG_LineTypeBox::isCanonicalName(const QString& name, const RS2::LineType type) const {
    return LC_LineTypeNames::foldName(name.trimmed()) ==
           LC_LineTypeNames::foldName(LC_LineTypeNames::lineTypeToName(type));
}

void QG_LineTypeBox::removeTemporaryRow() {
    // Found by kind: the text carries a translated sentence and the pen's name.
    for (int i = count() - 1; i >= 0; --i) {
        if (itemData(i, LineTypeRowKindRole).toInt() == TemporaryRow) {
            removeItem(i);
            break;
        }
    }
    if (!m_hasSeparator) {
        return;
    }
    // A separator that only introduced the temporary row has nothing left to
    // separate.
    for (int i = 0; i < count(); ++i) {
        const int kind = itemData(i, LineTypeRowKindRole).toInt();
        if (kind == TableRow || kind == TemporaryRow) {
            return;
        }
    }
    removeItem(count() - 1);
    m_hasSeparator = false;
}

int QG_LineTypeBox::appendTemporaryRow(const QString& name, const RS2::LineType type) {
    ensureSeparator();
    const int index = count();
    addItem(tr("%1 (not in this drawing)").arg(name));
    setItemData(index, static_cast<int>(type), LineTypeRole);
    setItemData(index, name, LineTypeNameRole);
    setItemData(index, TemporaryRow, LineTypeRowKindRole);
    setItemData(index, name, Qt::ToolTipRole);
    return index;
}

void QG_LineTypeBox::ensureSeparator() {
    if (!m_hasSeparator) {
        insertSeparator(count());
        m_hasSeparator = true;
    }
}

/**
 * Sets the currently selected linetype item to the given linetype.
 */
void QG_LineTypeBox::setLineType(const RS2::LineType t) {
    RS_DEBUG->print("QG_LineTypeBox::setLineType %d\n", t);
    switch (t) {
        case RS2::LineByLayer: {
            if (m_showByLayer) {
                setCurrentIndex(0 + (m_showUnchanged ? 1 : 0));
            }
            else {
                RS_DEBUG->print(RS_Debug::D_WARNING, "QG_LineTypeBox::setLineType: " "Combobox doesn't support linetype BYLAYER");
            }
            break;
        }
        case RS2::LineByBlock: {
            if (m_showByLayer) {
                setCurrentIndex(1 + (m_showUnchanged ? 1 : 0));
            }
            else {
                RS_DEBUG->print(RS_Debug::D_WARNING, "QG_LineTypeBox::setLineType: " "Combobox doesn't support linetype BYBLOCK");
            }
            break;
        }
        default: {
            if (t >= 0) {
                const int index = findData(t);
                setCurrentIndex(index);
            }
            else {
                RS_DEBUG->print(RS_Debug::D_WARNING, "QG_LineTypeBox::setLineType: " "Combobox doesn't support linetype %d", t);
            }
        }
    }
    slotLineTypeChanged(currentIndex());
}

/**
 * Sets the pixmap showing the linetype of the "By Layer" item.
 *
 * @todo needs an update, but not used currently
 */
void QG_LineTypeBox::setLayerLineType(const RS2::LineType t) {
    if (m_showByLayer) {
        QPixmap pixmap;
        switch (t) {
            case RS2::NoPen:
                pixmap = QPixmap(":linetypes/linetype00.lci");
                break;
            default: case RS2::SolidLine:
                pixmap = QPixmap(":linetypes/linetype01.lci");
                break;
            case RS2::DashLine:
                pixmap = QPixmap(":linetypes/linetype02.lci");
                break;
            case RS2::DotLine:
                pixmap = QPixmap(":linetypes/linetype03.lci");
                break;
            case RS2::DashDotLine:
                pixmap = QPixmap(":linetypes/linetype04.lci");
                break;
            case RS2::DivideLine:
                pixmap = QPixmap(":linetypes/linetype05.lci");
                break;
        }

        setItemIcon(0, QIcon(pixmap));
        setItemText(0, tr("By Layer"));

        // needed for the first time a layer is added:
        slotLineTypeChanged(currentIndex());
    }
}

/**
 * Called when the linetype has changed. This method 
 * sets the current linetype to the value chosen or even
 * offers a dialog to the user that allows him/ her to
 * choose an individual linetype.
 */
void QG_LineTypeBox::slotLineTypeChanged(const int index) {
    RS_DEBUG->print("QG_LineTypeBox::slotLineTypeChanged %d\n", index);
    readRow(index);
    RS_DEBUG->print("QG_LineTypeBox::slotLineTypeChanged %d: done\n", index);

    emit lineTypeChanged(m_currentLineType);
}

/**
 * Reads the given row into the value the combo reports, without emitting: a
 * silent rebuild still has to describe the row it restored.
 */
void QG_LineTypeBox::readRow(const int index) {
    m_unchanged = false;
    if (m_showByLayer) {
        switch (index) {
            case 0:
                if (m_showUnchanged) {
                    m_unchanged = true;
                }
                else {
                    m_currentLineType = RS2::LineByLayer;
                }
                break;

            case 1:
                if (m_showUnchanged) {
                    m_currentLineType = RS2::LineByLayer;
                }
                else {
                    m_currentLineType = RS2::LineByBlock;
                }
                break;

            default:
                m_currentLineType = static_cast<RS2::LineType>(itemData(index).toInt());
        }
    }
    else {
        m_currentLineType = static_cast<RS2::LineType>(itemData(index).toInt());
    }
}
