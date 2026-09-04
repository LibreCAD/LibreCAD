#ifndef LC_PRINTING_H
#define LC_PRINTING_H

#include <QPageSize>
#include <QPrinter>

#include "rs.h"
#include "rs_vector.h"

namespace LC_Printing
{
    QPrinter::PageSize rsToQtPaperFormat(RS2::PaperFormat f);
    /**
     * @brief toPageSize - build a QPageSize for the given paper format. Standard
     * formats keep their identity; a custom format is defined in portrait (the
     * paper size is converted from the drawing unit to millimeters), leaving the
     * orientation to setupPageLayout().
     */
    QPageSize toPageSize(QPrinter::PageSize paperSizeName, const RS_Vector& paperSize, RS2::Unit unit);
    void setupPageLayout(QPrinter& printer, bool landscape, const QPageSize& pageSize,
                         const QMarginsF& paperMargins);
}

#endif // LC_PRINTING_H
