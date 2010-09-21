#include "qg_textoptions.h"

#include <qvariant.h>
#include <qtextcodec.h>
#include "rs_system.h"
#include "rs_filterdxf.h"
#include "qg_textoptions.ui.h"
/*
 *  Constructs a QG_TextOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_TextOptions::QG_TextOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_TextOptions::~QG_TextOptions()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_TextOptions::languageChange()
{
    retranslateUi(this);
}

