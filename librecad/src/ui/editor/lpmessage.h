#ifndef LPMESSAGE_H
#define LPMESSAGE_H

#include <QObject>
#include <QWidget>
#include <QDialog>
#include <QLabel>
#include <QTimer>

#define LP_MSG_STYLE "border: 3px solid #329a77; border-radius: 4px; border-width: 2px; background: #c6ecd6; color: black; font: 10pt; padding-top: 8px; padding-right: 4px; padding-bottom: 8px;padding-left: 4px;"

#ifdef DEVELOPER

class LpMessage : public QWidget
{
    Q_OBJECT
public:
    explicit LpMessage(QWidget *parent = nullptr, const QString &message ="", int timeout=3000);
    ~LpMessage();

    static void info(const QString &info, QWidget *parent = nullptr, int timeout=3000);

protected:
    void mousePressEvent(QMouseEvent *event);

private slots:
    void closeMe();
    void init();

private:
    QTimer *m_initTimer;
    QTimer *m_acceptTimer = nullptr;
    QLabel *m_info;
    int m_res;
};

#endif // DEVELOPER

#endif // LPMESSAGE_H
