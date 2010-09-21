#include "qg_lineparalleloptions.h"

#include <qvariant.h>
#include "qg_lineparalleloptions.ui.h"
/*
 *  Constructs a QG_LineParallelOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LineParallelOptions::QG_LineParallelOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LineParallelOptions::~QG_LineParallelOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LineParallelOptions::languageChange()
{
    retranslateUi(this);
}

