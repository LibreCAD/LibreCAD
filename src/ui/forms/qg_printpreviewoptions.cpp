#include "qg_printpreviewoptions.h"

#include <qvariant.h>
#include "qg_printpreviewoptions.ui.h"
/*
 *  Constructs a QG_PrintPreviewOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_PrintPreviewOptions::QG_PrintPreviewOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_PrintPreviewOptions::~QG_PrintPreviewOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_PrintPreviewOptions::languageChange()
{
    retranslateUi(this);
}

