#include "qg_roundoptions.h"

#include <qvariant.h>
#include "qg_roundoptions.ui.h"
/*
 *  Constructs a QG_RoundOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_RoundOptions::QG_RoundOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_RoundOptions::~QG_RoundOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_RoundOptions::languageChange()
{
    retranslateUi(this);
}

