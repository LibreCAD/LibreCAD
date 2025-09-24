/****************************************************************************
**
* Utility base class for widgets that represents options for actions

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

#include "lc_actionoptionswidget.h"

#include <QLineEdit>
#include <QToolButton>

#include "lc_actioncontext.h"
#include "lc_convert.h"
#include "lc_linemath.h"
#include "rs_actioninterface.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "rs_settings.h"

class LC_LateCompletionRequestor;

LC_ActionOptionsWidget::LC_ActionOptionsWidget(QWidget *parent, Qt::WindowFlags fl) :
    QWidget(parent, fl) {
    m_interactiveInputControlsAutoRaise = LC_GET_ONE_BOOL("Widgets", "PickValueButtonsFlatIcons", true);
    m_interactiveInputControlsVisible = LC_GET_ONE_BOOL("Defaults", "InteractiveInputEnabled", true);
}

LC_ActionOptionsWidget::~LC_ActionOptionsWidget() = default;

/**
 * Method is called from the action when options should be hidden.
 * Simply hides UI and saves settings.
 */
void LC_ActionOptionsWidget::hideOptions(){
    hide();
    saveSettings();
}

/**
 * Setter for the action. Checks that action rtti type is valid and delegates actual processing to doSetAction.
 * doSetAction() is wrapped in beginGroup/endGroup for settings, and the name of group is defined by
 * getSettingsGroupName() method.
 * @param a action
 * @param update true if option values should be updated by the action, false - if they should be read from settings
 */
void LC_ActionOptionsWidget::setAction(RS_ActionInterface *a, bool update){
    if (a != nullptr){
        RS2::ActionType actionType = a->rtti();
        if (checkActionRttiValid(actionType)){
            // that should be ok for the most of the actions as most probably they will rely on the same group
            LC_GROUP(getSettingsGroupName());
            m_actionContext = a->getActionContext();
            m_laterCompletionRequestor = dynamic_cast<LC_LateCompletionRequestor*>(a);
            doSetAction(a, update);
            LC_GROUP_END();
        }
        else{
            RS_DEBUG->print(RS_Debug::D_ERROR, typeid(*this).name(), "::setAction: wrong action type");
        }
    }
    else {
        m_laterCompletionRequestor = nullptr;
        m_actionContext = nullptr;
    }
}

/**
 * Extension point for specific widgets to check that action type is valid
 * @param actionType rtti type of action
 * @return true if action is valid
 */
bool LC_ActionOptionsWidget::checkActionRttiValid([[maybe_unused]]RS2::ActionType actionType){
    return false;
}

/**
 * Generic method for saving settings. Simply delegates actual saving of options to doSaveSettings (that should be implemented in inherited widget),
 * by wrapping it by settings begin/end group calls.
 */
void LC_ActionOptionsWidget::saveSettings(){
    LC_GROUP(getSettingsGroupName());
    doSaveSettings();
    LC_GROUP_END();
}

/**
 * Just utility method for conversion of provided string to double value.
 * @param strValue string that represents value
 * @param res result of conversion
 * @param notMeaningful value that should be returned if given string denotes non-meaningful value (less than tolerance)
 * @param positiveOnly if true, positive value (via std::abs()) will be always returned, false - otherwise.
 * @return true if string was converted without errors
 */
bool LC_ActionOptionsWidget::toDouble(const QString& strValue, double &res, double notMeaningful, bool positiveOnly){
    bool ok = false;
    double x = RS_Math::eval(strValue, &ok);
    if(ok){
        res = LC_LineMath::getMeaningful(x, notMeaningful);
        if (positiveOnly){
            res = std::abs(res);
        }
    }
    return ok;
}

/**
 * Just utility method for conversion of provided string to double value, given that double is for angle
 * @param strValue string that represents value
 * @param res result of conversion
 * @param notMeaningful value that should be returned if given string denotes non-meaningful value (less than tolerance)
 * @param positiveOnly if true, positive value (via std::abs()) will be always returned, false - otherwise.
 * @return true if string was converted without errors
 */
bool LC_ActionOptionsWidget::toDoubleAngleDegrees(const QString& strValue, double &res, double notMeaningful, bool positiveOnly){
    bool ok = LC_Convert::parseToToDoubleAngleDegrees(strValue, res, notMeaningful, positiveOnly);
    return ok;
}

/**
 * Utility method for version of double value to string that will be displayed in editor
 * @param value double value
 * @return corresponding string
 */
QString LC_ActionOptionsWidget::fromDouble(double value){
    // here we'll convert to decimal degrees always...regardless of original input format.
    if (LC_LineMath::isNotMeaningful(value)){
        value = 0.0;
    }
    return QString::number(value, 'g', 6);
}

/**
 * Utility method that loads setting value. Default implementation assumes that all settings for the action
 * has the same prefix, and this method creates full name of settings based on prefix returned by getSettingsOptionNamePrefix()
 * @param name short name of setting (without prefix)
 * @param defaultValue default value that should be used
 * @return setting value
 */
QString LC_ActionOptionsWidget::load(QString name, QString defaultValue){
    QString key = getSettingsOptionNamePrefix() + name;
    return LC_GET_STR(key, defaultValue);
}

/**
 * Utility method that loads int settings value
 * @param name
 * @param defaultValue
 * @return
 */
int LC_ActionOptionsWidget::loadInt(QString name, int defaultValue){
    QString key = getSettingsOptionNamePrefix() + name;
    return LC_GET_INT(key, defaultValue);
}

bool LC_ActionOptionsWidget::loadBool(QString name, bool defaultValue){
    QString key = getSettingsOptionNamePrefix() + name;
    return LC_GET_INT(key, defaultValue ? 1 : 0) == 1;
}

void LC_ActionOptionsWidget::save(QString name, QString value){
    QString key = getSettingsOptionNamePrefix() + name;
    LC_SET(key, value);
}

void LC_ActionOptionsWidget::save(QString name, int value){
    QString key = getSettingsOptionNamePrefix() + name;
    LC_SET(key, value);
}

void LC_ActionOptionsWidget::save(QString name, bool value){
    QString key = getSettingsOptionNamePrefix() + name;
    LC_SET(key, value ? 1 : 0);
}

void LC_ActionOptionsWidget::connectInteractiveInputButton(QToolButton* button,
                                                           LC_ActionContext::InteractiveInputInfo::InputType
                                                           inputType, QString tag) {
    if (m_interactiveInputControlsVisible) {
        button->setVisible(true);
        button->setProperty("_interactiveInputButton", inputType);
        button->setProperty("_interactiveInputTag", tag);
        button->connect(button, &QToolButton::clicked, this, &LC_ActionOptionsWidget::onInteractiveInputButtonClicked);
        button->setAutoRaise(m_interactiveInputControlsAutoRaise);
    }
    else {
        button->setVisible(false);
    }
}

void LC_ActionOptionsWidget::requestFocusForTag(const QString& tag) {
    auto widgets = findChildren<QWidget*>();
    for (auto le:widgets) {
        auto tagProperty = le->property("_tagHolder");
        if (tagProperty.isValid() && !tagProperty.isNull()) {
            QString editTag = tagProperty.toString();
            if (tag == editTag) {
                le->setFocus();
                break;
            }
        }
    }
}

void LC_ActionOptionsWidget::pickDistanceSetup(QString tag, QToolButton* button, QLineEdit* lineedit) {
    connectInteractiveInputButton(button, LC_ActionContext::InteractiveInputInfo::DISTANCE, tag);
    lineedit->setProperty("_tagHolder", tag);
}

void LC_ActionOptionsWidget::pickAngleSetup(QString tag, QToolButton* button, QLineEdit* lineedit) {
    connectInteractiveInputButton(button, LC_ActionContext::InteractiveInputInfo::ANGLE, tag);
    lineedit->setProperty("_tagHolder", tag);
}

void LC_ActionOptionsWidget::onInteractiveInputButtonClicked([[maybe_unused]]bool checked) {
    auto senderButton = dynamic_cast<QToolButton*>(sender());
    if (senderButton != nullptr) {
        auto property = senderButton->property ("_interactiveInputButton");
        if (property.isValid()) {
            auto inputType = static_cast<LC_ActionContext::InteractiveInputInfo::InputType>(property.toInt());
            auto tagProperty = senderButton->property ("_interactiveInputTag");
            QString tag = tagProperty.toString();
            switch (inputType) {
                case LC_ActionContext::InteractiveInputInfo::DISTANCE:
                case LC_ActionContext::InteractiveInputInfo::ANGLE: {
                    m_actionContext->interactiveInputStart(inputType, m_laterCompletionRequestor, tag);
                    break;
                }
                case LC_ActionContext::InteractiveInputInfo::POINT: {
                    // NOTE: point is not supported. Potentially, this is developer's error....
                    break;
                }
                default:
                    break;
            }
        }
    }
}
