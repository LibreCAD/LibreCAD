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
#ifndef QG_PRINTPREVIEWOPTIONS_H
#define QG_PRINTPREVIEWOPTIONS_H

#include "lc_actionoptionswidgetbase.h"

class RS_ActionInterface;
class RS_ActionPrintPreview;
namespace Ui {
class Ui_PrintPreviewOptions;
}

class QG_PrintPreviewOptions : public LC_ActionOptionsWidgetBase{
    Q_OBJECT

public:
    QG_PrintPreviewOptions();
    ~QG_PrintPreviewOptions() override;

public slots:
    void onCenterClicked();
    void onScaleLineClicked(bool state);
    void onBlackWhiteClicked(bool on);
    void onFitClicked();
    void scale(const QString& s);
    void scaleByFactor(double factor);
    void updateScaleBox();
    void updateScaleBox(double f);
    /** print scale fixed to saved value **/
    void onScaleClicked(bool fixed);
    void calcPagesNum();

    void hideOptions() override;

protected:
    RS_ActionPrintPreview* action = nullptr;
    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;

protected slots:
    void languageChange() override;
private:
    void init();
    void initGuiActions();
    QStringList imperialScales;
    QStringList metricScales;
    bool updateDisabled = false;
    bool scaleLineWidth = false;
    bool blackWhiteDisabled = true;
    int defaultScales = 1;
    std::unique_ptr<Ui::Ui_PrintPreviewOptions> ui;
};

#endif // QG_PRINTPREVIEWOPTIONS_H
