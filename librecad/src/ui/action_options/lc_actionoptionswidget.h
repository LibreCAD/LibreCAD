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

#ifndef LC_ACTIONOPTIONSWIDGET_H
#define LC_ACTIONOPTIONSWIDGET_H

#include <QWidget>
#include "rs.h"

class RS_ActionInterface;

/**
 * Utility base class for widgets that represents options for actions.
 * Method contains several utility methods as well as default workflows, and it's purpose is
 * simply creation of options UI and reduce code repetition there.
 */
class LC_ActionOptionsWidget:public QWidget
{
    Q_OBJECT

public:
    explicit LC_ActionOptionsWidget(QWidget *parent = nullptr, Qt::WindowFlags fl = {});
    ~LC_ActionOptionsWidget();
    void setAction(RS_ActionInterface * a, bool update = false);
    /**
     * Called externally when the widget should be hidded
     */
    virtual void hideOptions();

    /**
     * Extension point. Method allows action to request update of UI (say, by enabling or hiding some parts of the widget).
     * Mode value is action-specific and should be processed by related option widget.
     * @param mode
     */
    virtual void updateUI([[maybe_unused]]int mode){};
protected:
    /**
     * Default workflow for saving settings values
     */
    virtual void saveSettings();
    /**
     * Extension point for actuall saving of settings
     */
    virtual void doSaveSettings(){};
    /**
     * Setter for corresponding action
     * @param a action
     * @param update true if options widget should be updated by action values, false - loading from settings
     */
    virtual void doSetAction(RS_ActionInterface* a, bool update) = 0;

    /**
     * Performs check that provided action type is accepted by options widget
     * @param actionType type of action
     * @return true if type is ok, false otherwise
     */
    virtual bool checkActionRttiValid(RS2::ActionType actionType);

    /**
     * Default name for settings group name
     * @return name of group
     */
    virtual QString getSettingsGroupName(){return "Draw";};

    /**
     * Default name for prefix for settings. It assumes that all settings for the action starts with the same prefix.
     * @return  prefix to use.
     */
    virtual QString getSettingsOptionNamePrefix(){ return "";}



    // saving settings shortcut methods
    void save(QString name, QString value);
    void save(QString name, int value);
    void save(QString name, bool value);

    // loading settings shortcut methods
    QString load(QString name, QString defaultValue);
    int loadInt(QString name, int defaultValue);
    bool loadBool(QString name, bool defaultValue);

    // conversion utilities
    QString fromDouble(double value);
    bool toDouble(const QString &strValue, double &res, double notMeaningful, bool positiveOnly);
    bool toDoubleAngle(const QString &strValue, double &res, double notMeaningful, bool positiveOnly);

protected slots:
    virtual void languageChange() {}

};

#endif // LC_ACTIONOPTIONSWIDGET_H
