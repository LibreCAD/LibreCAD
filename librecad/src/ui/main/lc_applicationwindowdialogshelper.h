#ifndef LC_APPLICATIONWINDOWDIALOGSHELPER_H
#define LC_APPLICATIONWINDOWDIALOGSHELPER_H

#include <QObject>
class LC_ReleaseChecker;
class QC_ApplicationWindow;
class LC_ApplicationWindowDialogsHelper : public QObject{
    Q_OBJECT
public:
    LC_ApplicationWindowDialogsHelper(QC_ApplicationWindow *appWin);
    void showAboutWindow();
    void showNewVersionAvailableDialog( LC_ReleaseChecker* releaseChecker);
    void invokeLicenseWindow();
    void showDeviceOptions();
    bool widgetOptionsDialog();
    bool requestOptionsGeneralDialog();
signals:
private:
    QC_ApplicationWindow* m_appWindow;
};

#endif // LC_APPLICATIONWINDOWDIALOGSHELPER_H
