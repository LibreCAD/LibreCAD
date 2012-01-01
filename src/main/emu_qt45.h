/*
 * Workaround: Emulate Qt 4.5 functions.
 */

#include <QtGlobal>

#if QT_VERSION < 0x040500

#include <QList>

template <class T>
void emu_qt45_QList_append(QList<T>& list, const QList<T>& value)
{
    QList<T>::const_iterator it;
    
    for (it = value.constBegin(); it != value.constEnd(); ++it)
        list.append(*it);
}

int emu_qt45_QInputDialog_getInt(QWidget * parent, 
                                 const QString & title, 
                                 const QString & label, 
                                 int value = 0, 
                                 int min = -2147483647, 
                                 int max = 2147483647, 
                                 int step = 1, 
                                 bool * ok = 0, 
                                 Qt::WindowFlags flags = 0);

#endif // QT_VERSION 0x040500
