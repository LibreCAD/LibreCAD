#ifndef QG_ACTIVELAYERNAME_H
#define QG_ACTIVELAYERNAME_H

#include "lc_graphicviewaware.h"
#include "rs_layerlistlistener.h"
#include "ui_qg_activelayername.h"

class RS_LayerList;

class QLabel;

/**
 * The name of the current layer in the status bar, with the icon the Layer List shows for a hidden or
 * a locked layer when it is: it listens to the layer list of the drawing shown, so it follows every
 * change of the current layer and its flags.
 */
class QG_ActiveLayerName : public QWidget, public LC_GraphicViewAware, public RS_LayerListListener, public Ui::QG_ActiveLayerName{
    Q_OBJECT
public:
    explicit QG_ActiveLayerName(QWidget *parent = nullptr);
    ~QG_ActiveLayerName() override;
    void updateActiveLayer() const;
    /// the icons that say the current layer is hidden, locked: shown when it is
    QLabel* hiddenIcon() const {return m_hiddenIcon;}
    QLabel* lockedIcon() const {return m_lockedIcon;}
    void setGraphicView(RS_GraphicView* gview) override;

    // Methods from RS_LayerListListener Interface:
    void layerListModified(bool) override;
    /// the list is being destroyed: forget it
    void layerListDestroyed() override;

private:
    void setLayerList(RS_LayerList* layerList);
    void showState(QLabel* label, const char* iconResource, const QString& toolTip, bool shown) const;
    RS_LayerList* m_layerList = nullptr;
    QLabel* m_hiddenIcon = nullptr;
    QLabel* m_lockedIcon = nullptr;
};

#endif
