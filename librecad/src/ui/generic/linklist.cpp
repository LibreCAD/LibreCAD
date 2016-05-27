#include "linklist.h"

#include <QDesktopServices>
#include <QUrl>

LinkList::LinkList(QWidget* parent)
    : QListWidget(parent)
{

    connect(this, SIGNAL(itemActivated(QListWidgetItem*)),
            this, SLOT(showWebPage(QListWidgetItem*)));
}

void LinkList::addLink(const QString& text, const QString& url)
{
    auto item = new QListWidgetItem;
    item->setText(text);
    item->setToolTip(url);
    addItem(item);
}

void LinkList::showWebPage(QListWidgetItem* item)
{
    QDesktopServices::openUrl(QUrl(item->toolTip()));
}
