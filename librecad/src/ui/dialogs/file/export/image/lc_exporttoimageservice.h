#ifndef LC_EXPORTTOIMAGESERVICE_H
#define LC_EXPORTTOIMAGESERVICE_H

#include "lc_appwindowdialogsinvoker.h"

class RS_Graphic;

class LC_ExportToImageService:public QObject, public LC_AppWindowAware{
public:
    LC_ExportToImageService(QC_ApplicationWindow* mainWin, LC_AppWindowDialogsInvoker* dlgHelper)
        : LC_AppWindowAware{mainWin},
          m_dlgHelpr{dlgHelper} {
    }
    bool exportGraphicsToImage(RS_Graphic* graphic, const QString& documentFileName);
private:
    LC_AppWindowDialogsInvoker* m_dlgHelpr;
};

#endif // LC_EXPORTTOIMAGESERVICE_H
