/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
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
#include "qg_mousewidget.h"

#include "lc_modifiersinfo.h"
#include "lc_shortcuts_manager.h"
#include "rs_settings.h"

/*
 *  Constructs a QG_MouseWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_MouseWidget::QG_MouseWidget(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, fl){
    setObjectName(name);
    setupUi(this);
    bool useClassicalStatusBar = LC_GET_ONE_BOOL("Startup", "UseClassicStatusBar", false);

    LC_GROUP_GUARD("Widgets");
    {
        bool custom_size = LC_GET_BOOL("AllowToolbarIconSize", false);
        iconSize = custom_size ? LC_GET_INT("ToolbarIconSize", 24) : 24;
        int height{64};

        if (useClassicalStatusBar) {
            int allow_statusbar_height = LC_GET_BOOL("AllowStatusbarHeight", false);
            if (allow_statusbar_height) {
                height = LC_GET_INT("StatusbarHeight", 64);
            }

            setMinimumHeight(height);
            setMaximumHeight(height);

            int halfHeight = height / 2 - 2;

            lblCtrl->setMinimumSize(halfHeight, halfHeight);
            lblCtrl->setMaximumSize(halfHeight, halfHeight);
            lblShift->setMinimumSize(halfHeight, halfHeight);
            lblShift->setMaximumSize(halfHeight, halfHeight);
        } else {
            lLeftButton->setWordWrap(false);
            lRightButton->setWordWrap(false);
            lLeftButton->setTextFormat(Qt::TextFormat::PlainText);
        }
    }

    lLeftButton->setText("");
    lRightButton->setText("");
    lMousePixmap->setPixmap(QPixmap(":/icons/mouse.lci")/*.scaled(height, height)*/);
}

void QG_MouseWidget::updatePixmap(QString iconName, QLabel *label){
    int width = label->pixmap().width();
    int height = label->pixmap().height();
    label->setPixmap(QIcon(iconName).pixmap(width, height));
}

void QG_MouseWidget::onIconsRefreshed(){
    updatePixmap(":/icons/mouse.lci", lMousePixmap);
    updatePixmap(":/icons/state-shift_yes.lci", lblShift);
    updatePixmap(":/icons/state_ctrl_yes.lci", lblCtrl);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_MouseWidget::~QG_MouseWidget(){
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_MouseWidget::languageChange(){
    retranslateUi(this);
}

void QG_MouseWidget::setHelp(const QString& left, const QString& right, const LC_ModifiersInfo& modifiersInfo) const {

    const QString &shiftMsg = modifiersInfo.getShiftMessage();
    setupModifier(lblShift, shiftMsg);

    const QString &ctrlMessage = modifiersInfo.getCtrlMessage();
    setupModifier(lblCtrl, ctrlMessage);

    QString leftText = left;
    QString rightText = right;

    lLeftButton->setText(leftText);
    lLeftButton->setToolTip(leftText);
    lRightButton->setText(rightText);
    lRightButton->setToolTip(rightText);
}

void QG_MouseWidget::setupModifier(QLabel *btn, const QString& helpMsg) const{
    if (helpMsg != nullptr){
        btn->setEnabled(true);
        btn->setToolTip(helpMsg);
    }
    else {
        btn->setEnabled(false);
        btn->setToolTip("");
    }
}

void QG_MouseWidget::clearActionIcon(){
    const QIcon icon = QIcon();
    setActionIcon(icon);
    lCurrentAction->setToolTip("");
}

void QG_MouseWidget::setActionIcon(QIcon icon) {
    lCurrentAction->setPixmap(icon.pixmap(iconSize));
}

void QG_MouseWidget::setCurrentQAction(QAction *a) {
    QIcon icon;
    if (a != nullptr){
        icon = a->icon();
        QString toolTip = LC_ShortcutsManager::getPlainActionToolTip(a);
        lCurrentAction->setToolTip(tr("Current Action:")+ " " + toolTip);
    }

    setActionIcon(icon);
}
