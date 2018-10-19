
/****************************************************************************
*  dividedlg.h - divide lines, circles and arcs                             *
*                                                                           *
*  Copyright (C) 2018 mad-hatter                                            *
*                                                                           *
*  This library is free software, licensed under the terms of the GNU       *
*  General Public License as published by the Free Software Foundation,     *
*  either version 2 of the License, or (at your option) any later version.  *
*  You should have received a copy of the GNU General Public License        *
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
****************************************************************************/

#ifndef DIVIDEDLG_H
#define DIVIDEDLG_H

#include <QDialog>
#include <QRadioButton>
#include <QLineEdit>
#include <QSpinBox>

#include "document_interface.h"

class dividedlg : public QDialog
{
    Q_OBJECT

public:
    dividedlg( Document_Interface *doc, QString,
               QWidget *parent = nullptr );
    ~dividedlg();

protected:
    bool eventFilter( QObject *, QEvent * );

public slots:
    void onWhichButtonSlot( bool );
    void onOffTicksSlot( bool );
    void onOffBreaksSlot( bool );
    void onSizeChangedSlot( int );
    void onQtyChangedSlot( int );
    void onStartAngleChangedSlot( const QString & );
    void onOkClickedSlot();
    void onInOutSlot( bool );

signals:
    void returnData( QString );

private:
    void choice( int, int, QFont );
    Document_Interface *d;
    QRadioButton *R1; //on/off ticks
    QRadioButton *R2; //on/off breaks
    QLineEdit *Le1; //circle - start angle
    QLineEdit *Le2; //new layer
    QSpinBox *Sp1; //qty
    QSpinBox *Sp2; //size
    QString dataToReturn;
    bool ticksShowHideFlag;
    bool breaksOnOffFlag;
    bool backSpace;
    bool inOut; //tick position in/out - above/below
    int size; //%
    int qty; //ticks and breaks
    int activeLayer;
};

#endif // end DIVIDEDLG_H
