#ifndef LC_APPLICATION_H
#define LC_APPLICATION_H

#include <QApplication>

class QStringList;

class LC_Application : public QApplication
{
public:
    LC_Application(int &argc, char **argv);
    bool event(QEvent *event);

    QStringList const& fileList() const;
private:
    QStringList files;
};

#endif // LC_APPLICATION_H
