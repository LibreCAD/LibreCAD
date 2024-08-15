#include "lc_actiongroup.h"

LC_ActionGroup::LC_ActionGroup(QObject *parent, const QString &name, const QString &description, const char* iconName)
    :QActionGroup(parent) {
    setObjectName(name);
    this->name = name;
    this->description = description;
    if (iconName != nullptr){
        icon = QIcon(iconName);
    }
}

LC_ActionGroup::~LC_ActionGroup() {
}

const QString &LC_ActionGroup::getName() const {
    return name;
}

void LC_ActionGroup::setName(const QString &name) {
    LC_ActionGroup::name = name;
}

const QString &LC_ActionGroup::getDescription() const {
    return description;
}

void LC_ActionGroup::setDescription(const QString &description) {
    LC_ActionGroup::description = description;
}

const QIcon &LC_ActionGroup::getIcon() const {
    return icon;
}

void LC_ActionGroup::setIcon(const QIcon &icon) {
    LC_ActionGroup::icon = icon;
}

bool LC_ActionGroup::isActionMappingsMayBeConfigured() const {
    return actionMappingsMayBeConfigured;
}

void LC_ActionGroup::setActionMappingsMayBeConfigured(bool actionMappingsMayBeConfigured) {
    LC_ActionGroup::actionMappingsMayBeConfigured = actionMappingsMayBeConfigured;
}
