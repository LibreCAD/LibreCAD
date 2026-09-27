/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2011 Rallaz, rallazz@gmail.com
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2026 LibreCAD (librecad.org)
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License as published by the Free Software
** Foundation either version 2 of the License, or (at your option)
**  any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
* USA
**
**********************************************************************/

#include "lc_linetypenames.h"

/**
 * Converts a line type name (e.g. "CONTINUOUS") into a RS2::LineType
 * object.
 */
RS2::LineType LC_LineTypeNames::nameToLineType(const QString &name) {

  QString uName = name.toUpper();

  // Standard linetypes for QCad II / AutoCAD
  if (uName.isEmpty() || uName == "BYLAYER") {
    return RS2::LineByLayer;
  }
  if (uName == "BYBLOCK") {
    return RS2::LineByBlock;
  }
  if (uName == "CONTINUOUS" || uName == "ACAD_ISO01W100") {
    return RS2::SolidLine;
  }
  if (uName == "ACAD_ISO07W100" || uName == "DOT") {
    return RS2::DotLine;
  }
  if (uName == "DOTTINY") {
    return RS2::DotLineTiny;
  }
  if (uName == "DOT2") {
    return RS2::DotLine2;
  }
  if (uName == "DOTX2") {
    return RS2::DotLineX2;
  }
  if (uName == "ACAD_ISO02W100" || uName == "ACAD_ISO03W100" ||
      uName == "DASHED") {
    return RS2::DashLine;
  }
  if (uName == "DASHEDTINY") {
    return RS2::DashLineTiny;
  }
  if (uName == "DASHED2") {
    return RS2::DashLine2;
  }
  if (uName == "DASHEDX2") {
    return RS2::DashLineX2;
  }
  if (uName == "HIDDEN") {
    return RS2::HiddenLine;
  }
  if (uName == "HIDDENTINY") {
    return RS2::HiddenLineTiny;
  }
  if (uName == "HIDDEN2") {
    return RS2::HiddenLine2;
  }
  if (uName == "HIDDENX2") {
    return RS2::HiddenLineX2;
  }
  if (uName == "ACAD_ISO10W100" || uName == "DASHDOT") {
    return RS2::DashDotLine;
  }
  if (uName == "DASHDOTTINY") {
    return RS2::DashDotLineTiny;
  }
  if (uName == "DASHDOT2") {
    return RS2::DashDotLine2;
  }
  if (uName == "ACAD_ISO04W100" || uName == "DASHDOTX2") {
    return RS2::DashDotLineX2;
  }
  if (uName == "ACAD_ISO12W100" || uName == "DIVIDE") {
    return RS2::DivideLine;
  }
  if (uName == "DIVIDETINY") {
    return RS2::DivideLineTiny;
  }
  if (uName == "DIVIDE2") {
    return RS2::DivideLine2;
  }
  if (uName == "ACAD_ISO05W100" || uName == "DIVIDEX2") {
    return RS2::DivideLineX2;
  }
  if (uName == "CENTER") {
    return RS2::CenterLine;
  }
  if (uName == "CENTERTINY") {
    return RS2::CenterLineTiny;
  }
  if (uName == "CENTER2") {
    return RS2::CenterLine2;
  }
  if (uName == "CENTERX2") {
    return RS2::CenterLineX2;
  }
  // ISO 128-20 type 09 "long-dashed double-short-dashed" (24,-3,6,-3,6,-3)
  // is PHANTOM's shape at PHANTOM's scale, like the ISO04/ISO05 aliases.
  if (uName == "ACAD_ISO09W100" || uName == "PHANTOM") {
    return RS2::PhantomLine;
  }
  if (uName == "PHANTOMTINY") {
    return RS2::PhantomLineTiny;
  }
  if (uName == "PHANTOM2") {
    return RS2::PhantomLine2;
  }
  if (uName == "PHANTOMX2") {
    return RS2::PhantomLineX2;
  }
  if (uName == "BORDER") {
    return RS2::BorderLine;
  }
  if (uName == "BORDERTINY") {
    return RS2::BorderLineTiny;
  }
  if (uName == "BORDER2") {
    return RS2::BorderLine2;
  }
  if (uName == "BORDERX2") {
    return RS2::BorderLineX2;
  }

  return RS2::SolidLine;
}

/**
 * Converts a RS_LineType into a name for a line type.
 */
QString LC_LineTypeNames::lineTypeToName(RS2::LineType lineType) {
  // Standard linetypes for QCad II / AutoCAD
  switch (lineType) {
  case RS2::SolidLine:
    return "CONTINUOUS";
  case RS2::DotLine:
    return "DOT";
  case RS2::DotLineTiny:
    return "DOTTINY";
  case RS2::DotLine2:
    return "DOT2";
  case RS2::DotLineX2:
    return "DOTX2";
  case RS2::DashLine:
    return "DASHED";
  case RS2::DashLineTiny:
    return "DASHEDTINY";
  case RS2::DashLine2:
    return "DASHED2";
  case RS2::DashLineX2:
    return "DASHEDX2";
  case RS2::HiddenLine:
    return "HIDDEN";
  case RS2::HiddenLineTiny:
    return "HIDDENTINY";
  case RS2::HiddenLine2:
    return "HIDDEN2";
  case RS2::HiddenLineX2:
    return "HIDDENX2";
  case RS2::DashDotLine:
    return "DASHDOT";
  case RS2::DashDotLineTiny:
    return "DASHDOTTINY";
  case RS2::DashDotLine2:
    return "DASHDOT2";
  case RS2::DashDotLineX2:
    return "DASHDOTX2";
  case RS2::DivideLine:
    return "DIVIDE";
  case RS2::DivideLineTiny:
    return "DIVIDETINY";
  case RS2::DivideLine2:
    return "DIVIDE2";
  case RS2::DivideLineX2:
    return "DIVIDEX2";
  case RS2::CenterLine:
    return "CENTER";
  case RS2::CenterLineTiny:
    return "CENTERTINY";
  case RS2::CenterLine2:
    return "CENTER2";
  case RS2::CenterLineX2:
    return "CENTERX2";
  case RS2::PhantomLine:
    return "PHANTOM";
  case RS2::PhantomLineTiny:
    return "PHANTOMTINY";
  case RS2::PhantomLine2:
    return "PHANTOM2";
  case RS2::PhantomLineX2:
    return "PHANTOMX2";
  case RS2::BorderLine:
    return "BORDER";
  case RS2::BorderLineTiny:
    return "BORDERTINY";
  case RS2::BorderLine2:
    return "BORDER2";
  case RS2::BorderLineX2:
    return "BORDERX2";
  case RS2::LineByLayer:
    return "ByLayer";
  case RS2::LineByBlock:
    return "ByBlock";
  default:
    break;
  }
  return "CONTINUOUS";
}

/**
 * Folds a line type name to its identity: NFC, then ASCII-only upper case, so
 * spellings that differ only in ASCII case or Unicode form fold alike. Not
 * nameToLineType()'s QString::toUpper(); the two agree on ASCII names only.
 */
QString LC_LineTypeNames::foldName(const QString &name) {
  // ASCII is NFC-stable, so the normalisation pass is skipped for it.
  bool ascii = true;
  for (const QChar c : name) {
    if (c.unicode() >= 0x80) {
      ascii = false;
      break;
    }
  }

  QString folded =
      ascii ? name : name.normalized(QString::NormalizationForm_C);

  // a-z only. QString::toUpper() folds non-ASCII case and grows "Straße";
  // std::toupper() is locale-dependent and byte-wise.
  for (QChar &c : folded) {
    if (c >= u'a' && c <= u'z') {
      c = QChar(static_cast<char16_t>(c.unicode() - 32));
    }
  }
  return folded;
}

/**
 * Converts a RS_LineType into a name for a line type.
 */
/*QString LC_LineTypeNames::lineTypeToDescription(RS2::LineType lineType) {

    // Standard linetypes for QCad II / AutoCAD
    switch (lineType) {
    case RS2::SolidLine:
        return "Solid line";
    case RS2::DotLine:
        return "ISO Dashed __ __ __ __ __ __ __ __ __ __ _";
    case RS2::DashLine:
        return "ISO Dashed with Distance __    __    __    _";
    case RS2::DashDotLine:
        return "ISO Long Dashed Dotted ____ . ____ . __";
    case RS2::DashDotDotLine:
        return "ISO Long Dashed Double Dotted ____ .. __";
    case RS2::LineByLayer:
        return "";
    case RS2::LineByBlock:
        return "";
    default:
        break;
    }

    return "CONTINUOUS";
}*/
