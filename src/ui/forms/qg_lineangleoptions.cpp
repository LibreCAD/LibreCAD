#include "qg_lineangleoptions.h"

#include <qvariant.h>
#include "qg_lineangleoptions.ui.h"
/*
 *  Constructs a QG_LineAngleOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LineAngleOptions::QG_LineAngleOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LineAngleOptions::~QG_LineAngleOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LineAngleOptions::languageChange()
{
    retranslateUi(this);
}

