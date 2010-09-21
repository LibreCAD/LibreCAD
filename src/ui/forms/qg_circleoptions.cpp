#include "qg_circleoptions.h"

#include <qvariant.h>
#include "qg_circleoptions.ui.h"
/*
 *  Constructs a QG_CircleOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CircleOptions::QG_CircleOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CircleOptions::~QG_CircleOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CircleOptions::languageChange()
{
    retranslateUi(this);
}

