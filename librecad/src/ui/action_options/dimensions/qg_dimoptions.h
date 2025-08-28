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
#ifndef QG_DIMOPTIONS_H
#define QG_DIMOPTIONS_H

#include<memory>
#include <qcombobox.h>

#include "lc_actionoptionswidgetbase.h"
#include "lc_dimstyleslistmodel.h"
#include "rs_graphic.h"

class RS_ActionInterface;
class RS_ActionDimension;
namespace Ui {
    class Ui_DimOptions;
}

class QG_DimOptions : public LC_ActionOptionsWidgetBase{
    Q_OBJECT
public:
    QG_DimOptions(RS_Graphic *graphic);
    ~QG_DimOptions() override;

    enum UIUpdateMode{
        UI_UPDATE_BASELINE_DISTANCE,
        UI_UPDATE_CIRCLE_ANGLE
    };

public slots:
    void updateLabel();
    void insertSign( const QString & c );
    void languageChange() override;
    void onAngleEditingFinished();
    void onHorClicked();
    void onVerClicked();
    void onBaselineDistanceFreeClicked();
    void onAngleCircleFreeClicked();
    void onBaselineDistanceTextChanged();
    void onAngleCircleTextChanged();
    void onDimStyleChanged(int index);
    void updateUI(int mode) override;
protected:
    RS_ActionDimension* m_action = nullptr;
    std::unique_ptr<Ui::Ui_DimOptions> ui;
    LC_StylesListModel* m_dimItemsListModel = nullptr;
    void initStylesCombobox(RS_Graphic *graphic);
    void updateAngle( const QString& a );
    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;
};

#endif // QG_DIMOPTIONS_H
