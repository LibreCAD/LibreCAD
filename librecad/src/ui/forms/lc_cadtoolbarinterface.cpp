#include <QMouseEvent>
#include <QGridLayout>
#include <QActionGroup>
#include <QIcon>
#include <QToolButton>
#include <QToolBar>
#include "lc_cadtoolbarinterface.h"
#include "qg_cadtoolbar.h"
#include "qg_actionhandler.h"
#include "rs_debug.h"

LC_CadToolBarInterface::LC_CadToolBarInterface(QG_CadToolBar* _parentTB, Qt::WindowFlags fl):
	QWidget(_parentTB, fl)
  ,cadToolBar(_parentTB)
  ,actionHandler(nullptr)
  ,m_pHidden(new QAction("ActionHidden", this))
  ,m_pGrid0(new QToolBar)
  ,m_pGrid1(new QToolBar)
  ,m_pActionGroup(new QActionGroup(this))
{
}

void LC_CadToolBarInterface::initToolBars()
{
	switch(rtti()){
	case RS2::ToolBarSelect:
        m_pButtonForward = new QAction( QIcon(":/extui/forward.png"), "Continue", this);
		//continue to default, no break by design
	default:
		m_pButtonBack = new QAction(QIcon(":/extui/back.png"), "Back", this);
	case RS2::ToolBarMain:
		break;
	}
	setStyleSheet("QToolBar{ margin: 0px }");
	setContentsMargins(0,0,0,0);
//	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	for(auto p: {m_pGrid0, m_pGrid1}){
		p->setFloatable(false);
		p->setMovable(false);
		p->setOrientation(Qt::Vertical);
		p->setContentsMargins(0,0,0,0);
	}

	m_pActionGroup->setExclusive(true);
	m_pHidden->setCheckable(true);
	m_pHidden->setChecked(true);
	m_pActionGroup->addAction(m_pHidden);

	QHBoxLayout* hLayout=new QHBoxLayout;
	hLayout->addWidget(m_pGrid0);
	hLayout->addWidget(m_pGrid1);
	hLayout->setSpacing(1);
	hLayout->setContentsMargins(0,0,0,0);

	QVBoxLayout* vLayout=new QVBoxLayout;
	vLayout->setSpacing(1);
	vLayout->setContentsMargins(0,0,0,0);
	if(m_pButtonBack){
		QToolButton* button=new QToolButton;
		button->setDefaultAction(m_pButtonBack);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		vLayout->addWidget(button);
		connect(m_pButtonBack, SIGNAL(triggered()), cadToolBar, SLOT(back()));
	}
	vLayout->addLayout(hLayout);
    if(rtti()!=RS2::ToolBarSelect)
        vLayout->addStretch(1);

	setLayout(vLayout);
}

void LC_CadToolBarInterface::setActionHandler(QG_ActionHandler* ah)
{
	actionHandler=ah;
}

void LC_CadToolBarInterface::finishCurrentAction(bool resetToolBar)
{
	if(!actionHandler) return;
	RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
	if(currentAction) {
		currentAction->finish(resetToolBar); //finish the action, but do not update toolBar
	}
}


void LC_CadToolBarInterface::killSelectActions()
{
	if(!actionHandler) return;
	actionHandler->killSelectActions();
}

void LC_CadToolBarInterface::killAllActions()
{
	if(!actionHandler) return;
	actionHandler->killAllActions();
}

QSize LC_CadToolBarInterface::sizeHint() const
{
    return QSize(-1,-1);
}


void LC_CadToolBarInterface::mousePressEvent(QMouseEvent* e) {
	if (e->button()==Qt::RightButton && cadToolBar) {
		finishCurrentAction(true);
		cadToolBar->showPreviousToolBar(true);
		e->accept();
	}
}

void LC_CadToolBarInterface::back()
{
	finishCurrentAction(true);
	if (cadToolBar) {
		cadToolBar->showPreviousToolBar(true);
	}
}


void LC_CadToolBarInterface::addSubAction(QAction*const action, bool addGroup)
{
	RS_DEBUG->print("LC_CadToolBarInterface::addSubAction(): begin\n");
	switch(rtti()){
	case RS2::ToolBarMain:
		action->setCheckable(false);
		break;
	default:
		action->setCheckable(true);
	}
	if(actions0>actions1){
		m_pGrid1->addAction(action);
		++actions1;
	}else{
		m_pGrid0->addAction(action);
		++actions0;
	}

	if(addGroup) m_pActionGroup->addAction(action);
	RS_DEBUG->print("LC_CadToolBarInterface::addSubAction(): end\n");

}


void LC_CadToolBarInterface::addSubActions(const std::vector<QAction*>& actions, bool addGroup)
{
	for(auto p: actions){
		this->addSubAction(p, addGroup);
    }
    resize(cadToolBar->size());
}
