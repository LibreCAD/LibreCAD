/*
 * Workaround: Emulate Qt 4.4 functions.
 */


#include <QtGlobal>

#if QT_VERSION < 0x040400

#include <QList>
#include <QString>
#include <QFileDialog>
#include <QPrinter>


QString emu_qt44_storageLocationDocuments(void);
QString emu_qt44_storageLocationData(void);

inline void emu_qt44_QFileDialog_setNameFilter(QFileDialog& fd, const QString &s)
{ fd.setFilter(s); }

inline void emu_qt44_QFileDialog_setNameFilters(QFileDialog& fd, const QStringList &sl)
{ fd.setFilters(sl); }

inline void emu_qt44_QPrinter_setPaperSize(QPrinter &p, QPrinter::PageSize s)
{ p.setPageSize(s); }

template<class T>
bool emu_qt44_removeOne(QList<T>& list, const T& value)
{
    int index;

    index = list.indexOf(value);
    if (index == -1)
        return false;
    else {
        list.removeAt(index);
        return true;
    }
}

#endif // QT_VERSION 0x040400
