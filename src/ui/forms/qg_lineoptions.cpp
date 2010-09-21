#include "qg_lineoptions.h"

#include <qvariant.h>
#include "qg_lineoptions.ui.h"
/*
 *  Constructs a QG_LineOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LineOptions::QG_LineOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LineOptions::~QG_LineOptions()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LineOptions::languageChange()
{
    retranslateUi(this);
}

