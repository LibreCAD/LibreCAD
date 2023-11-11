#ifndef LC_PRINTING_H
#define LC_PRINTING_H

#include <QPageSize>

#include "rs.h"

namespace LC_Printing
{
    QPageSize::PageSizeId rsToQtPaperFormat(RS2::PaperFormat f);
}

#endif // LC_PRINTING_H
