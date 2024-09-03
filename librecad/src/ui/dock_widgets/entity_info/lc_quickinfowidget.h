/****************************************************************************
**
* Widget that provides information about entities attributes and collected
* coordinates.

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/
#ifndef LC_QUICKINFOWIDGET_H
#define LC_QUICKINFOWIDGET_H

#include <QWidget>
#include <QDockWidget>
#include "qg_graphicview.h"
#include "rs_units.h"
#include "lc_quickinfopointsdata.h"
#include "lc_quickinfoentitydata.h"
#include "lc_quickinfowidgetoptions.h"

namespace Ui {
class LC_QuickInfoWidget;
}

class LC_QuickInfoWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * current mode of widget
     */
    enum {
        MODE_ENTITY_INFO, // displaying entity information
        MODE_COORDINATE_COLLECTING // collecting and displaying coordinates
    };

    LC_QuickInfoWidget(QWidget *parent, QMap<QString, QAction *> map);

    ~LC_QuickInfoWidget() override;
    void setDocumentAndView(RS_Document *document, QG_GraphicView* view);

    void processEntity(RS_Entity *en);
    void processCoordinate(const RS_Vector& point);
    void endAddingCoordinates();

    void updateCollectedPointsView(bool forceUpdate = false);

    RS_Vector getCollectedCoordinate(int index) {return pointsData.getCollectedCoordinate(index);};
    int getCollectedCoordinatesCount(){return pointsData.getCollectedCoordinatesCount();};

    void setCollectedPointsCoordinateViewMode(int mode);
    void setEntityPointsCoordinateViewMode(int mode);

    void setWidgetMode(int mode);

    bool isDisplayPointsPathOnPreview(){return options->displayPointsPath;};
    bool isSelectEntitiesInDefaultActionWithCTRL(){return options->selectEntitiesInDefaultActionByCTRL;};
    bool isAutoSelectEntitiesInDefaultAction(){return options->autoSelectEntitiesInDefaultAction;};

    void onEntityPropertiesEdited(unsigned long originalId, unsigned long editedCloneId);

protected slots:
    void onSettings();
    void onClearAll();
    void onCopyAll();
    void onTextChanged();
    void onPickEntity();
    void onPickCoordinates();
    void onSelectEntity();
    void onEditEntityProperties();
    void onAnchorClicked(const QUrl &link);
    void onAnchorHighlighted(const QUrl &link);
    void onAnchorUnHighlighted();
    void onCoordinateModeIndexChanged(int index);
    void onViewContextMenu(QPoint pos);
    void onToCmd(int index);
    void onSetRelZero(int index);
    void onRemoveCoordinate(int index);
    void onInsertCoordinates(int index);
    void onRelativeZeroChanged(const RS_Vector& relZero);

private:
    Ui::LC_QuickInfoWidget *ui;
    RS_GraphicView* graphicView = nullptr;
    RS_Document* document = nullptr;

    /**
     * options for widget related functions
     */
    LC_QuickInfoOptions* options = new LC_QuickInfoOptions();

    /**
     * Information for collected points data
     */
    LC_QuickInfoPointsData pointsData = LC_QuickInfoPointsData();

    /*
     * Information for selected entity properties
     */
    LC_QuickInfoEntityData entityData = LC_QuickInfoEntityData();

    /**
     * current widget mode
     */
    int widgetMode = MODE_ENTITY_INFO;

    /**
     * utility flag for that indicates that points is highlighted in preview
     */
    bool hasOwnPreview = false;

    void clearEntityInfo();
    void updateEntityInfoView(bool forceUpdate=false, bool updateView = true);
    RS_Entity* findEntityById(unsigned long entityId) const;
    void drawPreviewPoint(const RS_Vector &vector);
    RS_Vector retrievePositionForModelIndex(int index) const;
    void processURLCommand(const QString &path, int index);
    QString getCoordinateMenuName(const char *command, int idx) const;
    QString retrievePositionStringForModelIndex(int index) const;
    void showNoDataMessage();
    void invokeOptionsDialog();
};

#endif // LC_QUICKINFOWIDGET_H
