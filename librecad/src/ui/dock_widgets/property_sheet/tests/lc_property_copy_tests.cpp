/*
 * This file is part of LibreCAD, distributed under the GNU General Public
 * License version 2 or (at your option) any later version.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QEventLoop>
#include <QLineEdit>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMenu>
#include <QMimeData>
#include <QPointer>
#include <QProcess>
#include <QRadioButton>
#include <QTimer>

#include "lc_actioncontext.h"
#include "lc_actiongroupmanager.h"
#include "lc_actiontestsupport.h"
#include "lc_dlg_dimension.h"
#include "lc_dimordinate.h"
#include "lc_graphicviewport.h"
#include "lc_menufactory_graphicview.h"
#include "lc_properties_sheet.h"
#include "lc_property_container.h"
#include "lc_property_qstring.h"
#include "lc_property_qstring_lineedit_view.h"
#include "lc_propertysheet_widget_options.h"
#include "lc_propertysheetwidget.h"
#include "lc_undosection.h"
#include "qg_graphicview.h"
#include "rs_dimaligned.h"
#include "rs_dimlinear.h"
#include "rs_graphic.h"
#include "rs_leader.h"
#include "rs_line.h"

namespace {
class ClipboardGuard {
public:
    ClipboardGuard() : saved(std::make_unique<QMimeData>()) {
        const QMimeData* current = QApplication::clipboard()->mimeData();
        if (current != nullptr) {
            for (const QString& format : current->formats()) saved->setData(format, current->data(format));
        }
    }
    ~ClipboardGuard() { QApplication::clipboard()->setMimeData(saved.release()); }
private:
    std::unique_ptr<QMimeData> saved;
};

class DimensionMenuFactory : public LC_MenuFactoryGraphicView {
public:
    explicit DimensionMenuFactory(LC_ActionGroupManager* actions = nullptr) : LC_MenuFactoryGraphicView(nullptr, actions, nullptr) {}
    using LC_MenuFactoryGraphicView::createGVDimensionCopyActions;
    using LC_MenuFactoryGraphicView::createGVMenuEntitySpecific;
};
}

TEST_CASE("Copyable read-only property uses a selectable editor", "[gui][properties][clipboard]") {
    (void)lc::test::application();
    ClipboardGuard clipboardGuard;
    LC_PropertyQString property(nullptr, true);
    property.setValue(QStringLiteral("12.345"));
    property.setReadOnly();
    LC_PropertyQStringLineEditView view(&property);
    LC_PropertyViewDescriptor descriptor(LC_PropertyQStringLineEditView::VIEW_NAME);
    descriptor[LC_PropertyQStringLineEditView::ATTR_COPYABLE_READ_ONLY] = true;
    view.applyAttributes(descriptor);

    QWidget parent;
    QEvent activation(QEvent::MouseButtonRelease);
    LC_PropertyViewEditable::EditActivationContext context(&activation);
    auto* editor = qobject_cast<QLineEdit*>(view.createValueEditor(&parent, QRect(0, 0, 200, 24), &context));
    REQUIRE(editor != nullptr);
    CHECK(editor->isReadOnly());
    CHECK(editor->text() == "12.345");
    editor->selectAll();
    const auto bindings = QKeySequence::keyBindings(QKeySequence::Copy);
    REQUIRE_FALSE(bindings.isEmpty());
    const QKeyCombination shortcut = bindings.first()[0];
    QKeyEvent copyEvent(QEvent::KeyPress, shortcut.key(), shortcut.keyboardModifiers(), "c");
    QCoreApplication::sendEvent(editor, &copyEvent);
    CHECK(QApplication::clipboard()->text() == "12.345");
#ifdef Q_OS_MACOS
    if (QGuiApplication::platformName() == "cocoa") {
        QProcess paste;
        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        QObject::connect(&paste, &QProcess::finished, &loop, &QEventLoop::quit);
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        paste.start("/usr/bin/pbpaste", QStringList{});
        timeout.start(5000);
        loop.exec();
        REQUIRE(paste.state() == QProcess::NotRunning);
        CHECK(paste.exitCode() == 0);
        CHECK(paste.readAllStandardOutput() == QByteArray("12.345"));
    }
#endif
    QApplication::clipboard()->clear();
    std::unique_ptr<QMenu> textMenu(editor->createStandardContextMenu());
    QAction* copyAction = nullptr;
    for (QAction* action : textMenu->actions()) {
        if (action->text().contains("Copy", Qt::CaseInsensitive)) copyAction = action;
    }
    REQUIRE(copyAction != nullptr);
    copyAction->trigger();
    CHECK(QApplication::clipboard()->text() == "12.345");
    textMenu.reset();
    delete editor;
}

TEST_CASE("Unavailable read-only property does not open a copy editor", "[gui][properties][clipboard]") {
    (void)lc::test::application();
    LC_PropertyQString property(nullptr, true);
    property.setValue(QString());
    property.setReadOnly();
    LC_PropertyQStringLineEditView view(&property);
    LC_PropertyViewDescriptor descriptor(LC_PropertyQStringLineEditView::VIEW_NAME);
    descriptor[LC_PropertyQStringLineEditView::ATTR_COPYABLE_READ_ONLY] = true;
    view.applyAttributes(descriptor);
    QWidget parent;
    QEvent activation(QEvent::MouseButtonRelease);
    LC_PropertyViewEditable::EditActivationContext context(&activation);
    CHECK(view.createValueEditor(&parent, QRect(0, 0, 200, 24), &context) == nullptr);
}

TEST_CASE("Dimension property sheet copies its measured value and closes the editor on detach", "[gui][properties][clipboard]") {
    (void)lc::test::application();
    ClipboardGuard clipboardGuard;
    LC_ActionContext context;
    LC_ActionGroupManager actions(nullptr);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_DimensionData data;
    data.definitionPoint = RS_Vector(0, 3);
    data.middleOfText = RS_Vector(5, 3);
    data.style = "Standard";
    data.text = " ";
    auto* dimension = new RS_DimAligned(&graphic, data, {RS_Vector(0, 0), RS_Vector(10, 0)});
    graphic.addEntity(dimension);
    dimension->update();
    graphic.select(QList<RS_Entity*>{dimension}, true);

    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    context.setDocumentAndView(&graphic, &view);
    LC_PropertySheetWidget sheet(nullptr, &context, &actions);
    sheet.getOptions()->noSelectionWorkspace = false;
    sheet.getOptions()->noSelectionGraphicView = false;
    sheet.show();
    sheet.setGraphicView(&view);
    QCoreApplication::processEvents();

    auto* properties = sheet.findChild<LC_PropertiesSheet*>();
    REQUIRE(properties != nullptr);
    REQUIRE(properties->getPropertyContainer() != nullptr);
    QStringList propertyNames;
    for (const auto* property : properties->getPropertyContainer()->childProperties()) propertyNames << property->getName();
    INFO("root properties: " << propertyNames.join(", ").toStdString());
    CAPTURE(dimension->isSelected(), graphic.hasSelection());
    auto* measured = properties->findProperty("$_secDimText.$_secDimText$dimTextMeasurement");
    REQUIRE(measured != nullptr);
    REQUIRE(properties->setActiveProperty(measured, true));
    (void)properties->grab();
    int edits = 0;
    QObject::connect(properties, &LC_PropertiesSheet::propertyEdited, &sheet, [&edits] { ++edits; });
    QPoint valuePoint(-1, -1);
    for (int y = 0; y < properties->viewport()->height(); ++y) {
        const QPoint candidate(properties->viewport()->width() * 3 / 4, y);
        if (properties->getPropertyAt(candidate) == measured) {
            valuePoint = candidate;
            break;
        }
    }
    REQUIRE(valuePoint.x() >= 0);
    const QPoint globalPoint = properties->viewport()->mapToGlobal(valuePoint);
    QMouseEvent press(QEvent::MouseButtonPress, valuePoint, globalPoint, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease, valuePoint, globalPoint, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(properties->viewport(), &press);
    QCoreApplication::sendEvent(properties->viewport(), &release);
    auto* editor = properties->findChild<QLineEdit*>("LCPropertyValueEditor");
    REQUIRE(editor != nullptr);
    CHECK(editor->isReadOnly());
    CHECK(editor->text() == "10");
    QKeyEvent attemptEdit(QEvent::KeyPress, Qt::Key_9, Qt::NoModifier, "9");
    QCoreApplication::sendEvent(editor, &attemptEdit);
    CHECK(editor->text() == "10");
    editor->selectAll();
    const QKeyCombination shortcut = QKeySequence::keyBindings(QKeySequence::Copy).first()[0];
    QKeyEvent copyEvent(QEvent::KeyPress, shortcut.key(), shortcut.keyboardModifiers(), "c");
    QCoreApplication::sendEvent(editor, &copyEvent);
    CHECK(QApplication::clipboard()->text() == "10");
    CHECK(edits == 0);
    QPointer<QLineEdit> alive(editor);
    QPointer<QMenu> textMenu(editor->createStandardContextMenu());
    QAction* popupCopy = nullptr;
    for (auto* action : textMenu->actions()) {
        if (action->text().contains("Copy", Qt::CaseInsensitive)) popupCopy = action;
    }
    REQUIRE(popupCopy != nullptr);
    textMenu->popup(QPoint(20, 20));
    QCoreApplication::processEvents();
    REQUIRE_FALSE(alive.isNull());
    QApplication::clipboard()->clear();
    textMenu->setActiveAction(popupCopy);
    QKeyEvent popupEnter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QCoreApplication::sendEvent(textMenu.data(), &popupEnter);
    QCoreApplication::processEvents();
    CHECK(QApplication::clipboard()->text() == "10");
    delete textMenu.data();
    if (alive.isNull()) {
        QKeyEvent reactivateAfterPopup(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QCoreApplication::sendEvent(properties, &reactivateAfterPopup);
        editor = properties->findChild<QLineEdit*>("LCPropertyValueEditor");
        REQUIRE(editor != nullptr);
        alive = editor;
    }
    QKeyEvent enterEditor(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QCoreApplication::sendEvent(editor, &enterEditor);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    CHECK(alive.isNull());
    QKeyEvent reactivateAfterEnter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QCoreApplication::sendEvent(properties, &reactivateAfterEnter);
    editor = properties->findChild<QLineEdit*>("LCPropertyValueEditor");
    REQUIRE(editor != nullptr);
    alive = editor;
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QCoreApplication::sendEvent(editor, &escape);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    CHECK(alive.isNull());
    QKeyEvent reactivate(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QCoreApplication::sendEvent(properties, &reactivate);
    editor = properties->findChild<QLineEdit*>("LCPropertyValueEditor");
    REQUIRE(editor != nullptr);
    alive = editor;
    // Offscreen can establish focus deterministically without an active desktop.
    if (QGuiApplication::platformName() == "offscreen") {
        sheet.activateWindow();
        editor->setFocus(Qt::OtherFocusReason);
        QCoreApplication::processEvents();
        REQUIRE(editor->hasFocus());
        QKeyEvent tab(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
        QCoreApplication::sendEvent(editor, &tab);
        QCoreApplication::processEvents();
        CHECK((alive.isNull() || !alive->hasFocus()));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        if (alive.isNull()) {
            QKeyEvent reactivateAfterTab(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
            QCoreApplication::sendEvent(properties, &reactivateAfterTab);
            editor = properties->findChild<QLineEdit*>("LCPropertyValueEditor");
            REQUIRE(editor != nullptr);
            alive = editor;
        }
    }
    QPointer<QMenu> menuDuringRebind(editor->createStandardContextMenu());
    menuDuringRebind->popup(QPoint(20, 20));
    QCoreApplication::processEvents();

    auto* other = new RS_DimAligned(&graphic, data, {RS_Vector(0, 0), RS_Vector(10, 0)});
    graphic.addEntity(other);
    other->update();
    graphic.select(QList<RS_Entity*>{other}, true);
    sheet.setGraphicView(&view);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    CHECK(alive.isNull());
    CHECK(menuDuringRebind.isNull());
    QCoreApplication::processEvents();
    measured = properties->findProperty("$_secDimText.$_secDimText$dimTextMeasurement");
    REQUIRE(measured != nullptr);
    CHECK_FALSE(measured->isMultiValue());
    properties->setActiveProperty(measured, true);
    (void)properties->grab();
    QKeyEvent activateEqual(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QCoreApplication::sendEvent(properties, &activateEqual);
    editor = properties->findChild<QLineEdit*>("LCPropertyValueEditor");
    REQUIRE(editor != nullptr);
    CHECK(editor->text() == "10");
    alive = editor;

    other->setExtensionPoint2(RS_Vector(11, 0));
    other->update();
    sheet.setGraphicView(&view);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    CHECK(alive.isNull());
    QCoreApplication::processEvents();
    measured = properties->findProperty("$_secDimText.$_secDimText$dimTextMeasurement");
    REQUIRE(measured != nullptr);
    CHECK(measured->isMultiValue());
    properties->setActiveProperty(measured, true);
    (void)properties->grab();
    QKeyEvent activateMixed(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QCoreApplication::sendEvent(properties, &activateMixed);
    CHECK(properties->findChild<QLineEdit*>("LCPropertyValueEditor") == nullptr);
    CHECK(edits == 0);

    graphic.select(QList<RS_Entity*>{other}, false);
    sheet.setGraphicView(&view);
    QCoreApplication::processEvents();
    measured = properties->findProperty("$_secDimText.$_secDimText$dimTextMeasurement");
    REQUIRE(measured != nullptr);
    properties->setActiveProperty(measured, true);
    (void)properties->grab();
    QKeyEvent activateBeforeDetach(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QCoreApplication::sendEvent(properties, &activateBeforeDetach);
    alive = properties->findChild<QLineEdit*>("LCPropertyValueEditor");
    REQUIRE_FALSE(alive.isNull());
    {
        LC_UndoSection undo(&graphic, view.getViewPort());
        undo.addUndoable(dimension);
    }
    REQUIRE(graphic.undo());
    CHECK(dimension->isDeleted());
    view.defaultActionActivated(true, RS2::ActionDefault, RS2::ActionEditUndo);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    CHECK(alive.isNull());
    REQUIRE(graphic.redo());
    graphic.select(QList<RS_Entity*>{dimension}, true);
    sheet.setGraphicView(&view);
    QCoreApplication::processEvents();
    measured = properties->findProperty("$_secDimText.$_secDimText$dimTextMeasurement");
    REQUIRE(measured != nullptr);
    properties->setActiveProperty(measured, true);
    (void)properties->grab();
    QKeyEvent activateAfterUndo(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QCoreApplication::sendEvent(properties, &activateAfterUndo);
    alive = properties->findChild<QLineEdit*>("LCPropertyValueEditor");
    REQUIRE_FALSE(alive.isNull());
    sheet.setGraphicView(nullptr);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    CHECK(alive.isNull());
}

TEST_CASE("Dimension dialog reflects pending geometry without modifying its entity", "[gui][properties][clipboard]") {
    (void)lc::test::application();
    ClipboardGuard clipboardGuard;
    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    RS_DimensionData data;
    data.definitionPoint = RS_Vector(0, 3);
    data.middleOfText = RS_Vector(5, 3);
    data.style = "Standard";
    RS_DimLinear dimension(&graphic, data, {RS_Vector(0, 0), RS_Vector(3, 4), 0, 0});
    dimension.update();
    LC_DlgDimension dialog(nullptr, view.getViewPort(), &dimension);
    auto* value = dialog.findChild<QLineEdit*>("leMeasuredValue");
    auto* angle = dialog.findChild<QLineEdit*>("leAngle");
    REQUIRE(value != nullptr);
    REQUIRE(angle != nullptr);
    CHECK(value->isReadOnly());
    CHECK(value->text() == "3");
    value->selectAll();
    const QKeyCombination shortcut = QKeySequence::keyBindings(QKeySequence::Copy).first()[0];
    QKeyEvent copyEvent(QEvent::KeyPress, shortcut.key(), shortcut.keyboardModifiers(), "c");
    QCoreApplication::sendEvent(value, &copyEvent);
    CHECK(QApplication::clipboard()->text() == "3");
    QKeyEvent attemptEdit(QEvent::KeyPress, Qt::Key_9, Qt::NoModifier, "9");
    QCoreApplication::sendEvent(value, &attemptEdit);
    CHECK(value->text() == "3");
    angle->setText("90");
    CHECK(value->text().toDouble() == Catch::Approx(4));
    angle->setText("invalid angle");
    CHECK(value->text().isEmpty());
    dialog.reject();
    CHECK(dimension.getAngle() == Catch::Approx(0));
    CHECK(dimension.getMeasuredValueText() == "3");

    view.getViewPort()->createUCS(RS_Vector(0, 0), M_PI_2);
    LC_DlgDimension rotatedDialog(nullptr, view.getViewPort(), &dimension);
    auto* rotatedValue = rotatedDialog.findChild<QLineEdit*>("leMeasuredValue");
    auto* rotatedAngle = rotatedDialog.findChild<QLineEdit*>("leAngle");
    REQUIRE(rotatedValue != nullptr);
    REQUIRE(rotatedAngle != nullptr);
    rotatedAngle->setText("0");
    CHECK(rotatedValue->text().toDouble() == Catch::Approx(4));
    rotatedDialog.reject();
    CHECK(dimension.getAngle() == Catch::Approx(0));

    RS_DimensionData ordinateData(data);
    ordinateData.definitionPoint = RS_Vector(10, 20);
    LC_DimOrdinate ordinate(&graphic, ordinateData, {RS_Vector(13, 24), RS_Vector(15, 25), true});
    ordinate.update();
    LC_DlgDimension ordinateDialog(nullptr, view.getViewPort(), &ordinate);
    auto* ordinateValue = ordinateDialog.findChild<QLineEdit*>("leMeasuredValue");
    auto* ordinateY = ordinateDialog.findChild<QRadioButton*>("rbOrdinateY");
    REQUIRE(ordinateValue != nullptr);
    REQUIRE(ordinateY != nullptr);
    CHECK(ordinateValue->text() == "3");
    ordinateY->setChecked(true);
    CHECK(ordinateValue->text() == "4");
    ordinateDialog.reject();
    CHECK(ordinate.isForXDirection());
}

TEST_CASE("Dimension menu actions copy snapshots of the context entity", "[gui][properties][clipboard]") {
    (void)lc::test::application();
    ClipboardGuard clipboardGuard;
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_DimensionData data;
    data.definitionPoint = RS_Vector(0, 3);
    data.middleOfText = RS_Vector(5, 3);
    data.style = "Standard";
    data.text = "fixed label";
    RS_DimAligned dimension(&graphic, data, {RS_Vector(0, 0), RS_Vector(10, 0)});
    dimension.update();
    auto* selected = new RS_Line(&graphic, RS_Vector(0, 0), RS_Vector(1, 0));
    graphic.addEntity(selected);
    graphic.select(QList<RS_Entity*>{selected}, true);
    REQUIRE(selected->isSelected());
    CHECK_FALSE(dimension.isSelected());
    DimensionMenuFactory factory;
    QMenu menu;
    factory.createGVDimensionCopyActions(&menu, &dimension);
    REQUIRE(menu.actions().size() == 3);
    auto* value = menu.actions().at(0);
    auto* formatted = menu.actions().at(1);
    CHECK(value->text() == "Copy value");
    CHECK(formatted->text() == "Copy formatted");
    REQUIRE(value->isEnabled());
    REQUIRE(formatted->isEnabled());
    value->trigger();
    CHECK(QApplication::clipboard()->text() == "10");
    formatted->trigger();
    CHECK(QApplication::clipboard()->text() == "fixed label");
    CHECK(selected->isSelected());
    CHECK_FALSE(dimension.isSelected());

    dimension.setLabel(" ");
    dimension.update();
    formatted->trigger();
    CHECK(QApplication::clipboard()->text() == "fixed label");
    QMenu suppressed;
    factory.createGVDimensionCopyActions(&suppressed, &dimension);
    REQUIRE(suppressed.actions().size() == 3);
    CHECK(suppressed.actions().at(0)->isEnabled());
    CHECK_FALSE(suppressed.actions().at(1)->isEnabled());
    QApplication::clipboard()->setText("preserve");
    suppressed.actions().at(1)->trigger();
    CHECK(QApplication::clipboard()->text() == "preserve");

    RS_Line line(nullptr, RS_Vector(0, 0), RS_Vector(1, 0));
    QMenu unrelated;
    factory.createGVDimensionCopyActions(&unrelated, &line);
    CHECK(unrelated.isEmpty());
    RS_Leader leader(nullptr);
    factory.createGVDimensionCopyActions(&unrelated, &leader);
    CHECK(unrelated.isEmpty());

    RS_DimAligned invalid(nullptr, data, {RS_Vector(false), RS_Vector(0, 0)});
    QMenu invalidMenu;
    factory.createGVDimensionCopyActions(&invalidMenu, &invalid);
    REQUIRE(invalidMenu.actions().size() == 3);
    CHECK_FALSE(invalidMenu.actions().at(0)->isEnabled());
    CHECK_FALSE(invalidMenu.actions().at(1)->isEnabled());
    invalidMenu.actions().at(0)->trigger();
    CHECK(QApplication::clipboard()->text() == "preserve");
}

TEST_CASE("Dimension-specific menu includes copy actions without changing the document", "[gui][properties][clipboard]") {
    (void)lc::test::application();
    ClipboardGuard clipboardGuard;
    struct TestGraphic : RS_Graphic { using RS_Undo::countUndoCycles; } graphic;
    graphic.initForNewDocument();
    LC_ActionContext context;
    LC_ActionGroupManager actions(nullptr);
    QG_GraphicView view(nullptr, &graphic, &context);
    RS_DimensionData data;
    data.definitionPoint = RS_Vector(0, 3);
    data.middleOfText = RS_Vector(5, 3);
    data.style = "Standard";
    data.text = "fixed label";
    auto* dimension = new RS_DimAligned(&graphic, data, {RS_Vector(0, 0), RS_Vector(10, 0)});
    graphic.addEntity(dimension);
    dimension->update();
    graphic.setModified(false);
    const auto childCount = dimension->count();
    const auto measurement = dimension->getMeasurement();
    const auto undoCount = graphic.countUndoCycles();

    DimensionMenuFactory factory(&actions);
    QMenu menu;
    QApplication::clipboard()->setText("untouched");
    factory.createGVMenuEntitySpecific(&menu, &view, dimension, RS_Vector(0, 0));
    REQUIRE(menu.actions().size() >= 2);
    CHECK(menu.actions().at(0)->text() == "Copy value");
    CHECK(menu.actions().at(1)->text() == "Copy formatted");
    CHECK(QApplication::clipboard()->text() == "untouched");
    menu.actions().at(1)->trigger();
    CHECK(QApplication::clipboard()->text() == "fixed label");
    CHECK_FALSE(graphic.isModified());
    CHECK(graphic.countUndoCycles() == undoCount);
    CHECK(dimension->count() == childCount);
    CHECK(dimension->getMeasurement() == measurement);
}
