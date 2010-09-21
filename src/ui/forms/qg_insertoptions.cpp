#include "qg_insertoptions.h"

#include <qvariant.h>
#include "qg_insertoptions.ui.h"
/*
 *  Constructs a QG_InsertOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_InsertOptions::QG_InsertOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_InsertOptions::~QG_InsertOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_InsertOptions::languageChange()
{
    retranslateUi(this);
}

