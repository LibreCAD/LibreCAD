#ifndef LC_QUICKINFOWIDGET_H
#define LC_QUICKINFOWIDGET_H

#include <QWidget>
#include <QDockWidget>
#include "rs_units.h"
#include "qg_actionhandler.h"
#include "qg_graphicview.h"
#include "lc_peninforegistry.h"

namespace Ui {
class LC_QuickInfoWidget;
}

class LC_QuickInfoWidget : public QWidget
{
    Q_OBJECT

public:

    LC_QuickInfoWidget(QG_ActionHandler *ah, QWidget *parent, const char *name = nullptr);

    ~LC_QuickInfoWidget();
    void set_view(QG_GraphicView *view);
    void set_document(RS_Document *document);

signals:

    void escape();

protected:

    struct PropertyInfo{
        QString label;
        QString value;
        bool calculated;
    };

    struct EntityInfo{
        int entityRtti;
        int entityId;
        QString entityName;
        QVector<PropertyInfo*> properties;

        void addProperty(const char* name, QString value, bool calculated = false){
            auto* prop = new PropertyInfo();
            prop->label = tr(name);
            prop->value = value;
            prop->calculated = calculated;
            properties << prop;
        }

        void addProperty(const char* name, double value, bool calculated = false){
            QString val;
            val.setNum(value);
            addProperty(name, val, calculated);
        }
    };

private:
    Ui::LC_QuickInfoWidget *ui;
    RS_Entity* currentEntity = nullptr;
    RS_GraphicView* view = nullptr;
    RS_Document* document = nullptr;
    void clearInfo();
    LC_PenInfoRegistry* penRegistry = LC_PenInfoRegistry::instance();
    void setEntity(RS_Entity *entity, bool update);
    void collectLineProperties(RS_Line* line, EntityInfo &result);
    void collectCircleProperties(RS_Circle *circle, EntityInfo &result);
    void collectGenericProperties(RS_Entity *line, EntityInfo &result);
    QString formatVector(const RS_Vector &vector);
    QString formatAngle(double angle);
    QString formatDistance(double length);
};

#endif // LC_QUICKINFOWIDGET_H
