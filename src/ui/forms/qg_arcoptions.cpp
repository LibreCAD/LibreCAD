#include "qg_arcoptions.h"

#include <qvariant.h>
#include "rs_settings.h"
#include "qg_arcoptions.ui.h"
/*
 *  Constructs a QG_ArcOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_ArcOptions::QG_ArcOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_ArcOptions::~QG_ArcOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_ArcOptions::languageChange()
{
    retranslateUi(this);
}

