#ifndef QG_ACTIVELAYERNAME_H
#define QG_ACTIVELAYERNAME_H

#include "lc_graphicviewaware.h"
#include "rs_layerlistlistener.h"
#include "ui_qg_activelayername.h"

class RS_LayerList;

/**
 * The name of the current layer in the status bar, with "hidden" or "locked" when it is: it listens to
 * the layer list of the drawing shown, so it follows every change of the current layer and its flags.
 */
class QG_ActiveLayerName : public QWidget, public LC_GraphicViewAware, public RS_LayerListListener, public Ui::QG_ActiveLayerName{
    Q_OBJECT
public:
    explicit QG_ActiveLayerName(QWidget *parent = nullptr);
    ~QG_ActiveLayerName() override;
    void updateActiveLayer() const;
    void setGraphicView(RS_GraphicView* gview) override;

    // Methods from RS_LayerListListener Interface:
    void layerListModified(bool) override;
    /// the list is being destroyed: forget it
    void layerListDestroyed() override;

private:
    void setLayerList(RS_LayerList* layerList);
    RS_LayerList* m_layerList = nullptr;
};

#endif
