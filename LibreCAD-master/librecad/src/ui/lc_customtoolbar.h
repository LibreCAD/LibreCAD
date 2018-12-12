#ifndef LC_CUSTOMTOOLBAR_H
#define LC_CUSTOMTOOLBAR_H

#include <QToolBar>
#include <QMap>

class LC_CustomToolbar : public QToolBar
{
    Q_OBJECT

public:

    LC_CustomToolbar(QWidget* parent);
    ~LC_CustomToolbar();

    void actions_from_file(const QString& path,
                           QMap<QString, QAction*>& a_map);
    void add_separator();

    QStringList state_list;
    QString file_path;
    QAction* most_recent_action=nullptr;

public slots:

    void slot_add_or_remove_action();
    void slot_most_recent_action(QAction* q_action);

};

#endif // LC_CUSTOMTOOLBAR_H
