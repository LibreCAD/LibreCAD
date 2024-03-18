//
// Created by sand1 on 14/03/2024.
//

#ifndef LIBRECAD_LC_ABSTRACT_ACTION_DRAW_LINE_H
#define LIBRECAD_LC_ABSTRACT_ACTION_DRAW_LINE_H

#include "rs_line.h"
#include "rs_vector.h"
#include "rs_previewactioninterface.h"
#include "lc_abstractactionwithpreview.h"

/**
 * Utility base class for actions that draw a line
 */
class LC_AbstractActionDrawLine:public LC_AbstractActionWithPreview {

public:
    // action statuses
    enum Status {
        SetStartPoint,
        SetDirection,
        SetDistance,
        SetPoint,
        SetAngle,
        LAST
    };

    // direction in which line should be drawn
    enum Direction{
        DIRECTION_NONE, DIRECTION_X, DIRECTION_Y, DIRECTION_POINT, DIRECTION_ANGLE
    };

protected:
    double angleValue; // fixed angle for line
    bool angleIsRelative {true}; // is angle relative to previous segment (if any)
    int direction {DIRECTION_NONE}; // current line direction

    void setSetAngleState(bool relative);
    virtual bool processAngleValueInput(RS_CommandEvent *e, const QString &c);
    virtual bool doProceedCommand(RS_CommandEvent *pEvent, const QString &qString);
    virtual bool doProcessCommandValue(RS_CommandEvent *e, const QString &c);
    virtual const RS_Vector& getStartPointForAngleSnap() const = 0;
    virtual bool isStartPointValid() const;
    virtual void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapped);
    bool doCheckMayDrawPreview(QMouseEvent *pEvent, int status) override;
    RS_Vector doGetMouseSnapPoint(QMouseEvent *e) override;
public:
    LC_AbstractActionDrawLine(const char* name, RS_EntityContainer &container,RS_GraphicView &graphicView);

    ~LC_AbstractActionDrawLine() override;
    int getDirection(){return direction;}
    void setNewStartPointState();
    void setSetAngleDirectionState();
    void setSetPointDirectionState();
    void setSetXDirectionState();
    void setSetYDirectionState();
    void setAngleValue(double value);
    double getAngleValue();
    bool isAngleRelative();
    void setAngleIsRelative(bool value);
    virtual bool mayStart();
    bool doProcessCommand(RS_CommandEvent *e, const QString &c) override;
};

#endif //LIBRECAD_LC_ABSTRACT_ACTION_DRAW_LINE_H
