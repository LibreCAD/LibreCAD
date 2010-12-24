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

void QG_MoveRotateOptions::destroy() {
    RS_SETTINGS->beginGroup("/Modify");
    RS_SETTINGS->writeEntry("/MoveRotate", leAngle->text());
    RS_SETTINGS->endGroup();
}


void QG_MoveRotateOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionModifyMoveRotate) {
        action = (RS_ActionModifyMoveRotate*)a;

        QString sa;
        if (update) {
            sa = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
        } else {
            RS_SETTINGS->beginGroup("/Modify");
            sa = RS_SETTINGS->readEntry("/MoveRotate", "30");
            RS_SETTINGS->endGroup();
            action->setAngle(RS_Math::deg2rad(sa.toDouble()));
        }
        leAngle->setText(sa);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CircleOptions::setAction: wrong action type");
        action = NULL;
    }

}

void QG_MoveRotateOptions::updateAngle(const QString& a) {
    if (action!=NULL) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(a)));
    }
}
