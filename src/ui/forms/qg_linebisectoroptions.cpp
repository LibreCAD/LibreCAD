#include "qg_linebisectoroptions.h"

#include <qvariant.h>
#include "qg_linebisectoroptions.ui.h"
/*
 *  Constructs a QG_LineBisectorOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LineBisectorOptions::QG_LineBisectorOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LineBisectorOptions::~QG_LineBisectorOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LineBisectorOptions::languageChange()
{
    retranslateUi(this);
}

