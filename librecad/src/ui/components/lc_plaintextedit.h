/****************************************************************************
*
* Extended QSTextBrowser with
*
Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/
#ifndef LC_PLAINTEXTEDIT_H
#define LC_PLAINTEXTEDIT_H

#include <QMouseEvent>
#include <QTextBrowser>
#include "rs_debug.h"

class LC_PlainTextEdit:public QTextBrowser {
Q_OBJECT

private:
    QString clickedAnchor;

public:
    explicit LC_PlainTextEdit(QWidget *parent = 0):QTextBrowser(parent){
    }

public:

    void mousePressEvent(QMouseEvent *e) override{
        clickedAnchor = (e->button() & Qt::LeftButton) ? anchorAt(e->pos()) :
                        QString();
        QTextBrowser::mousePressEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent *e) override{
        if (e->button() & Qt::LeftButton && !clickedAnchor.isEmpty() &&
            anchorAt(e->pos()) == clickedAnchor){
            emit linkActivated(clickedAnchor);
        }

        QTextBrowser::mouseReleaseEvent(e);
    }

signals:
    void linkActivated(QString);
    void unhighlighted();

protected:
    void mouseMoveEvent(QMouseEvent *e) override{
        const QString &anchor = anchorAt(e->pos());
        if (anchor.isEmpty()){
//            LC_ERR<<__func__<<"(): NO Anchor" ;
           emit unhighlighted();
        }
        else{
//            LC_ERR<<__func__<<"(): Anchor" + anchor;
        }
        QTextBrowser::mouseMoveEvent(e);
    }
};

#endif // LC_PLAINTEXTEDIT_H
