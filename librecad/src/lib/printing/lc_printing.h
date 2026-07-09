#ifndef LC_PRINTING_H
#define LC_PRINTING_H

#include <QPrinter>

#include "rs.h"
#include "rs_vector.h"

namespace LC_Printing
{
    QPrinter::PageSize rsToQtPaperFormat(RS2::PaperFormat f);
    void setupPageLayout(QPrinter& printer, bool landscape, QPrinter::PageSize paperSizeName,
                         const RS_Vector& paperSize, RS2::Unit unit, const QMarginsF& paperMargins);
}

#endif // LC_PRINTING_H
