#include "qg_polylineoptions.h"

#include <qvariant.h>
#include "qg_polylineoptions.ui.h"
/*
 *  Constructs a QG_PolylineOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_PolylineOptions::QG_PolylineOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_PolylineOptions::~QG_PolylineOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_PolylineOptions::languageChange()
{
    retranslateUi(this);
}

