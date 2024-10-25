/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
** Copyright (C) 2015, 2016 ravas (github.com/r-a-v-a-s)
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
#ifndef QG_DLGOPTIONSGENERAL_H
#define QG_DLGOPTIONSGENERAL_H

#include <QDialog>
#include "ui_qg_dlgoptionsgeneral.h"
#include "lc_dialog.h"

class QColor;
class QComboBox;
class QString;

class QG_DlgOptionsGeneral : public LC_Dialog, public Ui::QG_DlgOptionsGeneral{
    Q_OBJECT
public:
    QG_DlgOptionsGeneral(QWidget* parent = nullptr);
    ~QG_DlgOptionsGeneral() override = default;
    static int current_tab;


protected slots:
    void ok();
    void languageChange();
    void setTemplateFile();
    void setLibraryPath();
    void setRestartNeeded();
    void onAutoBackupChanged(int state);
    void on_cbVisualizeHoveringClicked();
    void on_cbPersistentDialogsClicked();
    void on_cbGridExtendAxisLinesToggled();
    void on_cbClassicStatusBarToggled();
    void onCheckNewVersionChanged();
    void onTabCloseButtonChanged();
    void on_tabWidget_currentChanged(int index);
    void on_pb_background_clicked();
    void on_pb_gridPoints_clicked();
    void on_pb_gridLines_clicked();
    void on_pb_metaPoints_clicked();
    void on_pb_metaLines_clicked();
    void on_pb_selected_clicked();
    void on_pb_highlighted_clicked();
    void on_pb_start_clicked();
    void on_pb_handle_clicked();
    void on_pb_end_clicked();
    void on_pb_clear_all_clicked();
    void on_pb_clear_geometry_clicked();
    void on_pb_snap_color_clicked();
    void on_pb_snap_lines_color_clicked();
    void on_pb_relativeZeroColor_clicked();
    void on_pb_previewRefColor_clicked();
    void on_pb_previewRefHighlightColor_clicked();
    void on_rbRelSize_toggled(bool checked);
    void on_pb_axis_X_clicked();
    void on_pb_axis_Y_clicked();
    void setVariableFile();
    void setFontsFolder();
    void setTranslationsFolder();
    void setHatchPatternsFolder();
    void setShortcutsMappingsFoler();
    bool checkRestartNeeded();
    void on_pbOverlayBoxLine_clicked();
    void on_pbOverlayBoxFill_clicked();
    void on_pbOverlayBoxLineInverted_clicked();
    void on_pbOverlayBoxFillInverted_clicked();

    void set_color(QComboBox* combo, QColor custom);
private:
    bool restartNeeded=false;

    QString originalLibraryPath;
    bool originalUseClassicToolbar;
    bool originalAllowsMenusTearOff;

    void init();
    void initComboBox(QComboBox* cb, const QString& text);
    void destroy();
    void initReferencePoints();
    void updateLPtSzUnits();
    void saveReferencePoints();
    QString selectFolder(const char* title);


};

#endif // QG_DLGOPTIONSGENERAL_H
