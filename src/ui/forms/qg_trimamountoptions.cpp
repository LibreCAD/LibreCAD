#include "qg_trimamountoptions.h"

#include <qvariant.h>
#include "qg_trimamountoptions.ui.h"
/*
 *  Constructs a QG_TrimAmountOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_TrimAmountOptions::QG_TrimAmountOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_TrimAmountOptions::~QG_TrimAmountOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_TrimAmountOptions::languageChange()
{
    retranslateUi(this);
}

