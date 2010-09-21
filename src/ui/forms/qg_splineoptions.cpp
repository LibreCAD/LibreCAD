#include "qg_splineoptions.h"

#include <qvariant.h>
#include "qg_splineoptions.ui.h"
/*
 *  Constructs a QG_SplineOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_SplineOptions::QG_SplineOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_SplineOptions::~QG_SplineOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_SplineOptions::languageChange()
{
    retranslateUi(this);
}

