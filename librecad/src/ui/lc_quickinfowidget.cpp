#include "rs_line.h"

#include "lc_quickinfowidget.h"
#include "ui_lc_quickinfowidget.h"
#include "rs_graphic.h"

LC_QuickInfoWidget::LC_QuickInfoWidget(QG_ActionHandler *ah, QWidget *parent, const char *name):
    QWidget(parent),
    ui(new Ui::LC_QuickInfoWidget)
{
    ui->setupUi(this);
}

LC_QuickInfoWidget::~LC_QuickInfoWidget()
{
    delete ui;
}

void LC_QuickInfoWidget::setEntity(RS_Entity *en, bool update){
    if (en == nullptr){
        clearInfo();
    }
    else{
        bool shouldUpdate = false;
        if (currentEntity == en){
            shouldUpdate = update;
        }
        if (shouldUpdate){
            int rtti = en->rtti();
            EntityInfo result;
            collectGenericProperties(en, result);
            switch (rtti){
                case RS2::EntityLine: {
                    auto *line = reinterpret_cast<RS_Line *>(en);
                    collectLineProperties(line, result);
                    break;
                }
                case RS2::EntityCircle: {
                    auto *circle = reinterpret_cast<RS_Circle *>(en);
                    collectCircleProperties(circle, result);
                    break;
                }
            }
        }
    }
}

void LC_QuickInfoWidget::set_document(RS_Document *doc){
    document = doc;
    clearInfo();
}

void LC_QuickInfoWidget::set_view(QG_GraphicView *view){
    clearInfo();
}

void LC_QuickInfoWidget::clearInfo(){

}

void LC_QuickInfoWidget::collectGenericProperties(RS_Entity* e, EntityInfo &result){
    RS_Pen pen = e->getPen(false);
    RS_Pen resolvedPen = e->getPen(true);

    RS_Layer* layer = e->getLayer(true);
    QString layerName = "";
    if (layer != nullptr){
        layerName = layer->getName();
    }

    RS_Color color = pen.getColor();
    RS_Color resolvedColor = resolvedPen.getColor();

    RS2::LineType lineType = pen.getLineType();
    RS2::LineType resolvedLineType = resolvedPen.getLineType();
    RS2::LineWidth lineWidth = pen.getWidth();
    RS2::LineWidth resolvedLineWidth = resolvedPen.getWidth();

    int colorType = LC_PenInfoRegistry::NATURAL;

    QString colorName = penRegistry->getColorName(color, colorType);
    QString lineTypeName = penRegistry->getLineTypeText(lineType);
    QString lineWidthName = penRegistry->getLineWidthText(lineWidth);

    result.addProperty("ID", e->getId());
    result.addProperty("Layer", layerName);
    result.addProperty("Color", colorName);
    if (resolvedColor != color){
        QString actualColorName = "";
        result.addProperty("Actual Color", actualColorName);
    }
    result.addProperty("Line Type", lineTypeName);
    if (resolvedLineType != lineType){
        QString resolvedLineTypeName = penRegistry->getLineTypeText(resolvedLineType);
        result.addProperty("Actual Line Type", resolvedLineTypeName);
    }
    result.addProperty("Line Width", lineWidthName);
    if (resolvedLineType != lineType){
        QString resolvedLineWidthName = penRegistry->getLineWidthText(resolvedLineWidth);
        result.addProperty("Resolved Line Width", resolvedLineWidthName);
    }
}

void LC_QuickInfoWidget::collectLineProperties(RS_Line* line, EntityInfo &result){
    const RS_Vector &start = line->getStartpoint();
    const RS_Vector &end = line->getEndpoint();
    double angle = line->getAngle1();
    double length = line->getLength();
    RS_Vector delta = end - start;

    QString startPoint = formatVector(start);
    QString endPoint = formatVector(end);
    QString angleStr = formatAngle(angle);
    QString len = formatDistance(length);
    QString inc = formatVector(delta);


    result.addProperty("From", startPoint);
    result.addProperty("To", endPoint);
    result.addProperty("Delta", inc, true);
    result.addProperty("Angle", angleStr);
    result.addProperty("Length", len);
}

void LC_QuickInfoWidget::collectCircleProperties(RS_Circle *circle, EntityInfo& result){

}

QString LC_QuickInfoWidget::formatVector(const RS_Vector &vector){

    RS_Graphic* graphic = document->getGraphic();
    QString x = RS_Units::formatLinear(vector.x,  graphic->getUnit(),  graphic->getLinearFormat(),graphic->getLinearPrecision());
    QString y = RS_Units::formatLinear(vector.y,  graphic->getUnit(),  graphic->getLinearFormat(),graphic->getLinearPrecision());

    QString result = x + "," + y;
    return result;
}

QString LC_QuickInfoWidget::formatAngle(double angle){
    RS_Graphic* graphic = document->getGraphic();
    QString result =  RS_Units::formatAngle(angle,  graphic->getAngleFormat(), graphic->getAnglePrecision());
    return result;
}

QString LC_QuickInfoWidget::formatDistance(double length){
    RS_Graphic* graphic = document->getGraphic();
    QString result = RS_Units::formatLinear(length,  graphic->getUnit(),  graphic->getLinearFormat(),graphic->getLinearPrecision());
    return result;
}
