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

#include "lc_graphicviewawarewidget.h"
#include "rs.h"
#include "rs_vector.h"

class LC_QuickInfoEntityData;
class LC_QuickInfoPointsData;
class LC_QuickInfoOptions;
class RS_Vector;
class RS_Entity;

namespace Ui
{
    class LC_QuickInfoWidget;
}

class LC_QuickInfoWidget : public LC_GraphicViewAwareWidget
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
    QString getEntityDescription(RS_Entity* en, RS2::EntityDescriptionLevel shortDescription) const ;
    void processCoordinate(const RS_Vector& point);
    void endAddingCoordinates() const;
    void updateCollectedPointsView(bool forceUpdate = false) const;
    RS_Vector getCollectedCoordinate(int index) const;
    int getCollectedCoordinatesCount() const;
    void setCollectedPointsCoordinateViewMode(int mode) const;
    void setEntityPointsCoordinateViewMode(int mode) const;
    void setWidgetMode(int mode);
    bool isDisplayPointsPathOnPreview() const;
    bool isSelectEntitiesInDefaultActionWithCTRL() const;
    bool isAutoSelectEntitiesInDefaultAction() const;
    void onEntityPropertiesEdited(unsigned long originalId, unsigned long editedCloneId);
    void updateFormats() const;
public slots:
    void updateWidgetSettings() const;
protected slots:
    void onSettings();
    void onClearAll() const;
    void onCopyAll() const;
    void onTextChanged() const;
    void onPickEntity() const;
    void onPickCoordinates() const;
    void onSelectEntity() const;
    void onEditEntityProperties();
    void onAnchorClicked(const QUrl &link) const;
    void onAnchorHighlighted(const QUrl &link);
    void onAnchorUnHighlighted();
    void onCoordinateModeIndexChanged(int index) const;
    void onViewContextMenu(QPoint pos);
    void onToCmd(int index) const;
    void onSetRelZero(int index) const;
    void onRemoveCoordinate(int index) const;
    void onInsertCoordinates(int index) const;
    void onRelativeZeroChanged(const RS_Vector& relZero) const;
private:
    Ui::LC_QuickInfoWidget *ui = nullptr;
    RS_GraphicView* m_graphicView = nullptr; // fixme - sand - review dependency
    RS_Document* m_document = nullptr;

    /**
     * options for widget related functions
     */
    std::unique_ptr<LC_QuickInfoOptions> m_options;

    /**
     * Information for collected points data
     */
    std::unique_ptr<LC_QuickInfoPointsData> m_pointsData;

    /*
     * Information for selected entity properties
     */
    std::unique_ptr<LC_QuickInfoEntityData> m_entityData;

    /**
     * current widget mode
     */
    int m_widgetMode = MODE_ENTITY_INFO;

    /**
     * utility flag for that indicates that points is highlighted in preview
     */
    bool m_hasOwnPreview = false;

    void clearEntityInfo() const;
    void updateEntityInfoView(bool forceUpdate=false, bool updateView = true) const;
    RS_Entity* findEntityById(unsigned long entityId) const;
    void drawPreviewPoint(const RS_Vector &vector);
    RS_Vector retrievePositionForModelIndex(int index) const;
    void processURLCommand(const QString &path, int index) const;
    static QString getCoordinateMenuName(QString actionName, int idx);
    QString retrievePositionStringForModelIndex(int index) const;
    void showNoDataMessage() const;
    void invokeOptionsDialog();
};

#endif // LC_QUICKINFOWIDGET_H
