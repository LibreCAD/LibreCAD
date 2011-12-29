/*
 * Workaround: Emulate Qt 4.5 functions.
 */

#if QT_VERSION < 0x040500

#include <QWidget>
#include "emu_qt45.h"

int emu_qt45_QInputDialog_getInt(QWidget * parent, 
                                 const QString & title, 
                                 const QString & label, 
                                 int value,
                                 int min,
                                 int max,
                                 int step,
                                 bool * ok,
                                 Qt::WindowFlags flags)
{
    if (ok != NULL)
        *ok = false;
    return INT_MIN;
}

#endif // QT_VERSION 0x040500
