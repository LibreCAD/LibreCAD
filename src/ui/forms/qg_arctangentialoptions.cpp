#include "qg_arctangentialoptions.h"

#include <qvariant.h>
#include "rs_settings.h"
#include "qg_arctangentialoptions.ui.h"
/*
 *  Constructs a QG_ArcTangentialOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_ArcTangentialOptions::QG_ArcTangentialOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_ArcTangentialOptions::~QG_ArcTangentialOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_ArcTangentialOptions::languageChange()
{
    retranslateUi(this);
}

