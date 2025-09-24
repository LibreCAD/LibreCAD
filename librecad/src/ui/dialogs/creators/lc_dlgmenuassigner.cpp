/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_dlgmenuassigner.h"

#include <QMouseEvent>

#include "lc_dialog.h"
#include "ui_lc_dlgmenuassigner.h"

LC_DlgMenuAssigner::LC_DlgMenuAssigner(QWidget *parent, LC_MenuActivator* activator, QList<LC_MenuActivator*>* activators)
    :LC_Dialog(parent, "MenuAssigner"), ui(new Ui::LC_DlgMenuAssigner),
    m_activator{activator}, m_activators{activators}{

    ui->setupUi(this);

    bool ctrl{false}, alt{false}, shift{false};
    m_activator->getKeysState(ctrl, alt, shift);

    ui->cbKeyCtrl->setChecked(ctrl);
    ui->cbKeyALt->setChecked(alt);
    ui->cbKeyShift->setChecked(shift);

    auto button = m_activator->getButtonType();
    switch (button) {
        case LC_MenuActivator::LEFT:
            ui->rbBtnLeft->setChecked(true);
            break;
        case LC_MenuActivator::RIGHT:
            ui->rbBtnRight->setChecked(true);
            break;
        case LC_MenuActivator::MIDDLE:
            ui->rbBtnMiddle->setChecked(true);
            break;
        case LC_MenuActivator::BACK:
            ui->rbBtnBack->setChecked(true);
            break;
        case LC_MenuActivator::FORWARD:
            ui->rbBtnForward->setChecked(true);
            break;
        case LC_MenuActivator::TASK:
            ui->rbBtnTask->setChecked(true);
            break;
        default:
            break;
    }

    auto eventType = m_activator->getEventType();
    if (eventType == LC_MenuActivator::DOUBLE_CLICK) {
        ui->rbEvtDblClick->setChecked(true);
    }
    else {
        ui->rbEvtClickRelease->setChecked(true);
    }

    initEntityContextCombobox();

    QString entityTypeSuffix = m_activator->getEntityTypeStr();
    int currentIndex = ui->cbEntityContext->findData(entityTypeSuffix);
    ui->cbEntityContext->setCurrentIndex(currentIndex);

    connect(ui->cbKeyALt, &QCheckBox::toggled, this, &LC_DlgMenuAssigner::onKeyModifierToggled);
    connect(ui->cbKeyShift, &QCheckBox::toggled, this, &LC_DlgMenuAssigner::onKeyModifierToggled);
    connect(ui->cbKeyCtrl, &QCheckBox::toggled, this, &LC_DlgMenuAssigner::onKeyModifierToggled);

    connect(ui->rbEvtDblClick, &QRadioButton::toggled, this, &LC_DlgMenuAssigner::onEventTypeToggled);
    connect(ui->rbEvtClickRelease, &QRadioButton::toggled, this, &LC_DlgMenuAssigner::onEventTypeToggled);

    connect(ui->rbBtnLeft, &QRadioButton::toggled, this, &LC_DlgMenuAssigner::onBtnTypeToggled);
    connect(ui->rbBtnRight, &QRadioButton::toggled, this, &LC_DlgMenuAssigner::onBtnTypeToggled);
    connect(ui->rbBtnMiddle, &QRadioButton::toggled, this, &LC_DlgMenuAssigner::onBtnTypeToggled);
    connect(ui->rbBtnBack, &QRadioButton::toggled, this, &LC_DlgMenuAssigner::onBtnTypeToggled);
    connect(ui->rbBtnForward, &QRadioButton::toggled, this, &LC_DlgMenuAssigner::onBtnTypeToggled);
    connect(ui->rbBtnTask, &QRadioButton::toggled, this, &LC_DlgMenuAssigner::onBtnTypeToggled);
    connect(ui->cbEntityContext, &QComboBox::currentIndexChanged, this, &LC_DlgMenuAssigner::onContextEntityCurrentIndexChanged);

    ui->lblResult->installEventFilter(this);
    ui->gbInvocationShortcut->installEventFilter(this);

    updateShortcutView();

    setWindowTitle(tr("Menu Assignment - \"%1\" Menu").arg(m_activator->getMenuName()));
}

void LC_DlgMenuAssigner::initEntityContextCombobox() {
    ui->cbEntityContext->addItem(tr("Either Absent or Any Entity"), "EE");
    ui->cbEntityContext->addItem(tr("Absent Entity"), "NE");
    ui->cbEntityContext->addItem(tr("Any Entity"), "AE");
    ui->cbEntityContext->addItem(tr("Line"), "LI");
    ui->cbEntityContext->addItem(tr("Circle"), "CI");
    ui->cbEntityContext->addItem(tr("Arc"), "AR");
    ui->cbEntityContext->addItem(tr("Polyline"), "PL");
    ui->cbEntityContext->addItem(tr("Spline"), "SL");
    ui->cbEntityContext->addItem(tr("Spline By Points"), "SP");
    ui->cbEntityContext->addItem(tr("Ellipse"), "EL");
    ui->cbEntityContext->addItem(tr("Point"), "PO");
    ui->cbEntityContext->addItem(tr("Parabola"), "PA");
    ui->cbEntityContext->addItem(tr("Image"), "IM");
    ui->cbEntityContext->addItem(tr("Hatch"), "HA");
    ui->cbEntityContext->addItem(tr("Insert"), "IN");
    ui->cbEntityContext->addItem(tr("Dimension Linear"), "DL");
    ui->cbEntityContext->addItem(tr("Dimension Aligned"), "DA");
    ui->cbEntityContext->addItem(tr("Dimension Diametric"), "DD");
    ui->cbEntityContext->addItem(tr("Dimension Radial"), "DR");
    ui->cbEntityContext->addItem(tr("Dimension Ordinate"), "DO");
    ui->cbEntityContext->addItem(tr("Dimension Arc"), "DC");
    ui->cbEntityContext->addItem(tr("Leader"), "LD");
}

LC_DlgMenuAssigner::~LC_DlgMenuAssigner(){
    delete ui;
}

bool LC_DlgMenuAssigner::eventFilter(QObject* object, QEvent* event) {
    if ( object == ui->lblResult || object == ui->gbInvocationShortcut) {
        QMouseEvent* mouseEvent {nullptr};
        if (event->type() == QEvent::MouseButtonRelease) {
            // eliminate second click event on double click
            bool processClick = !m_doubleClickTimer.isValid() ? true : m_doubleClickTimer.elapsed() > 100;
            if (processClick) { // for win, double click interval is 100-900 ms, default 500ms
                mouseEvent = dynamic_cast<QMouseEvent*>(event);
                ui->rbEvtClickRelease->setChecked(true);
            }
        }
        else if (event->type() == QEvent::MouseButtonDblClick) {
            mouseEvent = dynamic_cast<QMouseEvent*>(event);
            ui->rbEvtDblClick->setChecked(true);
            m_doubleClickTimer.start();
        }
        if (mouseEvent != nullptr) {
            auto button = mouseEvent->button();
            // LC_ERR << "Button " << button;
            switch (button) {
                case Qt::LeftButton: {
                    ui->rbBtnLeft->setChecked(true);
                    break;
                }
                case Qt::MiddleButton: {
                    ui->rbBtnMiddle->setChecked(true);
                    break;
                }
                case Qt::RightButton: {
                    ui->rbBtnRight->setChecked(true);
                    break;
                }
                case Qt::TaskButton: {
                    ui->rbBtnTask->setChecked(true);
                    break;
                }
                case Qt::BackButton: {
                    ui->rbBtnBack->setChecked(true);
                    break;
                }
                case Qt::ForwardButton: {
                    ui->rbBtnForward->setChecked(true);
                    break;
                }
                default:
                    break;
            }

            auto modifiers = mouseEvent->modifiers();

            ui->cbKeyShift->setChecked(modifiers & Qt::ShiftModifier);
            ui->cbKeyCtrl->setChecked(modifiers & Qt::ControlModifier);
            ui->cbKeyALt->setChecked(modifiers & Qt::AltModifier);
            return true;
        }
    }
    return false;
}

void LC_DlgMenuAssigner::updateShortcutView() {
    m_activator->update();
    auto shortcutView = m_activator->getShortcutView();
    ui->lblResult->setText(shortcutView);
    validateShortcut();
}

QString LC_DlgMenuAssigner::findMenuForActivator() {
    qsizetype size = m_activators->size();
    for (int i = 0; i< size; i++) {
        auto a = m_activators->at(i);
        auto otherMenuName = a->getMenuName();
        if (a->isSameAs(m_activator) && (otherMenuName != m_activator->getMenuName())) {
            return otherMenuName;
        }
    }
    return "";
}

bool LC_DlgMenuAssigner::validateShortcut() {
    bool result = false;
    bool ctrl{false}, alt{false}, shift{false};
    m_activator->getKeysState(ctrl, alt, shift);
    bool noKeys = !m_activator->hasKeys();
    bool dblClick = m_activator->getEventType() == LC_MenuActivator::DOUBLE_CLICK;
    bool click = m_activator->getEventType() == LC_MenuActivator::CLICK_RELEASE;
    bool entityRequired = m_activator->isEntityRequired();
    auto button_type = m_activator->getButtonType();

    bool leftButton = button_type == LC_MenuActivator::LEFT;
    bool rightButton = button_type == LC_MenuActivator::RIGHT;
    bool middleButton = button_type == LC_MenuActivator::MIDDLE;

    QString noteMsg;

    // check for left mouse double click
    if (leftButton && noKeys && dblClick && entityRequired) {
        noteMsg = tr("NOTE: Menu assignment will be ignored. It is reserved for 'Entity Properties'.");
    }
    else  if (leftButton && noKeys && click) {
        noteMsg = tr("NOTE: Menu assignment will be ignored. It is reserved for 'Entity Select'.");
    }
    else  if (leftButton && ctrl && click && !alt && !shift) {
        noteMsg = tr("NOTE: Menu assignment will be ignored. It is reserved for 'Pan'.");
    }
    else  if (leftButton && shift && click && !alt && !ctrl) {
        noteMsg = tr("NNOTE: Menu assignment will be ignored. It is reserved for 'Select Contour'.");
    }
    else if (rightButton && noKeys && click) {
        noteMsg = tr("NOTE: This combination is reserved for default context menu and may prevent invocation of it!");
    }
    else if (middleButton && noKeys && click) {
        ui->lblNotes->setText(tr("NOTE: This combination is reserved for Pan! Menu assignment will be ignored"));
    }
    else {
        QString existingMenuName = findMenuForActivator();
        if (!existingMenuName.isEmpty()) {
            noteMsg = tr( "NOTE: This shortcut is already assigned to \"%1\" menu and that menu will be unassigned on save!").arg(
                existingMenuName);
        }
        else {
            noteMsg = tr("Shortcut is valid to use.");
            result = true;
        }
        if (button_type == LC_MenuActivator::FORWARD || button_type == LC_MenuActivator::BACK || button_type ==
            LC_MenuActivator::TASK) {
            noteMsg.append("\n\n").append(tr("Note: make sure that selected button is supported by your mouse device."));
        }
    }
    ui->lblNotes->setText(noteMsg);
    return result;
}

void LC_DlgMenuAssigner::onKeyModifierToggled([[maybe_unused]]bool checked) {
    m_activator->setKeys(ui->cbKeyCtrl->isChecked(), ui->cbKeyALt->isChecked(), ui->cbKeyShift->isChecked());
    updateShortcutView();
}

void LC_DlgMenuAssigner::onContextEntityCurrentIndexChanged([[maybe_unused]]int currentIndex) {
    auto data = ui->cbEntityContext->itemData(currentIndex);
    if (!data.isNull()) {
        QString entityTypeStr = data.toString();
        RS2::EntityType entityType {RS2::EntityUnknown};
        bool requiresEntity = false;
        m_activator->parseEntityType(entityTypeStr, requiresEntity, entityType);
        m_activator->setEntityRequired(requiresEntity);
        m_activator->setEntityType(entityType);
    }
    updateShortcutView();
}

void LC_DlgMenuAssigner::onEventTypeToggled([[maybe_unused]]bool checked) {
    m_activator->setEventType(ui->rbEvtDblClick->isChecked() ? LC_MenuActivator::DOUBLE_CLICK : LC_MenuActivator::CLICK_RELEASE);
    updateShortcutView();
}

void LC_DlgMenuAssigner::onBtnTypeToggled([[maybe_unused]]bool checked) {
    LC_MenuActivator::Button type = LC_MenuActivator::RIGHT;
    if (ui->rbBtnTask->isChecked()) {
        type = LC_MenuActivator::TASK;
    }
    else if (ui->rbBtnBack->isChecked()) {
        type = LC_MenuActivator::BACK;
    }
    else if (ui->rbBtnForward->isChecked()) {
        type = LC_MenuActivator::FORWARD;
    }
    else if (ui->rbBtnMiddle->isChecked()) {
        type = LC_MenuActivator::MIDDLE;
    }
    else if (ui->rbBtnRight->isChecked()) {
        type = LC_MenuActivator::RIGHT;
    }
    else if (ui->rbBtnLeft->isChecked()) {
        type = LC_MenuActivator::LEFT;
    }
    m_activator->setButtonType(type);
    updateShortcutView();
}
