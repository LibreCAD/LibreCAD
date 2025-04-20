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
#include "qg_dlghatch.h"

#include "lc_graphicviewport.h"
#include "rs_hatch.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_pattern.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
/*
 *  Constructs a QG_DlgHatch as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgHatch::QG_DlgHatch(QWidget *parent, LC_GraphicViewport *pViewport, RS_Hatch* hatch, bool forNew)
    :LC_EntityPropertiesDlg(parent, "HatchProperties", pViewport) {
    setupUi(this);
    init();
    setEntity(hatch, forNew);
}

QG_DlgHatch::~QG_DlgHatch() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgHatch::languageChange(){
    retranslateUi(this);
}

void QG_DlgHatch::init() {
    m_preview = std::make_unique<RS_EntityContainer>();
    gvPreview->setContainer(m_preview.get());
    gvPreview->getViewPort()->setBorders(15,15,15,15);
    gvPreview->initView();
    gvPreview->addScrollbars();
    gvPreview->loadSettings();
//    gvPreview->setHasNoGrid(false);
    cbPattern->init();
}

void QG_DlgHatch::polish() {
    LC_Dialog::ensurePolished();
    gvPreview->zoomAuto();
}

void QG_DlgHatch::showEvent ( QShowEvent * e) {
    LC_Dialog::showEvent(e);
    gvPreview->zoomAuto();
}

void QG_DlgHatch::setEntity(RS_Hatch* h, bool isNew) {
    m_entity = h;
    this->m_isNew = isNew;
    bool enablePrev = LC_GET_ONE_BOOL("Draw","HatchPreview", false);

    cbEnablePreview->setChecked(enablePrev);

    // read defaults from config file:
    if (isNew) {
        LC_GROUP_GUARD("Draw");
        {
            bool solid = LC_GET_BOOL("HatchSolid", false);
            QString pat = LC_GET_STR("HatchPattern", "ANSI31");
            QString scale = LC_GET_STR("HatchScale", "1.0");
            QString angle = LC_GET_STR("HatchAngle", "0.0");
            cbSolid->setChecked(solid);
            setPattern(pat);
            leScale->setText(scale);
            leAngle->setText(angle);
            leHatchArea->setText("");
        }
    }
    // initialize dialog based on given hatch:
    else {
        cbSolid->setChecked(m_entity->isSolid());
        setPattern(m_entity->getPattern());

        toUIValue(m_entity->getScale(), leScale);

        // todo - here we assumed that angle in hatch is always stored in wcs coordinates
        toUIAngleDeg(m_entity->getAngle(),leAngle);

        showArea();
    }
}

void QG_DlgHatch::updateEntity() {
    if (m_entity) {
        m_entity->setSolid(cbSolid->isChecked());
        m_entity->setPattern(cbPattern->currentText());

        m_entity->setScale(toWCSValue(leScale,1.0));
        // here we assume that the user enters angle of the hatch as current ucs basis angle, and it is stored as wcs
        m_entity->setAngle(toWCSAngle(leAngle, 0.0));

        if (!m_isNew) {
            showArea();
        }
        saveSettings();
        m_entity->update();
    }
}

void QG_DlgHatch::showArea(){
    double area = m_entity->getTotalArea();
    if (!RS_Math::equal(area, RS_MAXDOUBLE)) {
        QString number = QString::number(m_entity->getTotalArea(), 'g', 10);
        leHatchArea->setText(number);
    } else {
        leHatchArea->setText({});
    }
}

void QG_DlgHatch::setPattern(const QString& p) {
    if (!RS_PATTERNLIST->contains(p)) {
        cbPattern->addItem(p);
    }
    cbPattern->setCurrentIndex( cbPattern->findText(p) );
    m_pattern = cbPattern->getPattern();
}

void QG_DlgHatch::resizeEvent ( QResizeEvent * ) {
    updatePreview();
}

void QG_DlgHatch::updatePreview() {
    if (m_preview==nullptr) {
        return;
    }
    if (m_entity == nullptr || !cbEnablePreview->isChecked()) {
        m_preview->clear();
        gvPreview->zoomAuto();
        return;
    }
    m_pattern = cbPattern->getPattern();
    if (m_pattern->countDeep()==0)
        return;

    QString patName = cbPattern->currentText();
    bool isSolid = cbSolid->isChecked();
    double scale = toWCSValue(leScale, 1.0);
    double angle = toWCSAngle(leAngle, 0.0);
    double prevSize = 100.0;
    if (m_pattern) {
        m_pattern->calculateBorders();
        prevSize = std::max(prevSize, m_pattern->getSize().magnitude());
    }

    m_preview->clear();

    auto* prevHatch = new RS_Hatch(m_preview.get(),
                                       RS_HatchData(isSolid, scale, angle, patName));
    prevHatch->setPen(m_entity->getPen());

    auto* loop = new RS_EntityContainer(prevHatch);
//    loop->setPen(RS_Pen(RS2::FlagInvalid));
    const RS_Pen &pen = RS_Pen(RS_Color(RS2::FlagByLayer), RS2::WidthByLayer, RS2::LineByLayer);
    loop->setPen(pen);
    addRectangle(pen, {0., 0.}, {prevSize,prevSize}, loop);
    prevHatch->addEntity(loop);
    m_preview->addEntity(prevHatch);
    if (!isSolid) {
        prevHatch->update();
    }

    gvPreview->zoomAuto();
}

void QG_DlgHatch::addRectangle(RS_Pen pen, RS_Vector const &v0, RS_Vector const &v1, RS_EntityContainer* container){
    container->addEntity(new RS_Line{container, v0, {v1.x, v0.y}});
    container->addEntity(new RS_Line{container, {v1.x, v0.y}, v1});
    container->addEntity(new RS_Line{container, v1, {v0.x, v1.y}});
    container->addEntity(new RS_Line{container, {v0.x, v1.y}, v0});
    for (auto e: container->getEntityList()){
        e->setPen(pen);
    }
}

void QG_DlgHatch::saveSettings(){
    if (m_isNew){
        LC_GROUP_GUARD("Draw");
        LC_SET("HatchSolid", cbSolid->isChecked());
        LC_SET("HatchPattern", cbPattern->currentText());
        LC_SET("HatchScale", leScale->text());
        LC_SET("HatchAngle", leAngle->text());
        LC_SET("HatchPreview", cbEnablePreview->isChecked());
    }
}
