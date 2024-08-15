#ifndef LC_ACTIONGROUP_H
#define LC_ACTIONGROUP_H

#include <QActionGroup>

class LC_ActionGroup:public QActionGroup {
public:
    LC_ActionGroup(QObject *parent, const QString &name, const QString &description, const char* icon);

    ~LC_ActionGroup() override;

    const QString &getName() const;

    void setName(const QString &name);

    const QString &getDescription() const;

    void setDescription(const QString &description);

    const QIcon &getIcon() const;

    void setIcon(const QIcon &icon);

    bool isActionMappingsMayBeConfigured() const;

    void setActionMappingsMayBeConfigured(bool actionMappingsMayBeConfigured);

protected:
    QString name;
    QString description;
    QIcon icon;
    bool actionMappingsMayBeConfigured = true;
};

#endif // LC_ACTIONGROUP_H
