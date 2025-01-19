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
#include <QClipboard>
#include <QDockWidget>
#include <QListView>
#include <QMenu>
#include <QStandardItemModel>
#if defined(Q_OS_LINUX)
#include <QThread>
#endif
#include <QUrl>

#include "lc_flexlayout.h"
#include "lc_quickinfowidget.h"
#include "lc_quickinfowidgetoptionsdialog.h"
#include "rs_dialogfactory.h"
#include "rs_math.h"
#include "rs_point.h"
#include "rs_settings.h"
#include "ui_lc_quickinfowidget.h"

// todo - discover generic way for reliable refresh of entity info widget if entity editing properties/attributes is performed outside of outside of widget
// (via normal editing actions, mouse operations or custom actions)

#define DEBUG_QUICK_INFO_RAW_NO

LC_QuickInfoWidget::LC_QuickInfoWidget(QWidget *parent, QMap<QString, QAction *> map):
    QWidget(parent),
    ui(new Ui::LC_QuickInfoWidget)
{
    ui->setupUi(this);

    // support flexible layout for buttons and small size displays
    auto *layButtonsFlex = new LC_FlexLayout(0, 5, 5);
    layButtonsFlex->fillFromLayout(ui->layToFlexible);
    int buttonsPosition = ui->horizontalLayout->indexOf(ui->layToFlexible);
    QLayoutItem *pItem = ui->horizontalLayout->takeAt(buttonsPosition);
    delete pItem;

    ui->horizontalLayout->insertLayout(0,layButtonsFlex, 5);
    layButtonsFlex->setSoftBreakItems({6});
    ui->horizontalLayout_2->setAlignment(ui->tbSettings,Qt::AlignTop);

    // signals setup
    connect(ui->tbClear, &QToolButton::clicked, this, &LC_QuickInfoWidget::onClearAll);
    connect(ui->tbSettings, &QToolButton::clicked, this, &LC_QuickInfoWidget::onSettings);
    connect(ui->tbCopy, &QToolButton::clicked, this, &LC_QuickInfoWidget::onCopyAll);
    connect(ui->tbFind, &QToolButton::clicked, this, &LC_QuickInfoWidget::onSelectEntity);
    connect(ui->tbEditProperties, &QToolButton::clicked, this, &LC_QuickInfoWidget::onEditEntityProperties);
    connect(ui->pteInfo, &QTextEdit::textChanged, this, &LC_QuickInfoWidget::onTextChanged);
    connect(ui->pteInfo, &QTextBrowser::anchorClicked, this, &LC_QuickInfoWidget::onAnchorClicked);
    connect(ui->pteInfo, &LC_PlainTextEdit::highlighted, this, &LC_QuickInfoWidget::onAnchorHighlighted);
    connect(ui->pteInfo, &LC_PlainTextEdit::unhighlighted, this, &LC_QuickInfoWidget::onAnchorUnHighlighted);
    connect(ui->pteInfo, &LC_PlainTextEdit::customContextMenuRequested, this, &LC_QuickInfoWidget::onViewContextMenu);

    // text editor setup
    ui->pteInfo->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->cbPointsCoordinatesMode, &QComboBox::currentIndexChanged, this, &LC_QuickInfoWidget::onCoordinateModeIndexChanged);
    ui->pteInfo->setOpenLinks(false);
    ui->pteInfo->document()->setDefaultStyleSheet("a {text-decoration: none;} body {background-color: white;}");

    // raw content control useful for debugging, but not needed in live mode
#ifdef DEBUG_QUICK_INFO_RAW
    ui->pteInfo1->setVisible(true);
#else
    ui->pteInfo1->setVisible(false);
#endif
    // support of invocation of actions via widget toolbar buttons, so assigning actions to controls
    QAction* entityInfoAction = map["EntityInfo"];

    if (entityInfoAction != nullptr){
        ui->tbSelectEntity->setDefaultAction(entityInfoAction);
    }
    else{
        ui->tbSelectEntity->setVisible(false);
    }
    QAction* pickCoordinatesAction = map["PickCoordinates"];
    if (pickCoordinatesAction != nullptr){
        ui->tbCollectCoords->setDefaultAction(pickCoordinatesAction);
    }
    else{
        ui->tbCollectCoords->setVisible(false);
    }

    // loading options

    options->load();
    entityData.setOptions(options);

    options->displayEntityID = LC_GET_ONE_BOOL("Appearance","ShowEntityIDs", false);

    LC_GROUP_GUARD("QuickInfoWidget");
    {
        entityData.setCoordinatesMode(LC_GET_INT("EntityCoordinatesMode", LC_QuickInfoBaseData::COORD_ABSOLUTE));
        pointsData.setCoordinatesMode(LC_GET_INT("PointsCoordinatesMode", LC_QuickInfoBaseData::COORD_ABSOLUTE));
    }

    // initial message
    showNoDataMessage();
}

LC_QuickInfoWidget::~LC_QuickInfoWidget(){
    delete ui;
    delete options;
}

/**
 * Central method for processing entity and displaying it's properties.
 * Will be called from corresponding actions (default action, or entity info action)
 * @param en entity
 */
void LC_QuickInfoWidget::processEntity(RS_Entity *en){
    setWidgetMode(MODE_ENTITY_INFO);
    if (en == nullptr){
        clearEntityInfo();
    }
    else { // just delegate action processing to entity data
        options->displayEntityID = LC_GET_ONE_BOOL("Appearance","ShowEntityIDs", false);
        bool updated = entityData.processEntity(en);
        if (updated){
            updateEntityInfoView();
        }
    }
 }

QString LC_QuickInfoWidget::getEntityDescription(RS_Entity *en, RS2::EntityDescriptionLevel shortDescription) {
    return entityData.getEntityDescription(en, shortDescription);
}

/**
 * Central method for processing collected coordinates
 * @param point coordinate
 */
void LC_QuickInfoWidget::processCoordinate(const RS_Vector &point){
    setWidgetMode(MODE_COORDINATE_COLLECTING);
    pointsData.processCoordinate(point); // delegate processing
    updateCollectedPointsView();
}

/**
 * Specifies which mode should be used for displaying coordinates that are part of entity information
 * @param mode coordinate displaying mode
 */
void LC_QuickInfoWidget::setEntityPointsCoordinateViewMode(int mode){
    bool updated = entityData.updateForCoordinateViewMode(mode);
    if (updated){
        updateEntityInfoView();
    }
}

/**
 * Specifies which mode should be used for displaying coordinates for collected points
 * @param mode  coordinate display mode
 */
void LC_QuickInfoWidget::setCollectedPointsCoordinateViewMode(int mode){
    bool updated = pointsData.updateForCoordinateViewMode(mode);
    if (updated){
        updateCollectedPointsView();
    }
}

/**
 * Cleanup of collected entity information
 */
void LC_QuickInfoWidget::clearEntityInfo(){
    entityData.clear();
    showNoDataMessage();
}

/**
 * Regenerates view data for collected points and displays them
 */
void LC_QuickInfoWidget::updateCollectedPointsView(bool forceUpdate){
    QString data = pointsData.generateView(options->displayDistanceAndAngle, forceUpdate);
    ui->pteInfo->setHtml(data);
    ui->pteInfo1->setPlainText(data);
}

void LC_QuickInfoWidget::updateEntityInfoView(bool forceUpdate, bool updateView){
    if (forceUpdate){
        if (entityData.hasData()){
            unsigned long entityId = entityData.getEntityId();
            RS_Entity *entity = findEntityById(entityId);
            if (entity != nullptr){
                entityData.clear();
                options->displayEntityID = LC_GET_ONE_BOOL("Appearance","ShowEntityIDs", false);
                entityData.processEntity(entity);
            }
        }
    }
    if (updateView){
        if (entityData.hasData()){
            QString data = entityData.generateView();
            ui->pteInfo->setHtml(data);
#ifdef DEBUG_QUICK_INFO_RAW
            ui->pteInfo1->setPlainText(data);
#endif
        } else {
            showNoDataMessage();
        }
    }
}

/**
 * Settings invocation
 */
void LC_QuickInfoWidget::onSettings(){
    invokeOptionsDialog();
}

/**
 * Clearing currently displayed data
 */
void LC_QuickInfoWidget::onClearAll(){
    if (widgetMode == MODE_ENTITY_INFO){
        entityData.clear();
    }
    else if (widgetMode == MODE_COORDINATE_COLLECTING){
        pointsData.clear();
    }
    showNoDataMessage();
}

/**
 * Handler for changing coordinates mode combobox
 * @param index
 */
void LC_QuickInfoWidget::onCoordinateModeIndexChanged(int index){
    LC_GROUP_GUARD("QuickInfoWidget");
    {
        if (widgetMode == MODE_ENTITY_INFO) {
            setEntityPointsCoordinateViewMode(index);
            LC_SET("EntityCoordinatesMode", index);
        } else if (widgetMode == MODE_COORDINATE_COLLECTING) {
            setCollectedPointsCoordinateViewMode(index);
            LC_SET("PointsCoordinatesMode", index);
        }
    }
}

/**
 * Handler for to cmd menu item
 * @param index
 */
void LC_QuickInfoWidget::onToCmd(int index){
    processURLCommand("coord", index);
}

/**
 * Handler for setting relative zero menu handler
 * @param index
 */
void LC_QuickInfoWidget::onSetRelZero(int index){
    processURLCommand("zero", index);
}

/**
 * Handler for removing specific collected coordinate
 * @param index  index of coordinate
 */
void LC_QuickInfoWidget::onRemoveCoordinate(int index){
    if (pointsData.removeCoordinate(index)){
        updateCollectedPointsView(true);
    }
}

/**
 * Handler for insertion of coordinates into specified position
 * @param index  index to insert
 */
void LC_QuickInfoWidget::onInsertCoordinates(int index){
    pointsData.setPointInsertionIndex(index);
    onPickCoordinates();
}

/**
 *  Support method called by action to notify that adding coordinates is completed
 */
void LC_QuickInfoWidget::endAddingCoordinates(){
    pointsData.setPointInsertionIndex(-1);
}

#define DEBUG_MENU_LINK_

/**
 * Context menu for text editor.
 * @param pos
 */
void LC_QuickInfoWidget::onViewContextMenu(QPoint pos){
    // use standard context menu
    QMenu* contextMenu = ui->pteInfo->createStandardContextMenu();

    const QList<QAction *> &actions = contextMenu->actions();
    int visibleCount = 0;
    // remove "copy link location" menu item
    for (auto actionToRemove : actions){
        if (actionToRemove->isVisible()){
            visibleCount ++;
        }
        if (visibleCount == 2){
            contextMenu->removeAction(actionToRemove);
            break;
        }
    }

    contextMenu->addAction(tr("&Copy All"), this, &LC_QuickInfoWidget::onCopyAll);
    const QString &anchor = ui->pteInfo->anchorAt(pos);
    if (!anchor.isEmpty()){
        // process anchor-specific commands first
#ifdef DEBUG_MENU_LINK
        QMenu* newMenu = contextMenu->addMenu(anchor);
        contextMenu->insertMenu(actions.first(), newMenu);
#endif

        if (anchor.startsWith("/coord") || anchor.startsWith("/zero")){ // coordinates related actions
            int sepIndex = anchor.indexOf('?');
            if (sepIndex  != -1){
                QString idx = anchor.mid(sepIndex+1);
                bool ok;
                int pointIndex = idx.toInt(&ok);
                if (ok){
                    if (widgetMode == MODE_COORDINATE_COLLECTING){
                        // specific commands for anchors on collected points view
                        QAction* toCmdAction = contextMenu->addAction(getCoordinateMenuName(tr("&To Cmd"), pointIndex));
                        connect(toCmdAction, &QAction::triggered, this,  [this, pointIndex]{ onToCmd(pointIndex); });

                        QAction* relZeroAction = contextMenu->addAction(getCoordinateMenuName(tr("&Set Relative Zero"),pointIndex));
                        connect(relZeroAction, &QAction::triggered, this,  [this, pointIndex]{ onSetRelZero(pointIndex); });
                        contextMenu->addSeparator();
                        QAction* removeAction = contextMenu->addAction(getCoordinateMenuName(tr("&Remove Coordinate"), pointIndex));
                        connect(removeAction, &QAction::triggered, this,  [this, pointIndex]{ onRemoveCoordinate(pointIndex); });

                        QAction* insertAction = contextMenu->addAction(getCoordinateMenuName(tr("&Insert Coordinates"), pointIndex));
                        connect(insertAction, &QAction::triggered, this,  [this, pointIndex]{ onInsertCoordinates(pointIndex); });
                    }
                    else{
                        // specific commands for anchors on entity info view
                        QAction* toCmdAction = contextMenu->addAction(getCoordinateMenuName(tr("&To Cmd"), -1));
                        connect(toCmdAction, &QAction::triggered, this,  [this, pointIndex]{ onToCmd(pointIndex); });

                        QAction* relZeroAction = contextMenu->addAction(getCoordinateMenuName(tr("&Set Relative Zero"),-1));
                        connect(relZeroAction, &QAction::triggered, this,  [this, pointIndex]{ onSetRelZero(pointIndex); });
                    }
                }
            }
        }
        else if (anchor.startsWith("/val")){ // value-related actions
            contextMenu->addAction(tr("&To Cmd"), this, &LC_QuickInfoWidget::onCopyAll);
        }
    }
    contextMenu->addSeparator();
    // generic actions
    contextMenu->addAction(tr("&Clear"), this, &LC_QuickInfoWidget::onClearAll);
    contextMenu->addAction(tr("&Select Entity"), this, &LC_QuickInfoWidget::onPickEntity);
    if (widgetMode == MODE_ENTITY_INFO){
        if (entityData.getEntityId() > 0){
            contextMenu->addAction(tr("&Select in Drawing"), this, &LC_QuickInfoWidget::onSelectEntity);
            contextMenu->addAction(tr("&Edit Properties"), this, &LC_QuickInfoWidget::onEditEntityProperties);
        }
        contextMenu->addAction(tr("&Collect Coordinates"), this, &LC_QuickInfoWidget::onPickCoordinates);
    }

    contextMenu->popup(ui->pteInfo->viewport()->mapToGlobal(pos));
}

/**
 * Utility method for creation of menu command that includes index of item
 * @param command
 * @param idx
 * @return
 */
QString LC_QuickInfoWidget::getCoordinateMenuName(QString actionName, int idx) const{
    if (idx >= 0){
        QString index;
        index.setNum(idx+1);
        actionName.append(" (").append(index).append(")");
    }
    return actionName;
}

/**
 * Processing of highlighting anchors for coordinates - we'll highlight corresponding coordinate point in drawing
 * @param link
 */
void LC_QuickInfoWidget::onAnchorHighlighted(const QUrl &link){
    QString path = link.fileName();
#ifdef DEBUG_QUICK_INFO_RAW
    const std::string &string = path.toStdString();
#endif
    QString query = link.query();
    int index = query.toInt();
    if (path == "coord" || path == "zero"){
        RS_Vector coord = retrievePositionForModelIndex(index);
        if (coord.valid){
            drawPreviewPoint(coord);
        }
    }
}

/**
 * Processing of anchor un-highlighting - as user moves mouse from anchor, if we've highlighted point - we'll remove it there
 */
void LC_QuickInfoWidget::onAnchorUnHighlighted(){
    if (hasOwnPreview){
        RS_EntityContainer *container = graphicView->getOverlayContainer(RS2::ActionPreviewEntity);
        container->clear();
        graphicView->redraw(RS2::RedrawOverlay);
        hasOwnPreview = false;
    }
}

/**
 * Handler for click on anchor in text editor
 * @param link
 */
void LC_QuickInfoWidget::onAnchorClicked(const QUrl &link){
    QString path = link.fileName();
    QString query = link.query();
    int index = query.toInt();
    processURLCommand(path, index);
}

/**
 * processing of command encoded in URL (move to zero or copy to cmd widget)
 * @param path
 * @param index
 */
void LC_QuickInfoWidget::processURLCommand(const QString &path, int index){
    if (path == "zero"){ // move relative zero to needed coordinate
        RS_Vector data = retrievePositionForModelIndex(index);
        if (data.valid){
            graphicView->moveRelativeZero(data);
        }
    }
    else if (path == "val"){ // copy value to Cmd widget
        if (entityData.hasData()){
            QString value = entityData.getValue(index);
            RS_DIALOGFACTORY->command(value);
        }
    }
    else if (path == "coord"){ // copy coordinate to Cmd widget
        QString data = retrievePositionStringForModelIndex(index);
        RS_DIALOGFACTORY->command(data);
    }
}

/**
 * Returns vector either from collected points or from entity properties
 * @param index index of vector
 * @return vector
 */
RS_Vector LC_QuickInfoWidget::retrievePositionForModelIndex(int index) const{
    auto data = RS_Vector{false};
    if (widgetMode == MODE_ENTITY_INFO){ // return entity property
        if (entityData.hasData()){
            data = entityData.getVectorForIndex(index);
        }
    }
    else if (widgetMode == MODE_COORDINATE_COLLECTING){ // return collected coordinate
        if (pointsData.hasData()){
            data = pointsData.getVectorForIndex(index);
        }
    }
    return data;
}
/**
 * Returns formatted vector string for given index
 * @param index
 * @return
 */
QString LC_QuickInfoWidget::retrievePositionStringForModelIndex(int index) const{
    QString data;
    if (widgetMode == MODE_ENTITY_INFO){
        if (entityData.hasData()){
            data = entityData.getFormattedVectorForIndex(index);
        }
    }
    else if (widgetMode == MODE_COORDINATE_COLLECTING){
        if (pointsData.hasData()){
            data = pointsData.getFormattedVectorForIndex(index);
        }
    }
    return data;
}

/**
 * Draws point in preview for given coordinate
 * @param vector
 */
void LC_QuickInfoWidget::drawPreviewPoint(const RS_Vector& vector) {

    RS_EntityContainer *container =graphicView->getOverlayContainer(RS2::ActionPreviewEntity);
    container->clear();
    // Little hack for now so we don't delete the preview twice
    container->setOwner(false);
    // use pen from options
    RS_Pen pen = options->pen;
    // create preview point
    auto *entity = new RS_Point(container, vector);
    entity->setLayer(nullptr);
    entity->setSelected(false);
    entity->reparent(document);
    entity->setPen(pen);
    container->addEntity(entity);

    graphicView->redraw(RS2::RedrawOverlay);
    hasOwnPreview = true;
}

/**
 * Handler for changing text in text view, used for setting states for buttons
 */
void LC_QuickInfoWidget::onTextChanged(){
    QString text = ui->pteInfo->toPlainText();
    bool hasText = !text.isEmpty();
    bool hasEntityData = entityData.hasData();
    bool hasData = hasEntityData || pointsData.hasData();
    ui->tbClear->setEnabled(hasText && hasData);
    ui->tbCopy->setEnabled(hasText && hasData);
    ui->tbFind->setEnabled(hasText && hasEntityData && widgetMode == MODE_ENTITY_INFO);
    ui->tbEditProperties->setEnabled(hasText && hasEntityData && widgetMode == MODE_ENTITY_INFO);
}

/**
 * Set mode of widget. As different set of coordinates modes is support for entity info mode and for coordinates mode,
 * also do adjustment for coordinates combobox items.
 * @param mode
 */
void LC_QuickInfoWidget::setWidgetMode(int mode){
    widgetMode = mode;

    auto* view = qobject_cast<QListView *>(ui->cbPointsCoordinatesMode->view());
    Q_ASSERT(view != nullptr);

    auto* model = qobject_cast<QStandardItemModel*>(ui->cbPointsCoordinatesMode->model());
    Q_ASSERT(model != nullptr);

    if (mode == MODE_ENTITY_INFO){ // hide 2 modes of coordinates for entity selection
        QStandardItem* item = model->item(2);
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        item = model->item(3);
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);

        view->setRowHidden(2, true);
        view->setRowHidden(3, true);

        // update current coordinate mode
        ui->cbPointsCoordinatesMode->setCurrentIndex(entityData.getCoordinatesMode());
    }
    else if (mode == MODE_COORDINATE_COLLECTING){ // show all 4 items for coordinate modes
        QStandardItem* item = model->item(2);
        item->setFlags(item->flags() | Qt::ItemIsEnabled);
        item = model->item(3);
        item->setFlags(item->flags() | Qt::ItemIsEnabled);

        view->setRowHidden(2, false);
        view->setRowHidden(3, false);
        // update current coordinate mode
        ui->cbPointsCoordinatesMode->setCurrentIndex(pointsData.getCoordinatesMode());
    }
}

/**
 * Copies content of text view to clipboard.
 */
void LC_QuickInfoWidget::onCopyAll(){
    // todo - this is simplest implementation so far, just copy the text from the browser to clipboard.
    // todo - however, more sophisticated functionality may be supported if needed - for example, copy using some structured format (csv, xml etc.)
    // todo - may be it worth to add this later.

    QString text = ui->pteInfo->toPlainText();
    QClipboard* clipboard = QApplication::clipboard();

    clipboard->setText(text, QClipboard::Clipboard);

    if (clipboard->supportsSelection()) {
        clipboard->setText(text, QClipboard::Selection);
    }

#if defined(Q_OS_LINUX)
    QThread::msleep(1); //workaround for copied text not being available...
#endif
}

/**
 * Handler for picking entity menu item
 */
void LC_QuickInfoWidget::onPickEntity(){
    ui->tbSelectEntity->click();
}

/**
 * handler for pick coordinates menu item
 */
void LC_QuickInfoWidget::onPickCoordinates(){
    ui->tbCollectCoords->click();
}

/**
 * Selects entity for which info is shown in in drawing (if entity still exists)
 */
void LC_QuickInfoWidget::onSelectEntity(){
    if (entityData.hasData()){
        // try to find entity by its id.
        unsigned long entityId = entityData.getEntityId();
        RS_Entity* e = findEntityById(entityId);
        if (e != nullptr){
            // entity found, do selection
            e->setSelected(true);
            graphicView->redraw();
        }
        else{
            // if we're there - entity may be selected, or its id may be changed due to modification.
            // so just do cleanup
            clearEntityInfo();
        }
    }
}

/**
 * Perform search of entity of the document by given id
 * @param entityId
 * @return
 */
RS_Entity* LC_QuickInfoWidget::findEntityById(unsigned long entityId) const{
    for (RS_Entity *e: *document) {
        if (e != nullptr && e->getId() == entityId && e->isVisible()){
            return e;
        }
    }
    return nullptr;
}

/**
 * Performs editing of properties for entity currently shown by the widget (if entity with id still exists in the document)
 */
void LC_QuickInfoWidget::onEditEntityProperties(){
    if (entityData.hasData() && document != nullptr){
        unsigned long entityId = entityData.getEntityId();
        RS_Entity *en = findEntityById(entityId);
        if (en != nullptr){
            // entity found, do editing
            std::unique_ptr<RS_Entity> clone{en->clone()};
            en->setSelected(true);

            RS_Entity* newEntity = clone.get();
            if (RS_DIALOGFACTORY->requestModifyEntityDialog(newEntity)){
                // properties changed, do edit
                document->addEntity(newEntity);

                // update widget view
                processEntity(newEntity);

                en->setSelected(false);

                clone->setSelected(false);

                document->startUndoCycle();

                document->addUndoable(newEntity);
                en->setUndoState(true);
                document->addUndoable(en);

                document->endUndoCycle();

                clone.release();
                RS_DIALOGFACTORY->updateSelectionWidget(document->countSelected(), document->totalSelectedLength());
            }
        }
        else{ // entity not found, cleanup
            clearEntityInfo();
        }
        graphicView->redraw();
    }
}

/**
 * Setup of document and graphic view for the widget
 * @param doc
 * @param v
 */
void LC_QuickInfoWidget::setDocumentAndView(RS_Document *doc, QG_GraphicView* v){

    // add tracking of relative point for new view
    if (v != nullptr){
        connect(v, &QG_GraphicView::relative_zero_changed, this, &LC_QuickInfoWidget::onRelativeZeroChanged);
    }
    // remove tracking of relative point from old view
    if (graphicView != nullptr && graphicView != v){
        disconnect(graphicView, &QG_GraphicView::relative_zero_changed, this, &LC_QuickInfoWidget::onRelativeZeroChanged);
    }
    // do setup
    document = doc;
    graphicView = v;
    entityData.setDocumentAndView(doc, v);
    pointsData.setDocumentAndView(doc, v);
    showNoDataMessage();
    hasOwnPreview = false;
}

/**
 * Handler for relative zero changed signal. It's used for updating coordinates if relative zero changed
 * and coordinates mode is relative.
 */
void LC_QuickInfoWidget::onRelativeZeroChanged([[maybe_unused]]const RS_Vector &relZero){
    if (entityData.hasData()){
        if (entityData.getCoordinatesMode() == LC_QuickInfoBaseData::COORD_RELATIVE){
            updateEntityInfoView(true, widgetMode == MODE_ENTITY_INFO);
        }
    }
    if (pointsData.hasData()){
        if (pointsData.getCoordinatesMode() == LC_QuickInfoBaseData::COORD_RELATIVE && widgetMode == MODE_COORDINATE_COLLECTING){
            updateCollectedPointsView(true);
        }
    }
}

/**
 * Displays standard message if no data present
 */
void LC_QuickInfoWidget::showNoDataMessage(){
    ui->pteInfo->setHtml(tr("No data - select entity of coordinates first..."));
}

/**
 * Options editing dialog
 */
void LC_QuickInfoWidget::invokeOptionsDialog(){
    LC_QuickInfoWidgetOptionsDialog dlg = LC_QuickInfoWidgetOptionsDialog(this, options);

    bool oldDisplayDistance = options->displayDistanceAndAngle;
    int dialogResult = dlg.exec();
    if (dialogResult == QDialog::Accepted){
        options->save();
        // do refresh of collected points, if needed
        if (options->displayDistanceAndAngle != oldDisplayDistance){
            if (pointsData.hasData()){
                updateCollectedPointsView(true);
            }
        }
        // refreshing entity info, if any
        if (entityData.hasData()){
            updateEntityInfoView(true, widgetMode == MODE_ENTITY_INFO);
        }

        update();
    }
}

/**
 * Method that is called outside to notify the user that some entity was edited - so the widget has a chance to update entity info, if possible
 * @param originalId  original entity id
 * @param editedCloneId if editing includes creation of clone for original entity - id of clone
 */
void LC_QuickInfoWidget::onEntityPropertiesEdited(unsigned long originalId, unsigned long editedCloneId){
  if (entityData.hasData()){
      unsigned long currentEntityId = entityData.getEntityId();
      if (currentEntityId == originalId) {  // entity that is currently displayed was edited
          if (editedCloneId > 0){ // this was editing via properties dialog, so clone was created
              RS_Entity *editedEntity = findEntityById(editedCloneId);
              processEntity(editedEntity);
          }
          else{
              // well, that's actually for future - if editing of entity was performed via mouse and so some coordinates were changed,
              // it should be possible to update the entity in real time.
              // however, not clear how to handle this is come centralized way so far without modifying lots of actions...
              updateEntityInfoView(true);
          }
      }
  }
}
