#include "qg_dimlinearoptions.h"

#include <qvariant.h>
#include "qg_dimlinearoptions.ui.h"
/*
 *  Constructs a QG_DimLinearOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_DimLinearOptions::QG_DimLinearOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DimLinearOptions::~QG_DimLinearOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DimLinearOptions::languageChange()
{
    retranslateUi(this);
}

