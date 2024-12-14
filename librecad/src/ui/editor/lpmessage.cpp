#include "lpmessage.h"

#include <QVBoxLayout>

#ifdef DEVELOPER

LpMessage::LpMessage(QWidget *parent, const QString &message, int timeout)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    if (timeout > 0)
    {
        m_acceptTimer = new QTimer(this);
        connect(m_acceptTimer, &QTimer::timeout, this, &LpMessage::closeMe);
        m_acceptTimer->start(timeout);
    }

    m_info = new QLabel;
    m_info->setText(message);
    m_info->setStyleSheet(LP_MSG_STYLE);
    layout->addWidget(m_info);

    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    m_initTimer = new QTimer(this);
    connect(m_initTimer, &QTimer::timeout, this, &LpMessage::init);
    m_initTimer->start(50);
}

void LpMessage::info(const QString &info, QWidget *parent, int timeout)
{
    LpMessage *dlg = new LpMessage(parent, info, timeout);
    dlg->show();
}

LpMessage::~LpMessage()
{
    if (m_acceptTimer != nullptr)
    {
        delete m_acceptTimer;
    }
    delete m_initTimer;
    delete m_info;
}

void LpMessage::init()
{
    m_initTimer->stop();

    if (parent() != nullptr)
    {
        QWidget *pa = dynamic_cast<QWidget *>(parent());
        int x = pa->x() + pa->width() - width() - 20;
        int y = pa->y() + pa->height() - height() - 100;
        move(x,y);
    }
}

void LpMessage::closeMe()
{
    m_acceptTimer->stop();
    this->close();
}

void LpMessage::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    this->close();
}

#endif // DEVELOPER
