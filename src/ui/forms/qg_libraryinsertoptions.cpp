#include "qg_libraryinsertoptions.h"

#include <qvariant.h>
#include "qg_libraryinsertoptions.ui.h"
/*
 *  Constructs a QG_LibraryInsertOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LibraryInsertOptions::QG_LibraryInsertOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LibraryInsertOptions::~QG_LibraryInsertOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LibraryInsertOptions::languageChange()
{
    retranslateUi(this);
}

