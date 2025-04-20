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
#include <QObject>
#include "lc_peninforegistry.h"

#include "rs_color.h"

/**
 * internal value used for colors storing in pen palette storage to denote "unchanged" color value
 */
static const int UNCHANGED_COLOR_INTERNAL_VALUE = -3;

LC_PenInfoRegistry::LC_PenInfoRegistry()= default;

LC_PenInfoRegistry::~LC_PenInfoRegistry(){
    m_lineTypesIconMap.clear();
    m_lineTypesNamesMap.clear();
    m_lineWidthIconMap.clear();
    m_lineWidthNamesMap.clear();
    m_colorNamesMap.clear();
    m_standardLCColorNamesMap.clear();
}

/**
 * Returns display name for linetype
 * @param lineType linetype
 * @return string for UI
 */
QIcon LC_PenInfoRegistry::getLineTypeIcon(RS2::LineType lineType) const{
    QIcon result = m_lineTypesIconMap[lineType];
    return result;
}

/**
 * Returns display name for line width
 * @param lineType line width
 * @return string for UI
 */
QIcon LC_PenInfoRegistry::getLineWidthIcon(RS2::LineWidth lineWidth) const{
    QIcon result = m_lineWidthIconMap[lineWidth];
    return result;
}

/**
 * Returns name of linetype as it is shown in UI
 * @param lineType  line type
 * @return name to display
 */
QString LC_PenInfoRegistry::getLineTypeText(RS2::LineType lineType) const{
    QString result = m_lineTypesNamesMap[lineType];
    return result;
}

/**
 * Returns line width name for UI
 * @param lineWidth line width
 * @return  name to display
 */
QString LC_PenInfoRegistry::getLineWidthText(RS2::LineWidth lineWidth) const{
    QString result = m_lineWidthNamesMap[lineWidth];
    return result;
}

/**
 * Returns singletone instance of the registry
 * @return instance
 */
LC_PenInfoRegistry* LC_PenInfoRegistry::instance(){
    static LC_PenInfoRegistry* uniqueInstance;
    if (uniqueInstance == nullptr){
        uniqueInstance = new LC_PenInfoRegistry{};
        uniqueInstance->registerLineTypes();
        uniqueInstance->registerLineWidths();
        uniqueInstance->registerColorNames();
    }
    return uniqueInstance;
}

/**
 * Returns icon that should be shown for provided color. For predefined colors - returns cached items,
 * for other colors - creates icon with specified dimensions filled by provided color
 *
 * @param color color
 * @param iconSizeW icon width
 * @param iconSizeH icon height
 * @return icon for color
 */
QIcon LC_PenInfoRegistry::getColorIcon(const RS_Color &color, int iconSizeW, int iconSizeH) const{
    if (color.isByLayer()){
        return m_iconByLayer;
    }
    else if (color.isByBlock()){
        return m_iconByBlock;
    }
    else if (isUnchangedColor(color)){
        return m_iconUnchanged;
    }
    if (color.isValid()){
        QPixmap pixmap(iconSizeW, iconSizeH);
        pixmap.fill(color);
        return pixmap;
    }
    return {};
}

/**
 * Utility method that checks whether value of color is unchanged.
 * Unchanged color has flag RS2::FlagFrozen set.
 *
 * Todo - actually, this is good candidate for RS_Color itself,
 * however, not sure whether adding support of such flag on global layer
 * outside of pen palette widget will not break something...
 *
 * @param color color to test
 * @return  true if this is "unchanged" color
 */
bool LC_PenInfoRegistry::isUnchangedColor(const RS_Color &color){
    return color.getFlag(RS2::FlagFrozen);
}

/**
 * Creates color that used to mark "unchanged" color value
 * Internally, relies on RS2::FlagFrozen flag set
 * @return color
 */
RS_Color LC_PenInfoRegistry::createUnchangedColor(){
    return RS_Color(RS2::FlagFrozen);
}

/**
 * Re-creates color from internal string representation used for storing colors.
 * String represents just int value, with special meaning for "unchanged" color.
 * Internally, relies on "fromInColor" method of RS_Color
 * @param str string to parse
 * @return created color (will be invalid if not parsed properly)
 */
RS_Color LC_PenInfoRegistry::getColorFromInternalString(QString &str){
    bool conversionOk = false;
    int intColor = str.toInt(&conversionOk);
    RS_Color result;
    if (conversionOk){
        // were able to parse int value
        if (intColor == UNCHANGED_COLOR_INTERNAL_VALUE){
            // special handling of "unchanged"
            result = createUnchangedColor();
            return result;
        } else {
            // just rely on method from color
            result.fromIntColor(intColor);
            return result;
        }
    }    
    return result;
}

/**
 * Returns string representation of given color for further storing in persistence.
 * String contains int value of the color
 * @param color provided color
 * @return string representation
 */
QString LC_PenInfoRegistry::getInternalColorString(const RS_Color &color){
    int colorInt;
    if (isUnchangedColor(color)){
        colorInt = UNCHANGED_COLOR_INTERNAL_VALUE;
    }
    else{
        colorInt = color.toIntColor();
    }
    QString result =  QString::number(colorInt);
    return result;
}

/**
 * Returns name of the color that should be shown in UI.
 * Based on provided mode, name may represent:
 * HEX value, RGB value of color or mix (natural QT color name, if matched, or HEX)
 * For special color values (like "By Layer") - appropriate string is returned.
 * @param color given color
 * @param colorNameMode specifies how name of the color should be presented
 * @return name of color as it will be used in UI
 */
QString LC_PenInfoRegistry::getColorName(const RS_Color &color, int colorNameMode) const{
    if (color.isByLayer()){
        return QObject::tr("By Layer");
    }
    else if (color.isByBlock()){
        return QObject::tr("By Block");
    }
    else if (isUnchangedColor(color)){
        return QObject::tr("- Unchanged -");
    }
    switch (colorNameMode) {
        case LC_PenInfoRegistry::HEX:
            return color.name();
        case LC_PenInfoRegistry::RGB: {
            QString result = QString("rgb(%1,%2,%3)").arg(color.red(), 1, 10).arg(color.green(), 1, 10).arg(color.blue(), 1, 10);
            return result;
        }
        case LC_PenInfoRegistry::NATURAL: {
            // check where this is standard color first

            const QRgba64 &key = color.rgba64();
            if (m_standardLCColorNamesMap.contains(key)){
               return m_standardLCColorNamesMap.value(key, "");
            }
            else{
                // try to find qt name, if no such name - give up and return hex color name
                return m_colorNamesMap.value(key, color.name());
            }
        }
        default:
            break;
    }
    return QString();
}

/**
 * Method checks whether provided int correspond to one of valid registered line types
 * @param typeCandidate potential line type
 * @return true if this is valid line type value
 */
bool LC_PenInfoRegistry::hasLineType(int typeCandidate){
    RS2::LineType lineType = static_cast<RS2::LineType>(typeCandidate);
    return m_lineTypesNamesMap.contains(lineType);
}

/**
 * Method checks whether provided int correspond to one of valid registered line width values
 * @param typeCandidate potential line width
 * @return true if this is valid line width value
 */
bool LC_PenInfoRegistry::hasLineWidth(int widthCandidate){
    RS2::LineWidth lineWidth = static_cast<RS2::LineWidth>(widthCandidate);
    return m_lineWidthNamesMap.contains(lineWidth);
}

/**
 * Registration of all line widths
 */
void LC_PenInfoRegistry::registerLineWidths(){
    doRegisterLineWidth(":/icons/point_cross.lci", tr("- Unchanged -"), RS2::LineWidth::WidthUnchanged);
//    registerLineWidth(":linetypes/width00.png", "- Unchanged -", RS2::LineWidth::WidthUnchanged);
    doRegisterLineWidth(":/icons/point_blank_square.lci", tr("By Layer"), RS2::LineWidth::WidthByLayer);
//    registerLineWidth(":linetypes/width00.png", "By Layer", RS2::LineWidth::WidthByLayer);
    doRegisterLineWidth(":/icons/point_plus_square.lci", tr("By Block"), RS2::LineWidth::WidthByBlock);
//    registerLineWidth(":linetypes/width00.png", "By Block", RS2::LineWidth::WidthByBlock);

    doRegisterLineWidth(":linetypes/width01.lci", tr("Default"), RS2::WidthDefault);
    doRegisterLineWidth(":linetypes/width01.lci", tr("0.00mm"), RS2::Width00);
    doRegisterLineWidth(":linetypes/width01.lci", tr("0.05mm"), RS2::Width01);
    doRegisterLineWidth(":linetypes/width01.lci", tr("0.09mm"), RS2::Width02);
    doRegisterLineWidth(":linetypes/width01.lci", tr("0.13mm (ISO)"), RS2::Width03);
    doRegisterLineWidth(":linetypes/width01.lci", tr("0.15mm"), RS2::Width04);
    doRegisterLineWidth(":linetypes/width01.lci", tr("0.18mm (ISO)"), RS2::Width05);
    doRegisterLineWidth(":linetypes/width01.lci", tr("0.20mm"), RS2::Width06);
    doRegisterLineWidth(":linetypes/width01.lci", tr("0.25mm (ISO)"), RS2::Width07);
    doRegisterLineWidth(":linetypes/width01.lci", tr("0.30mm"), RS2::Width08);
    doRegisterLineWidth(":linetypes/width03.lci", tr("0.35mm (ISO)"), RS2::Width09);
    doRegisterLineWidth(":linetypes/width03.lci", tr("0.40mm"), RS2::Width10);
    doRegisterLineWidth(":linetypes/width04.lci", tr("0.50mm (ISO)"), RS2::Width11);
    doRegisterLineWidth(":linetypes/width05.lci", tr("0.53mm"), RS2::Width12);
    doRegisterLineWidth(":linetypes/width05.lci", tr("0.60mm"), RS2::Width13);
    doRegisterLineWidth(":linetypes/width06.lci", tr("0.70mm (ISO)"), RS2::Width14);
    doRegisterLineWidth(":linetypes/width07.lci", tr("0.80mm"), RS2::Width15);
    doRegisterLineWidth(":linetypes/width08.lci", tr("0.90mm"), RS2::Width16);
    doRegisterLineWidth(":linetypes/width09.lci", tr("1.00mm (ISO)"), RS2::Width17);
    doRegisterLineWidth(":linetypes/width10.lci", tr("1.06mm"), RS2::Width18);
    doRegisterLineWidth(":linetypes/width10.lci", tr("1.20mm"), RS2::Width19);
    doRegisterLineWidth(":linetypes/width12.lci", tr("1.40mm (ISO)"), RS2::Width20);
    doRegisterLineWidth(":linetypes/width12.lci", tr("1.58mm"), RS2::Width21);
    doRegisterLineWidth(":linetypes/width12.lci", tr("2.00mm (ISO)"), RS2::Width22);
    doRegisterLineWidth(":linetypes/width12.lci", tr("2.11mm"), RS2::Width23);
}

/**
 * Registration of all line types
 */
void LC_PenInfoRegistry::registerLineTypes(){
    doRegisterLineType(":/icons/point_cross.lci", tr("- Unchanged -"), RS2::LineTypeUnchanged);
//    registerLineType(":ui/linetype00.png", "- Unchanged -", RS2::LineTypeUnchanged);
    doRegisterLineType(":/icons/point_blank_square.lci", tr("By Layer"), RS2::LineByLayer);
//    registerLineType(":ui/linetype00.png", "By Layer", RS2::LineByLayer);
    doRegisterLineType(":/icons/point_plus_square.lci", tr("By Block"), RS2::LineByBlock);
//    registerLineType(":ui/linetype00.png", "By Block", RS2::LineByBlock);
    doRegisterLineType(":linetypes/linetype00.lci", tr("No Pen"), RS2::NoPen);
    doRegisterLineType(":linetypes/linetype01.lci", tr("Continuous"), RS2::SolidLine);
    doRegisterLineType(":linetypes/linetype02.lci", tr("Dot"), RS2::DotLine);
    doRegisterLineType(":linetypes/linetype02.lci", tr("Dot (tiny)"), RS2::DotLineTiny);
    doRegisterLineType(":linetypes/linetype02.lci", tr("Dot (small)"), RS2::DotLine2);
    doRegisterLineType(":linetypes/linetype02.lci", tr("Dot (large)"), RS2::DotLineX2);
    doRegisterLineType(":linetypes/linetype03.lci", tr("Dash"), RS2::DashLine);
    doRegisterLineType(":linetypes/linetype03.lci", tr("Dash (tiny)"), RS2::DashLineTiny);
    doRegisterLineType(":linetypes/linetype03.lci", tr("Dash (small)"), RS2::DashLine2);
    doRegisterLineType(":linetypes/linetype03.lci", tr("Dash (large)"), RS2::DashLineX2);
    doRegisterLineType(":linetypes/linetype04.lci", tr("Dash Dot"), RS2::DashDotLine);
    doRegisterLineType(":linetypes/linetype04.lci", tr("Dash Dot (tiny)"), RS2::DashDotLineTiny);
    doRegisterLineType(":linetypes/linetype04.lci", tr("Dash Dot (small)"), RS2::DashDotLine2);
    doRegisterLineType(":linetypes/linetype04.lci", tr("Dash Dot (large)"), RS2::DashDotLineX2);
    doRegisterLineType(":linetypes/linetype05.lci", tr("Divide"), RS2::DivideLine);
    doRegisterLineType(":linetypes/linetype05.lci", tr("Divide (tiny)"), RS2::DivideLineTiny);
    doRegisterLineType(":linetypes/linetype05.lci", tr("Divide (small)"), RS2::DivideLine2);
    doRegisterLineType(":linetypes/linetype05.lci", tr("Divide (large)"), RS2::DivideLineX2);
    doRegisterLineType(":linetypes/linetype06.lci", tr("Center"), RS2::CenterLine);
    doRegisterLineType(":linetypes/linetype06.lci", tr("Center (tiny)"), RS2::CenterLineTiny);
    doRegisterLineType(":linetypes/linetype06.lci", tr("Center (small)"), RS2::CenterLine2);
    doRegisterLineType(":linetypes/linetype06.lci", tr("Center (large)"), RS2::CenterLineX2);
    doRegisterLineType(":linetypes/linetype07.lci", tr("Border"), RS2::BorderLine);
    doRegisterLineType(":linetypes/linetype07.lci", tr("Border (tiny)"), RS2::BorderLineTiny);
    doRegisterLineType(":linetypes/linetype07.lci", tr("Border (small)"), RS2::BorderLine2);
    doRegisterLineType(":linetypes/linetype07.lci", tr("Border (large)"), RS2::BorderLineX2);
}

/**
 * Here we just iterate over QT colors names and store them in the map
 * Also, we register names for standard colors as they are used in LibreCAD
 */
void LC_PenInfoRegistry::registerColorNames(){
    // register QT colors
    const QStringList &colorNamesList = QColor::colorNames();
    int count = colorNamesList.count();
    for (int i=0; i < count; i++){
        QString colorName = colorNamesList.at(i);
        QColor color(colorName);
        // todo - basically, localization of QT color names is needed for this mapping
        m_colorNamesMap.insert(color.rgba64(), QObject::tr(colorName.toStdString().c_str()));
    }

    // register standard LibreCAD Colors
    registerLCColor(Qt::red,tr("Red"));
    registerLCColor(Qt::darkRed, tr("Dark Red"));
    registerLCColor(Qt::yellow, tr("Yellow"));
    registerLCColor(Qt::darkYellow, tr("Dark Yellow"));
    registerLCColor(Qt::green, tr("Green"));
    registerLCColor(Qt::darkGreen, tr("Dark Green"));
    registerLCColor(Qt::cyan, tr("Cyan"));
    registerLCColor(Qt::darkCyan, tr("Dark Cyan"));
    registerLCColor(Qt::blue, tr("Blue"));
    registerLCColor(Qt::darkBlue, tr("Dark Blue"));
    registerLCColor(Qt::magenta, tr("Magenta"));
    registerLCColor(Qt::darkMagenta, tr("Dark Magenta"));
    registerLCColor(Qt::black, tr("Black / White"));
    registerLCColor(Qt::gray,tr("Gray"));
    registerLCColor(Qt::darkGray,tr("Dark Gray"));
    registerLCColor(Qt::lightGray,tr("Light Gray"));

}

void LC_PenInfoRegistry::registerLCColor(QColor color, const QString colorName){
    m_standardLCColorNamesMap.insert(color.rgba64(), colorName);
}

/**
 * Internal method that performs registration of line type information and stores one in the map
 * @param iconName name of icon for linetype
 * @param labelKey display name of linetype
 * @param lineType line type itself
 */

void LC_PenInfoRegistry::doRegisterLineType(const char* iconName, const QString labelKey, RS2::LineType lineType){
    QIcon icon = QIcon(iconName);
    m_lineTypesIconMap.insert(lineType, icon);
    m_lineTypesNamesMap.insert(lineType, labelKey);
}

/**
 * Internal method for registration of line width
 * @param iconName icon for line width
 * @param labelKey  label to display in UI
 * @param lineWidth line width
 */
void LC_PenInfoRegistry::doRegisterLineWidth(const char* iconName, const QString labelKey, RS2::LineWidth lineWidth){
    QIcon icon = QIcon(iconName);
    m_lineWidthIconMap.insert(lineWidth, icon);
    m_lineWidthNamesMap.insert(lineWidth, labelKey);
}
