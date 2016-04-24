#include "lc_actiongroupmanager.h"

#include <QActionGroup>

LC_ActionGroupManager::LC_ActionGroupManager(QObject* parent)
    : QObject(parent)
    , block(new QActionGroup(this))
    , circle(new QActionGroup(this))
    , curve(new QActionGroup(this))
    , edit(new QActionGroup(this))
    , ellipse(new QActionGroup(this))
    , file(new QActionGroup(this))
    , dimension(new QActionGroup(this))
    , info(new QActionGroup(this))
    , layer(new QActionGroup(this))
    , line(new QActionGroup(this))
    , modify(new QActionGroup(this))
    , options(new QActionGroup(this))
    , other(new QActionGroup(this))
    , polyline(new QActionGroup(this))
    , select(new QActionGroup(this))
    , snap(new QActionGroup(this))
    , view(new QActionGroup(this))
    , widgets(new QActionGroup(this))
{
    block->setObjectName(QObject::tr("Block"));
    circle->setObjectName(QObject::tr("Circle"));
    curve->setObjectName(QObject::tr("Curve"));
    edit->setObjectName(QObject::tr("Edit"));
    ellipse->setObjectName(QObject::tr("Ellipse"));
    file->setObjectName(QObject::tr("File"));
    dimension->setObjectName(QObject::tr("Dimension"));
    info->setObjectName(QObject::tr("Info"));
    layer->setObjectName(QObject::tr("Layer"));
    line->setObjectName(QObject::tr("Line"));
    modify->setObjectName(QObject::tr("Modify"));
    options->setObjectName(QObject::tr("Options"));
    other->setObjectName(QObject::tr("Other"));
    polyline->setObjectName(QObject::tr("Polyline"));
    select->setObjectName(QObject::tr("Select"));
    snap->setObjectName(QObject::tr("Snap"));
    view->setObjectName(QObject::tr("View"));
    widgets->setObjectName(QObject::tr("Widgets"));

    foreach (auto ag, findChildren<QActionGroup*>())
    {
        ag->setExclusive(false);
        if (ag->objectName() != QObject::tr("File"))
        {
            connect(parent, SIGNAL(windowsChanged(bool)),
                    ag, SLOT(setEnabled(bool)));
        }
    }

    foreach (auto ag, toolGroups())
    {
        connect(ag, SIGNAL(triggered(QAction*)),
                parent, SLOT(relayAction(QAction*)));
    }
}

QList<QActionGroup*> LC_ActionGroupManager::toolGroups()
{
    QList<QActionGroup*> ag_list;
    ag_list << block
            << circle
            << curve
            << ellipse
            << dimension
            << info
            << line
            << modify
            << polyline
            << select;

    return ag_list;
}
