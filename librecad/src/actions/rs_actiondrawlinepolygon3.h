#ifndef RS_ACTIONDRAWLINEPOLYGONCENTAN_H
#define RS_ACTIONDRAWLINEPOLYGONCENTAN_H


#include "rs_previewactioninterface.h"


/**
 * This action class can handle user events to draw polygons.
 *
 */
class RS_ActionDrawLinePolygonCenTan : public RS_PreviewActionInterface {
    Q_OBJECT
    enum Status {
        SetCenter,    /**< Setting center. */
        SetTangent,    /**< Setting tangent. */
        SetNumber     /**< Setting number in the command line. */
    };

public:
    RS_ActionDrawLinePolygonCenTan(RS_EntityContainer& container,
                              RS_GraphicView& graphicView);
    ~RS_ActionDrawLinePolygonCenTan() override;

    void trigger() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void updateMouseButtonHints() override;

    void coordinateEvent(RS_CoordinateEvent* e) override;
    void commandEvent(RS_CommandEvent* e) override;
        QStringList getAvailableCommands() override;

    void hideOptions() override;
    void showOptions() override;

    void updateMouseCursor() override;

    int getNumber() const{
        return number;
    }

    void setNumber(int n) {
        number = n;
    }

private:
    struct Points;
    std::unique_ptr<Points> pPoints;
    /** Number of edges. */
    int number;
    /** Last status before entering text. */
    Status lastStatus;
};

#endif // RS_ACTIONDRAWLINEPOLYGONCENTAN_H
