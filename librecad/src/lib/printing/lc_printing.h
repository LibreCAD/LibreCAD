#ifndef LC_PRINTING_H
#define LC_PRINTING_H

#include <QPrinter>
#include "rs.h"

namespace LC_Printing
{
    QPrinter::PageSize rsToQtPaperFormat(RS2::PaperFormat f);
}

#endif // LC_PRINTING_H
