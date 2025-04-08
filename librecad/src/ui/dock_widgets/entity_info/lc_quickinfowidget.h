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

#include "lc_graphicviewaware.h"
#include "lc_quickinfoentitydata.h"
#include "lc_quickinfopointsdata.h"
#include "lc_quickinfowidgetoptions.h"
#include "qg_graphicview.h"
#include "rs_units.h"

namespace Ui {
class LC_QuickInfoWidget;
}

class LC_QuickInfoWidget : public QWidget,public LC_GraphicViewAware
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

    void setGraphicView(RS_GraphicView* view) override;

    void processEntity(RS_Entity *en);
    QString getEntityDescription(RS_Entity *en, RS2::EntityDescriptionLevel shortDescription);
    void processCoordinate(const RS_Vector& point);
    void endAddingCoordinates();

    void updateCollectedPointsView(bool forceUpdate = false);

    RS_Vector getCollectedCoordinate(int index) const {
        return m_pointsData.getCollectedCoordinate(index);
    }
    int getCollectedCoordinatesCount() const {
        return m_pointsData.getCollectedCoordinatesCount();
    }

    void setCollectedPointsCoordinateViewMode(int mode);
    void setEntityPointsCoordinateViewMode(int mode);

    void setWidgetMode(int mode);

    bool isDisplayPointsPathOnPreview() const {
        return m_options->displayPointsPath;
    }
    bool isSelectEntitiesInDefaultActionWithCTRL() const {
        return m_options->selectEntitiesInDefaultActionByCTRL;
    }
    bool isAutoSelectEntitiesInDefaultAction() const {
        return m_options->autoSelectEntitiesInDefaultAction;
    }

    void onEntityPropertiesEdited(unsigned long originalId, unsigned long editedCloneId);
    void updateFormats();
public slots:
    void updateWidgetSettings();
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
    Ui::LC_QuickInfoWidget *ui = nullptr;
    RS_GraphicView* m_graphicView = nullptr; // fixme - sand - review dependency
    RS_Document* m_document = nullptr;

    /**
     * options for widget related functions
     */
    LC_QuickInfoOptions* m_options = new LC_QuickInfoOptions();

    /**
     * Information for collected points data
     */
    LC_QuickInfoPointsData m_pointsData = LC_QuickInfoPointsData();

    /*
     * Information for selected entity properties
     */
    LC_QuickInfoEntityData m_entityData = LC_QuickInfoEntityData();

    /**
     * current widget mode
     */
    int m_widgetMode = MODE_ENTITY_INFO;

    /**
     * utility flag for that indicates that points is highlighted in preview
     */
    bool m_hasOwnPreview = false;

    void clearEntityInfo();
    void updateEntityInfoView(bool forceUpdate=false, bool updateView = true);
    RS_Entity* findEntityById(unsigned long entityId) const;
    void drawPreviewPoint(const RS_Vector &vector);
    RS_Vector retrievePositionForModelIndex(int index) const;
    void processURLCommand(const QString &path, int index);
    QString getCoordinateMenuName(QString actionName, int idx) const;
    QString retrievePositionStringForModelIndex(int index) const;
    void showNoDataMessage();
    void invokeOptionsDialog();
};

#endif // LC_QUICKINFOWIDGET_H
