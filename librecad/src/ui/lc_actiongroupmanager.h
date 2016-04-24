#ifndef LC_ACTIONGROUPMANAGER_H
#define LC_ACTIONGROUPMANAGER_H

#include <QObject>
#include <QList>

class QActionGroup;

class LC_ActionGroupManager : public QObject
{
    Q_OBJECT

public:
    explicit LC_ActionGroupManager(QObject* parent);

    QActionGroup* block;
    QActionGroup* circle;
    QActionGroup* curve;
    QActionGroup* edit;
    QActionGroup* ellipse;
    QActionGroup* file;
    QActionGroup* dimension;
    QActionGroup* info;
    QActionGroup* layer;
    QActionGroup* line;
    QActionGroup* modify;
    QActionGroup* options;
    QActionGroup* other;
    QActionGroup* polyline;
    QActionGroup* select;
    QActionGroup* snap;
    QActionGroup* view;
    QActionGroup* widgets;

    QList<QActionGroup*> toolGroups();
};

#endif // LC_ACTIONGROUPMANAGER_H
