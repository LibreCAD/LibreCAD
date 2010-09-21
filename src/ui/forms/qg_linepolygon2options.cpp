#include "qg_linepolygon2options.h"

#include <qvariant.h>
#include "qg_linepolygon2options.ui.h"
/*
 *  Constructs a QG_LinePolygon2Options as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LinePolygon2Options::QG_LinePolygon2Options(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LinePolygon2Options::~QG_LinePolygon2Options()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LinePolygon2Options::languageChange()
{
    retranslateUi(this);
}

