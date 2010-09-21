#include "qg_moverotateoptions.h"

#include <qvariant.h>
#include "qg_moverotateoptions.ui.h"
/*
 *  Constructs a QG_MoveRotateOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_MoveRotateOptions::QG_MoveRotateOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_MoveRotateOptions::~QG_MoveRotateOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_MoveRotateOptions::languageChange()
{
    retranslateUi(this);
}

