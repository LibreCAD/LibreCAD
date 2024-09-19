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
#include <QComboBox>
#include <QRegularExpression>

#include "qg_dimensionlabeleditor.h"
#include "rs.h"
#include "rs_dimension.h"
#include "rs_debug.h"

namespace {
const QChar g_diametericPrefix{0x2205};
}
/*
 *  Constructs a QG_DimensionLabelEditor as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_DimensionLabelEditor::QG_DimensionLabelEditor(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);

    connect(bDiameter, &QAbstractButton::toggled, this, &QG_DimensionLabelEditor::updatePrefix);

    // Initialize the symbol selection
    cbSymbol->setCurrentIndex(-1);
    connect(cbSymbol, &QComboBox::currentTextChanged, this, &QG_DimensionLabelEditor::insertSign);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DimensionLabelEditor::~QG_DimensionLabelEditor() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DimensionLabelEditor::languageChange()
{
    retranslateUi(this);
}

void QG_DimensionLabelEditor::setLabel(const QString& l) {
    int i0, i1a, i1b, i2;
    QString label, tol1, tol2;
    bool hasDiameter = false;

    label = l;

    if ( !label.isEmpty()) {
        if (label.at(0)==QChar(g_diametericPrefix) || label.at(0)==QChar(0xF8)) {
            hasDiameter = true;
            bDiameter->setChecked(true);
            bDiameter->setText({{QChar(g_diametericPrefix)}});
        } else if (label.startsWith(tr("R", "Radial dimension prefix"))) {
            bDiameter->setIcon({});
            bDiameter->setText(tr("R", "Radial dimesnion prefix"));
            bDiameter->setCheckable(true);
            bDiameter->setChecked(true);
        }
    }

    i0 = l.indexOf("\\S");
    if (i0>=0) {
        i1a = l.indexOf("^ ", i0);
        i1b = i1a+1;
        if (i1a<0) {
            i1a = i1b = l.indexOf('^', i0);
        }
        if (i1a>=0) {
            i2 = l.indexOf(';', i1b);
            label = l.mid(0, i0);
            tol1 = l.mid(i0+2, i1a-i0-2);
            tol2 = l.mid(i1b+1, i2-i1b-1);
        }
    }

    leLabel->setText(label.mid(hasDiameter));
    leTol1->setText(tol1);
    leTol2->setText(tol2);
}

QString QG_DimensionLabelEditor::getLabel() {
    // TODO: an extra '&' shouldn't be added
    // TODO: fix the the root cause
    QString l = leLabel->text();
    if (l.startsWith('&'))
        l = l.mid(1);
    QString prefix = m_hasDiameter ? bDiameter->text() : QString{};
    if (prefix.startsWith('&'))
        prefix = prefix.mid(1);

    QRegularExpression re{QString{R"(^\s*%1)"}.arg(prefix)};
    // diameter:
    if (!bDiameter->text().isEmpty()) {
        auto match = re.match(l);
        if (bDiameter->isChecked()) {
            if (l.isEmpty()) {
                l = QString("%1<>").arg(prefix);
            }
            else {
                if (!match.hasMatch())
                    l = prefix + l;
            }
        } else {
            if (match.hasMatch()) {
                l = l.mid(match.capturedEnd(0));
            } else if (!l.isEmpty() && l.at(0) == g_diametericPrefix) {
                l = l.mid(1);
            }
        }
    }

    if (!leTol1->text().isEmpty() || !leTol2->text().isEmpty()) {
        l += "\\S" + leTol1->text() + "^ " + leTol2->text() + ";";
    }
    return l;
}

void QG_DimensionLabelEditor::insertSign(const QString& s) {
    const QString prefix = s.left(1);
    const QString &current = leLabel->text();
    if (current.isEmpty())
        leLabel->setText(prefix + R"(<>)");
    else if (!current.startsWith(prefix))
        leLabel->setText(prefix + current);
}

void QG_DimensionLabelEditor::updatePrefix(bool isChecked)
{
    QString prefix = bDiameter->text();
    if (prefix.startsWith('&'))
        prefix = prefix.mid(1);
    QRegularExpression re{QString{R"(^\s*%1)"}.arg(prefix)};
    QString label = leLabel->text();
    auto match = re.match(label);
    if (!isChecked && match.hasMatch())
        leLabel->setText(label.mid(match.capturedEnd(0)));
}

void QG_DimensionLabelEditor::setRadialType(const RS_Dimension& dim)
{
    switch(dim.rtti()) {
    case RS2::EntityDimRadial:
        bDiameter->setIcon({});
        bDiameter->setText(tr("R", "Radial dimesnion prefix"));
        bDiameter->setCheckable(true);
        bDiameter->setVisible(true);
	m_hasDiameter = true;
        break;
    case RS2::EntityDimDiametric:
        bDiameter->setIcon({});
        bDiameter->setText({{QChar(g_diametericPrefix)}});
        bDiameter->setCheckable(true);
        bDiameter->setVisible(true);
	m_hasDiameter = true;
        break;
    default:
        bDiameter->setIcon({});
        bDiameter->setText({});
        bDiameter->setChecked(false);
        bDiameter->setVisible(false);
	m_hasDiameter = false;
        break;
    }
}
