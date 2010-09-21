#include "qg_lineparallelthroughoptions.h"

#include <qvariant.h>
#include "qg_lineparallelthroughoptions.ui.h"
/*
 *  Constructs a QG_LineParallelThroughOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LineParallelThroughOptions::QG_LineParallelThroughOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LineParallelThroughOptions::~QG_LineParallelThroughOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LineParallelThroughOptions::languageChange()
{
    retranslateUi(this);
}

