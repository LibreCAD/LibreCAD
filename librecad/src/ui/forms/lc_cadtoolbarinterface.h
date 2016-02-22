#ifndef LC_CADTOOLBARINTERFACE_H
#define LC_CADTOOLBARINTERFACE_H

#include <QWidget>
#include "rs.h"

class QToolBar;
class QG_CadToolBar;
class QG_ActionHandler;
class RS_ActionInterface;
class QAction;
class QGridLayout;
class QActionGroup;

class LC_CadToolBarInterface: public QWidget
{
public:
	LC_CadToolBarInterface() = delete;
	LC_CadToolBarInterface(QG_CadToolBar* _parentTB, Qt::WindowFlags fl = 0);
	virtual ~LC_CadToolBarInterface()=default;

	virtual RS2::ToolBarId rtti() const = 0;
	//! \{ \brief populate toolbar with QAction buttons
	void addSubAction(QAction*const action, bool addGroup=true);
	virtual void addSubActions(const std::vector<QAction*>& actions, bool addGroup=true);
	//! \}
	virtual void setActionHandler(QG_ActionHandler* ah);
	void finishCurrentAction(bool resetToolBar=false); //clear current action
	void killSelectActions();
	void killAllActions();

	virtual void resetToolBar() {}
	virtual void runNextAction() {}
	virtual void restoreAction() {} //restore action from checked button
	virtual void showCadToolBar(RS2::ActionType /*actionType*/) {}
	virtual void setSelectAction( RS_ActionInterface * /*selectAction*/ ) {}
	virtual void setNextAction( int /*nextAction*/ ) {}
    virtual QSize 	sizeHint() const;

public slots:
	virtual void back();
	virtual void mousePressEvent( QMouseEvent * e );
//	virtual void contextMenuEvent( QContextMenuEvent * e );

protected:
	QG_CadToolBar* cadToolBar;
	QG_ActionHandler* actionHandler;

	QAction* m_pButtonBack=nullptr;
	QAction* m_pButtonForward=nullptr;
	QAction* m_pHidden;
	QToolBar *m_pGrid0, *m_pGrid1;
	size_t actions0=0, actions1=0;
	QActionGroup* m_pActionGroup;
	void initToolBars();
};

#endif // LC_CADTOOLBARINTERFACE_H
