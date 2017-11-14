#include <QStringList>
#include <QFileOpenEvent>

#include "lc_application.h"

LC_Application::LC_Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
    QStringList files = QStringList();
}

// This is only used until the event filter is in place in mainwindow
bool LC_Application::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
        files.append(openEvent->file());
    }
    return QApplication::event(event);
}

QStringList const& LC_Application::fileList() const {
    return files;
}
