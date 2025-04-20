/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
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

#ifndef QG_ACTIONHANDLER_H
#define QG_ACTIONHANDLER_H
#include "rs.h"
#include <QObject>

struct RS_SnapMode;
class RS_GraphicView;
class QC_ApplicationWindow;
class RS_Document;
class RS_ActionInterface;
class LC_DefaultActionContext;
class QG_SnapToolBar;
class RS_Layer;
class LC_SnapManager;

/**
 * This class can trigger actions (from menus, buttons, ...).
 */
class QG_ActionHandler : public QObject {
    Q_OBJECT
public:
    explicit QG_ActionHandler(QC_ApplicationWindow *parent);
    ~QG_ActionHandler() override = default;
    RS_ActionInterface *getCurrentAction() const;
    std::shared_ptr<RS_ActionInterface> setCurrentAction(RS2::ActionType id, void* data = nullptr) const;
    /**
    * @brief killAllActions kill all actions
    */
    void killAllActions() const;
    bool keycode(const QString &code);
    bool command(const QString &cmd);
    QStringList getAvailableCommands() const;
    void setDocumentAndView(RS_Document* document, RS_GraphicView* graphicView);
    void setActionContext(LC_DefaultActionContext* actionContext) {m_actionContext = actionContext;};
    void setSnapManager(LC_SnapManager* snapManager);
    std::shared_ptr<RS_ActionInterface> createActionInstance(RS2::ActionType id, void* data) const;
public slots:
    void setSnaps(RS_SnapMode const &s) const;
    void slotSnapMiddleManual();
    void slotSetRelativeZero();
    void slotLockRelativeZero(bool on);
private:
    RS_GraphicView *view {nullptr};
    RS_Document* document {nullptr};
    LC_DefaultActionContext* m_actionContext{nullptr};
    LC_SnapManager* m_snapManager;
};

#endif
