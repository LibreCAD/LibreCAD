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

#include <QCoreApplication>
#include <QEvent>
#include <QFont>
#include <QPainter>
#include <QPalette>
#include <QSignalBlocker>
#include <QStringList>

#include <algorithm>
#include <utility>
#include <vector>

#include "lc_iconengineshared.h"
#include "lc_linetype.h"
#include "lc_linetypelist.h"
#include "lc_linetypenames.h"
#include "rs_debug.h"

namespace {
// Wide enough to read a dash list; list mode only.
const QSize kPreviewIconSize(40, 14);
// The stock .lci pictures use a centred 32x12 area; a preview draws in it too.
const int kPreviewAreaWidth = 32;
const int kStrokeHeight = 3;
const int kMinSegment = 3;

QString dashListText(const std::vector<double>& pattern) {
    QStringList parts;
    for (const double value : pattern) {
        // Ten significant digits keep a long dash out of scientific notation.
        parts << QString::number(value, 'g', 10);
    }
    return parts.join(QStringLiteral(", "));
}

QString dashText(const std::vector<double>& pattern) {
    // A record may only name a line type; say so instead of an empty list.
    return pattern.empty()
        ? QCoreApplication::translate("QG_LineTypeBox", "no dashes")
        : dashListText(pattern);
}

QString tableRowTooltip(const LC_LineType& entry) {
    QStringList parts;
    if (!entry.description.isEmpty()) {
        parts << entry.description;
    }
    parts << dashText(entry.pattern);
    return parts.join(QStringLiteral(" — "));
}

struct LitRun {
    int x;
    int width;
};

/// The lit runs of a pattern across an area measured in whole logical pixels:
/// every dash and gap is at least three pixels wide, and a dot advances like a
/// dash so the gaps on both of its sides survive.
std::vector<LitRun> layOutPattern(const std::vector<double>& pattern, const int areaWidth) {
    std::vector<LitRun> runs;
    const std::vector<double> canonical = LC_LineTypeNames::canonicalDashes(pattern);
    double period = 0.0;
    for (const double value : canonical) {
        period += value;
    }
    if (canonical.empty() || !(period > 0.0)) {
        // Nothing to alternate: one solid bar across the drawing area.
        runs.push_back({0, areaWidth});
        return runs;
    }
    const int elements = static_cast<int>(canonical.size());
    int periods = elements <= 2 ? 3 : (elements <= 4 ? 2 : 1);
    std::vector<int> widths;
    const auto layOut = [&canonical, &widths, areaWidth, period](const int count) {
        widths.clear();
        // From each element's share of the period, which stays within 0..1
        // however small or large the lengths of a record are.
        const double periodWidth = static_cast<double>(areaWidth) / static_cast<double>(count);
        for (const double value : canonical) {
            widths.push_back(std::max(kMinSegment, qRound(value / period * periodWidth)));
        }
    };
    const auto drawnLength = [&widths](const int count) {
        long long sum = 0;
        for (const int width : widths) {
            sum += width;
        }
        return static_cast<long long>(count) * (sum - widths.back());
    };
    layOut(periods);
    while (drawnLength(periods) > areaWidth && periods > 1) {
        --periods;
        layOut(periods);
    }
    // One period still too wide: shave the widest segment above the minimum,
    // then let the right edge clip the rest.
    while (drawnLength(periods) > areaWidth) {
        std::size_t widest = widths.size();
        for (std::size_t i = 0; i + 1 < widths.size(); ++i) {
            if (widths[i] > kMinSegment &&
                (widest == widths.size() || widths[i] > widths[widest])) {
                widest = i;
            }
        }
        if (widest == widths.size()) {
            break;
        }
        --widths[widest];
    }
    int x = 0;
    for (int p = 0; p < periods && x < areaWidth; ++p) {
        for (std::size_t i = 0; i < widths.size() && x < areaWidth; ++i) {
            if (i % 2 == 0) {
                // A run the right edge would cut below the minimum is left
                // out: a sliver reads as another element.
                const int width = std::min(widths[i], areaWidth - x);
                if (width >= kMinSegment) {
                    runs.push_back({x, width});
                }
            }
            x += widths[i];
        }
    }
    return runs;
}

QPixmap layerStockPixmap(const RS2::LineType type) {
    switch (type) {
        case RS2::NoPen:
            return QPixmap(QStringLiteral(":linetypes/linetype00.lci"));
        case RS2::DashLine:
            return QPixmap(QStringLiteral(":linetypes/linetype02.lci"));
        case RS2::DotLine:
            return QPixmap(QStringLiteral(":linetypes/linetype03.lci"));
        case RS2::DashDotLine:
            return QPixmap(QStringLiteral(":linetypes/linetype04.lci"));
        case RS2::DivideLine:
            return QPixmap(QStringLiteral(":linetypes/linetype05.lci"));
        case RS2::SolidLine:
        default:
            return QPixmap(QStringLiteral(":linetypes/linetype01.lci"));
    }
}

QPixmap paintPatternPixmap(const std::vector<double>& pattern, const QColor& color,
                           const QSize& logical, const qreal dpr) {
    QPixmap pixmap(QSize(qRound(logical.width() * dpr), qRound(logical.height() * dpr)));
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(Qt::NoPen);

    // The stock .lci pictures occupy a centred 32x12 area; a preview uses the
    // same whole-pixel area so its dashes start and end where the stock ones do.
    const int areaWidth = std::min(kPreviewAreaWidth, logical.width());
    const int areaX = (logical.width() - areaWidth) / 2;
    const int strokeY = (logical.height() - kStrokeHeight) / 2;

    for (const LitRun& run : layOutPattern(pattern, areaWidth)) {
        painter.fillRect(QRect(areaX + run.x, strokeY, run.width, kStrokeHeight), color);
    }
    return pixmap;
}

QPixmap paintMissingPixmap(const QColor& color, const QSize& logical, const qreal dpr,
                           const QFont& font) {
    QPixmap pixmap(QSize(qRound(logical.width() * dpr), qRound(logical.height() * dpr)));
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(Qt::NoPen);

    const int areaWidth = std::min(kPreviewAreaWidth, logical.width());
    const int areaHeight = std::min(12, logical.height());
    const int areaX = (logical.width() - areaWidth) / 2;
    const int areaRight = areaX + areaWidth;
    const int strokeY = (logical.height() - kStrokeHeight) / 2;

    // A faint bar with a gap in its middle for the question mark.
    QColor faint = color;
    faint.setAlphaF(0.30 * color.alphaF());
    const int middle = areaX + areaWidth / 2;
    const int gapHalf = 5;
    painter.fillRect(QRect(areaX, strokeY, (middle - gapHalf) - areaX, kStrokeHeight), faint);
    painter.fillRect(QRect(middle + gapHalf, strokeY, areaRight - (middle + gapHalf),
                           kStrokeHeight), faint);

    QFont glyphFont = font;
    glyphFont.setPixelSize(std::max(7, static_cast<int>(areaHeight * 0.75)));
    glyphFont.setBold(true);
    painter.setFont(glyphFont);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(color);
    painter.drawText(QRect(middle - gapHalf, 0, 2 * gapHalf, logical.height()),
                     Qt::AlignCenter, QStringLiteral("?"));
    return pixmap;
}

QColor iconColorForMode(const QIcon::Mode mode, const QPalette& palette) {
    const QString key = QString::fromLatin1(LC_SVGIconEngineAPI::KEY_COLOR_MAIN);
    LC_SVGIconEngineAPI::IconMode engineMode = LC_SVGIconEngineAPI::Normal;
    switch (mode) {
        case QIcon::Active:
            engineMode = LC_SVGIconEngineAPI::Active;
            break;
        case QIcon::Selected:
            engineMode = LC_SVGIconEngineAPI::Selected;
            break;
        case QIcon::Disabled:
            engineMode = LC_SVGIconEngineAPI::Disabled;
            break;
        default:
            break;
    }
    QString value = LC_SVGIconEngineAPI::getColorAppProperty(
        key, engineMode, LC_SVGIconEngineAPI::Off);
    if (value.isEmpty()) {
        value = LC_SVGIconEngineAPI::getColorAppProperty(
            key, LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState);
    }
    const QColor color(value);
    return color.isValid() ? color : palette.color(QPalette::Text);
}

/// One picture per icon mode, in the color the stock pictures take for it: the
/// preview of a pattern, or the "no definition" picture for a null pattern.
QIcon modeIconFor(const std::vector<double>* pattern, const QSize& logical, const qreal dpr,
                  const QPalette& palette, const QFont& font) {
    QIcon icon;
    const QIcon::Mode modes[] = {
        QIcon::Normal, QIcon::Active, QIcon::Selected, QIcon::Disabled};
    std::vector<std::pair<QColor, QPixmap>> painted;
    for (const QIcon::Mode mode : modes) {
        const QColor color = iconColorForMode(mode, palette);
        QPixmap pixmap;
        for (const auto& item : painted) {
            if (item.first == color) {
                // Modes that resolve to one color share one painted pixmap.
                pixmap = item.second;
                break;
            }
        }
        if (pixmap.isNull()) {
            pixmap = pattern != nullptr
                ? paintPatternPixmap(*pattern, color, logical, dpr)
                : paintMissingPixmap(color, logical, dpr, font);
            painted.push_back({color, pixmap});
        }
        icon.addPixmap(pixmap, mode, QIcon::Off);
    }
    return icon;
}
}

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
    if (list != nullptr) {
        // Only list mode needs room for a drawn pattern.
        setIconSize(kPreviewIconSize);
        m_listIconSize = true;
    }
    else if (m_listIconSize) {
        // An invalid size hands the icon back to the style, as a combo that
        // never had a list keeps it.
        setIconSize(QSize());
        m_listIconSize = false;
    }
    m_hasSeparator = false;
    // The host paints the By Layer row again after a rebuild.
    m_layerPen.reset();

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
    const int specialRows = (m_showUnchanged ? 1 : 0) + (m_showByLayer ? 2 : 0) +
                            (m_showNoPen ? 1 : 0);
    for (int i = 0; i < specialRows; ++i) {
        setItemData(i, QString(), LineTypeNameRole);
        setItemData(i, SpecialRow, LineTypeRowKindRole);
    }

    if (list == nullptr) {
        m_builtInStart = count();
        addBuiltInRows(nullptr);
        return;
    }

    // The drawing's own line types come before the built-ins, so a user reaches
    // them without scrolling past the stock rows.
    std::vector<const LC_LineType*> rows;
    for (unsigned i = 0; i < list->count(); ++i) {
        const LC_LineType* entry = list->at(i);
        if (entry == nullptr) {
            continue;
        }
        const QString canonical = LC_LineTypeNames::lineTypeToName(
            LC_LineTypeNames::nameToLineType(entry->name));
        // key() is already the fold of the name. A built-in the record redefines
        // is one of the drawing's rows too, beside its stock row.
        const bool ownSpelling = entry->key() != LC_LineTypeNames::foldName(canonical);
        if (!ownSpelling && !LC_LineTypeList::redefinesBuiltIn(*entry)) {
            continue;
        }
        rows.push_back(entry);
    }
    std::sort(rows.begin(), rows.end(),
              [](const LC_LineType* a, const LC_LineType* b) { return a->key() < b->key(); });
    for (const LC_LineType* entry : rows) {
        const int index = count();
        addItem(entry->name);
        setItemData(index, static_cast<int>(LC_LineTypeNames::nameToLineType(entry->name)), LineTypeRole);
        setItemData(index, entry->name, LineTypeNameRole);
        setItemData(index, TableRow, LineTypeRowKindRole);
        setRowEntry(index, *entry);
        const QString tooltip = tableRowTooltip(*entry);
        if (!tooltip.isEmpty()) {
            setItemData(index, tooltip, Qt::ToolTipRole);
        }
    }
    if (!rows.empty()) {
        insertSeparator(count());
        m_hasSeparator = true;
    }
    m_builtInStart = count();
    addBuiltInRows(list);
    applyRowIcons();
}

/**
 * Adds the 33 built-in rows in their present order; a row the drawing
 * redefines carries the drawing's record and a sentence saying so.
 */
void QG_LineTypeBox::addBuiltInRows(const LC_LineTypeList* list) {
    const int start = count();
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

    for (int i = start; i < count(); ++i) {
        const auto type = static_cast<RS2::LineType>(itemData(i).toInt());
        QString name;
        if (list != nullptr) {
            const QString canonical = LC_LineTypeNames::lineTypeToName(type);
            const LC_LineType* entry = list->find(canonical);
            if (entry != nullptr && LC_LineTypeList::redefinesBuiltIn(*entry)) {
                name = canonical;
                setRowEntry(i, *entry);
                setItemData(i, tr("As this drawing defines it: %1").arg(dashText(entry->pattern)),
                            Qt::ToolTipRole);
            }
        }
        setItemData(i, name, LineTypeNameRole);
        setItemData(i, BuiltInRow, LineTypeRowKindRole);
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
    const int previousKind = previous >= 0
        ? itemData(previous, LineTypeRowKindRole).toInt()
        : 0;
    // Only a selection that still sits on the source row keeps that source; a
    // pen the user replaced must not come back.
    const bool wasSource = m_hasSource && previous >= 0 && previous == m_sourceRowIndex;
    m_hasList = list != nullptr;
    fillRows(list);
    int index = -1;
    if (previousKind == BuiltInRow) {
        // A built-in row stays the stock row of its enum, even when the new
        // list also redefines that built-in as one of the drawing's rows.
        index = indexForEnum(selectedType);
    }
    else if (!selectedName.isEmpty() && m_hasList) {
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
    int index = -1;
    {
        // The temporary row and its separator come and go: currentIndexChanged
        // would otherwise report the transient indices in between.
        const QSignalBlocker blocker(this);
        if (m_hasList) {
            removeTemporaryRow();
        }
        if (m_hasList && pen.hasLineTypeName()) {
            const QString name = pen.getLineTypeName();
            index = indexForRowOfName(name, pen.getLineType());
            if (index < 0 && !isCanonicalName(name, pen.getLineType())) {
                index = appendTemporaryRow(name, pen.getLineType());
            }
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
            break;
    }
    // A drawing row carries its name's enum but does not stand for that
    // built-in, so only the special and built-in rows may answer.
    const int data = static_cast<int>(type);
    for (int i = 0; i < count(); ++i) {
        const int kind = itemData(i, LineTypeRowKindRole).toInt();
        if ((kind == SpecialRow || kind == BuiltInRow) && itemData(i).toInt() == data) {
            return i;
        }
    }
    return -1;
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
    for (int i = 0; i < count(); ++i) {
        if (itemData(i, LineTypeRowKindRole).toInt() == TemporaryRow) {
            removeItem(i);
            if (i < m_builtInStart) {
                --m_builtInStart;
            }
            break;
        }
    }
    if (!m_hasSeparator) {
        return;
    }
    // A separator with no drawing row left on either side goes away.
    for (int i = 0; i < count(); ++i) {
        const int kind = itemData(i, LineTypeRowKindRole).toInt();
        if (kind == TableRow || kind == TemporaryRow) {
            return;
        }
    }
    removeItem(m_builtInStart - 1);
    --m_builtInStart;
    m_hasSeparator = false;
}

int QG_LineTypeBox::appendTemporaryRow(const QString& name, const RS2::LineType type) {
    // The temporary row is the last of the drawing's rows, before the separator.
    const int index = m_hasSeparator ? m_builtInStart - 1 : m_builtInStart;
    insertItem(index, missingIcon(), tr("%1 (not in this drawing)").arg(name));
    setItemData(index, static_cast<int>(type), LineTypeRole);
    setItemData(index, name, LineTypeNameRole);
    setItemData(index, TemporaryRow, LineTypeRowKindRole);
    setItemData(index, tr("This drawing has no definition for %1").arg(name), Qt::ToolTipRole);
    ++m_builtInStart;
    if (!m_hasSeparator) {
        insertSeparator(index + 1);
        m_hasSeparator = true;
        ++m_builtInStart;
    }
    return index;
}

void QG_LineTypeBox::setRowEntry(const int index, const LC_LineType& entry) {
    // The combo keeps its own copy: no pointer into the list outlives a rebuild.
    setItemData(index, entry.description, LineTypeDescriptionRole);
    QVariantList pattern;
    pattern.reserve(static_cast<qsizetype>(entry.pattern.size()));
    for (const double value : entry.pattern) {
        pattern.append(value);
    }
    setItemData(index, pattern, LineTypePatternRole);
}

std::vector<double> QG_LineTypeBox::rowPattern(const int index) const {
    std::vector<double> result;
    const QVariantList values = itemData(index, LineTypePatternRole).toList();
    result.reserve(static_cast<std::size_t>(values.size()));
    for (const QVariant& value : values) {
        result.push_back(value.toDouble());
    }
    return result;
}

void QG_LineTypeBox::applyRowIcons() {
    for (int i = 0; i < count(); ++i) {
        const int kind = itemData(i, LineTypeRowKindRole).toInt();
        const bool namedRow = kind == BuiltInRow &&
                              !itemData(i, LineTypeNameRole).toString().isEmpty();
        if (kind == TableRow || namedRow) {
            setItemIcon(i, previewIcon(rowPattern(i)));
        }
        else if (kind == TemporaryRow) {
            setItemIcon(i, missingIcon());
        }
    }
}

/**
 * The main color the icon engine gives a stock picture in this mode: the mode's
 * own property, else the any-mode one. With neither set to a color, as in an
 * application that never loaded the icon options, the palette's text color.
 */

/**
 * One picture per icon mode, in the color the stock pictures take for it: the
 * preview of a pattern, or the "no definition" picture for a null pattern.
 */
QIcon QG_LineTypeBox::modeIcon(const std::vector<double>* pattern) const {
    QSize logical = iconSize();
    if (!logical.isValid() || logical.width() <= 0 || logical.height() <= 0) {
        logical = kPreviewIconSize;
    }
    return modeIconFor(pattern, logical, devicePixelRatioF(), palette(), font());
}

QIcon QG_LineTypeBox::previewIcon(const std::vector<double>& pattern) const {
    return modeIcon(&pattern);
}

QIcon QG_LineTypeBox::missingIcon() const {
    return modeIcon(nullptr);
}



void QG_LineTypeBox::changeEvent(QEvent* event) {
    QComboBox::changeEvent(event);
    bool repaint = false;
    switch (event->type()) {
        case QEvent::PaletteChange:
        case QEvent::StyleChange:
        case QEvent::FontChange:
            repaint = true;
            break;
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        case QEvent::DevicePixelRatioChange:
            repaint = true;
            break;
#endif
        default:
            break;
    }
    if (!repaint) {
        return;
    }
    applyRowIcons();
    // The By Layer row holds a copy, not a live icon, so it needs the picture
    // again as well.
    if (m_layerPen.has_value()) {
        paintLayerRow(*m_layerPen);
    }
}

std::optional<LC_LineType> QG_LineTypeBox::definitionOf(const RS_Pen& pen) const {
    if (!m_hasList || !pen.hasLineTypeName()) {
        return std::nullopt;
    }
    // The trimmed, folded key finds a record however its name is spelled,
    // including the blanks a record may keep around it.
    const int index = indexForName(pen.getLineTypeName());
    if (index < 0) {
        return std::nullopt;
    }
    const int kind = itemData(index, LineTypeRowKindRole).toInt();
    if (kind != TableRow && kind != BuiltInRow) {
        return std::nullopt;
    }
    const std::vector<double> pattern = rowPattern(index);
    if (pattern.empty()) {
        // A record that only names a line type defines nothing.
        return std::nullopt;
    }
    LC_LineType entry(itemData(index, LineTypeNameRole).toString());
    entry.description = itemData(index, LineTypeDescriptionRole).toString();
    entry.pattern = pattern;
    return entry;
}

/// Copies the picture of the row a layer pen selects onto the By Layer row.
void QG_LineTypeBox::paintLayerRow(const RS_Pen& layerPen) {
    const int byLayerRow = m_showUnchanged ? 1 : 0;
    int row = -1;
    if (m_hasList && layerPen.hasLineTypeName()) {
        row = indexForRowOfName(layerPen.getLineTypeName(), layerPen.getLineType());
        if (row < 0 && !isCanonicalName(layerPen.getLineTypeName(), layerPen.getLineType())) {
            // The drawing has no definition for the name: say so, as the
            // temporary row of a pen of that name does.
            setItemIcon(byLayerRow, missingIcon());
            setItemText(byLayerRow, tr("By Layer"));
            return;
        }
    }
    else {
        row = indexForEnum(layerPen.getLineType());
    }
    if (row < 0) {
        setItemIcon(byLayerRow, QIcon(layerStockPixmap(layerPen.getLineType())));
    }
    else {
        setItemIcon(byLayerRow, itemIcon(row));
    }
    setItemText(byLayerRow, tr("By Layer"));
}

void QG_LineTypeBox::setLayerLineTypeOf(const RS_Pen& layerPen) {
    if (!m_showByLayer) {
        setLayerLineType(layerPen.getLineType());
        return;
    }
    // Only a combo with the row has a copy to repaint later.
    m_layerPen = layerPen;
    paintLayerRow(layerPen);
    // needed for the first time a layer is added:
    slotLineTypeChanged(currentIndex());
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
                const int index = indexForEnum(t);
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
