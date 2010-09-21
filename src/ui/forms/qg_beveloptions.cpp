#include "qg_beveloptions.h"

#include <qvariant.h>
#include "qg_beveloptions.ui.h"
/*
 *  Constructs a QG_BevelOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_BevelOptions::QG_BevelOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_BevelOptions::~QG_BevelOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_BevelOptions::languageChange()
{
    retranslateUi(this);
}

