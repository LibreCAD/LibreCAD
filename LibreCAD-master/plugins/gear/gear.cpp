/*****************************************************************************/
/*  gear.cpp - plugin gear for LibreCAD                                      */
/*                                                                           */
/*  Copyright (C) 2016 Cédric Bosdonnat cedric@bosdonnat.fr                  */
/*  Edited 2017 Luis Colorado <luiscoloradourcola@gmail.com>                 */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 2 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

#include <QGridLayout>
#include <QPushButton>
#include <QSettings>
#include <QMessageBox>
#include <QTransform>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <vector>
#include <cmath>
#include <cfloat>

#include "document_interface.h"
#include "gear.h"

QString LC_Gear::name() const
{
    return (tr("Gear creation plugin"));
}

PluginCapabilities LC_Gear::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
            << PluginMenuLocation("plugins_menu", tr("Gear plugin"));
    return pluginCapabilities;
}

LC_Gear::LC_Gear()
{
}

LC_Gear::~LC_Gear()
{
}

void LC_Gear::execComm(Document_Interface *doc,
                        QWidget *parent, QString cmd)
{
    Q_UNUSED(doc);
    Q_UNUSED(cmd);

    QPointF center;
    if (!doc->getPoint(&center, QString("select center"))) {
        return;
    }

    if (!parameters_dialog) {
        parameters_dialog = new lc_Geardlg(parent);
        if (!parameters_dialog) {
            return;
        }
    }

    int result =  parameters_dialog->exec();
    if (result == QDialog::Accepted)
        parameters_dialog->processAction(doc, cmd, center);
}

/*****************************/

lc_Geardlg::lc_Geardlg(QWidget *parent) :
    QDialog(parent),
    settings(QSettings::IniFormat, QSettings::UserScope, "LibreCAD", "gear_plugin")
{
    const char *windowTitle = "Draw a gear";

    setWindowTitle(tr(windowTitle));

    QLabel *label;
    QGridLayout *mainLayout = new QGridLayout(this);

    int i = 0, j = 0;

#define RST() do{ if(j) { ++i; j = 0; } } while(0)

#define Q(name, type, text, min, max, stp) do {                  \
            label = new QLabel((text), this);                    \
            name = new type(this);                               \
            name->setMinimum(min);                               \
            name->setMaximum(max);                               \
            name->setSingleStep(stp);                            \
            mainLayout->addWidget(label,  i, 0);                 \
            mainLayout->addWidget((name), i, 1);                 \
        } while(0)

#define QDSB(name, text, min, max, stp, dec) do {                \
            RST();                                               \
            Q(name, QDoubleSpinBox, (text),(min),(max),(stp));   \
            name->setDecimals(dec);                              \
            ++i; j = 0;                                          \
        } while(0)

#define QSB(name, text, min, max, stp) do {                      \
            RST();                                               \
            Q(name, QSpinBox, (text),(min),(max), (stp));        \
            ++i; j = 0;                                          \
        } while(0)

#define QCB(name, text) do {                                     \
            name = new QCheckBox((text), this);                  \
            mainLayout->addWidget(name, i, j);                   \
            j++; if (j >= 2) { j = 0; i++; }                     \
        } while(0)

    QDSB(rotateBox,               tr("Rotation angle"), -360.0, 360.0, 1.0, 6);
    QSB (nteethBox,               tr("Number of teeth"), 1, 2000, 1);
    QDSB(modulusBox,              tr("Modulus"), 1.0E-10, 1.0E+10, 0.1, 6); 
    QDSB(pressureBox,             tr("Pressure angle (deg)"), 0.1, 89.9, 1.0, 5);
    QDSB(addendumBox,             tr("Addendum (rel. to modulus)"), 0.0, 5.0, 0.1, 5);
    QDSB(dedendumBox,             tr("Dedendum (rel. to modulus)"), 0.0, 5.0, 0.1, 5);
    QSB (n1Box,                   tr("Number of segments to draw (dedendum)"), 1, 1024, 8);
    QSB (n2Box,                   tr("Number of segments to draw (addendum)"), 1, 1024, 8);
    QCB (drawAllTeethBox,         tr("Draw all teeth?"));
    QCB (drawBothSidesOfToothBox, tr("Draw symmetric face?"));

    QCB (useLayersBox,            tr("Use layers?")); RST();
    QCB (drawAddendumCircleBox,   tr("Draw addendum circle?"));
    QCB (drawPitchCircleBox,      tr("Draw pitch circle?"));
    QCB (drawBaseCircleBox,       tr("Draw base circle?"));
    QCB (drawRootCircleBox,       tr("Draw root circle?"));
    QCB (drawPressureLineBox,     tr("Draw pressure line?"));
    QCB (drawPressureLimitBox,    tr("Draw pressure limits?"));

    QCB (calcInterferenceBox,     tr("Calculate interference?"));
    QSB (n3Box,                   tr("Number of segments to draw (interference)"), 1, 1024,     8);

    QPushButton *acceptbut = new QPushButton(tr("Accept"), this);
    QPushButton *cancelbut = new QPushButton(tr("Cancel"), this);
    QHBoxLayout *acceptLayout = new QHBoxLayout();

    acceptLayout->addStretch();
    acceptLayout->addWidget(acceptbut);
    acceptLayout->addStretch();
    acceptLayout->addWidget(cancelbut);
    acceptLayout->addStretch();

    mainLayout->addLayout(acceptLayout, i, 0, 1, 2);
    setLayout(mainLayout);

    readSettings();

    connect(cancelbut, SIGNAL(clicked()), this, SLOT(reject()));
    connect(acceptbut, SIGNAL(clicked()), this, SLOT(checkAccept()));
}

/* calculate the radius of a point in canonical evoluta
 * whose radius is given. */
static double radius2arg(const double radius, const double alpha = 0.0)
{
    const double aux = 1.0 - alpha;
    return sqrt(radius * radius - aux*aux);
}

/* canonical evolute is generated by a 1.0 radius circle.
 * We consider it the next complex function:
 * (1.0 - alpha - i*phi) * exp(i*phi) 
 */
static double re_evolute(const double phi, const double alpha = 0.0)
{
    return (1.0 - alpha) * cos(phi) + phi * sin(phi);
}

static double im_evolute(const double phi, const double alpha = 0.0)
{
    return (1.0 - alpha) * sin(phi) - phi * cos(phi);
}

static double mod_evolute(const double phi, const double alpha = 0.0)
{
    double aux = (1.0 - alpha);
    return sqrt(aux*aux + phi*phi);
}

static double arg_evolute(const double phi, const double alpha = 0.0)
{
    double aux = (1.0 - alpha);
    return phi - atan2(phi, aux);
}

struct evolute {

    static const double default_eps;

    evolute(int n_t, double add, double ded, double p_ang);

    QPointF evo0(const double phi); /* evolute for tooth face */
    QPointF evo1(const double phi); /* evolute for tooth carving (interference) */
    double aux(const double phi); /* auxiliary function */
    double find_common_phi_evo1(const double eps = default_eps); 

    const int n_teeth;
    const double
        addendum, dedendum,
        c_modulus,
        p_angle, cos_p_angle, cos2_p_angle,
        angle_0, cos_angle_0, sin_angle_0,
        dedendum_radius, addendum_radius,
        phi_at_dedendum, phi_at_addendum,
        alpha, angle_1,
        cos_angle_1, sin_angle_1;
};

const double evolute::default_eps = 8 * DBL_EPSILON;

evolute::evolute(int n_t, double add, double ded, double p_ang):
    n_teeth(n_t),
    addendum(add),
    dedendum(ded),
    c_modulus(2.0/n_teeth), 
    p_angle(p_ang),
    cos_p_angle(cos(p_ang)),
    cos2_p_angle(cos_p_angle * cos_p_angle),
    angle_0(p_angle - tan(p_angle)),
    cos_angle_0(cos(angle_0)),
    sin_angle_0(sin(angle_0)),
    dedendum_radius(1.0 - c_modulus * dedendum),
    addendum_radius(1.0 + c_modulus * addendum),
    phi_at_dedendum(dedendum_radius > cos_p_angle
            ? radius2arg(dedendum_radius / cos_p_angle)
            : 0.0),
    phi_at_addendum(radius2arg(addendum_radius / cos_p_angle)),
    alpha(1.0 - dedendum_radius),
    angle_1(-alpha * tan(p_angle)),
    cos_angle_1(cos(angle_1)),
    sin_angle_1(sin(angle_1))
{
}

/* this evolute calculates points for an argument phi for the
 * curve that defines de active face of the tooth.  */
QPointF evolute::evo0(const double phi)
{
    double x = cos_p_angle * re_evolute(phi),
           y = cos_p_angle * im_evolute(phi);
    return QPointF(cos_angle_0 * x - sin_angle_0 * y,
                   sin_angle_0 * x + cos_angle_0 * y);
}

/* this evolute calculates points for an argument phi for the
 * curve that defines the carved neck of the tooth in case of
 * interference. */
QPointF evolute::evo1(const double phi)
{
    double x = re_evolute(phi, alpha);
    double y = im_evolute(phi, alpha);
    return QPointF(cos_angle_1 * x - sin_angle_1 * y,
                   sin_angle_1 * x + cos_angle_1 * y);
}

/* Auxiliary function to determine if we are in one side of the
 * primary evolute or in the other side.  We look for a zero in this
 * function to derive the phi angle of the secondary evolute at which
 * it crosses the primary.   This is the common point for both evolutes
 */
double evolute::aux(const double phi)
{
    const double mod = mod_evolute(phi, alpha);
    const double arg = arg_evolute(phi, alpha);

    if (mod <= cos_p_angle) {
        return arg + angle_1 - angle_0;
    }
    const double phi0 = radius2arg(mod / cos_p_angle);
    return arg + angle_1 + atan(phi0) - phi0 - angle_0;
}

/* find the common point of both evolutes.  this function uses two
 * values a and b of phi in the evolute::evo1 curve to find a root of
 * the function evolute::aux that gives the difference between the argument
 * at which the evolute of the primary curve touches the base circle minus
 * the argument at which an evolute that pases for the point calculated in
 * the second curve hits the base circle.  This being positive means the
 * second evolute has already crossed the first.  Being negative means it
 * has not yet crossed the primary evolute. */
double evolute::find_common_phi_evo1(const double eps)
{
    double a = -radius2arg(cos_p_angle, alpha);
    double b = -radius2arg(1.0, alpha);
    double f_a = aux(a), f_b = aux(b);
    double x = a, f_x = f_a;

    if (f_a > 0) do {

        x = (a*f_b - b*f_a) / (f_b - f_a);
        f_x = aux(x);

        if (fabs(x - a) < fabs(x - b)) {
            b = x; f_b = f_x;
        } else {
            a = x; f_a = f_x;
        }
    } while (fabs(a-b) >= eps);

    return x;
} /* find_common_phi_evo1 */

void lc_Geardlg::processAction(Document_Interface *doc, const QString& cmd, QPointF& center)
{
    Q_UNUSED(doc);
    Q_UNUSED(cmd);
    Q_UNUSED(center);

    std::vector<Plug_VertexData> polyline;
    std::vector<QPointF> first_tooth;
    QTransform rotate_and_disp;

    /* we shall proceed by calculating the points for root radius,
     * base of tooth, n1 line segments to the pitch circle (this makes
     * possible to have a reference point on the pitch circle)
     * n2 line segments to the addendum circle.
     * The tooth face is aligned so the pitch point is aligned with the
     * X axis, so we can align gears over this reference point.
     *
     * Once we get one face, we mirror it using as the axis one quarter
     * of the pitch angular modulus (half of the tooth width) passing
     * along the origin.
     *
     * Finally, we get the complete set of teeth by rotating them by the
     * pitch angular modulus to get the whole gear */

    evolute ev(nteethBox->value(), /* number of teeth */
               addendumBox->value(), /* addendum */
               dedendumBox->value(), /* dedendum */
               M_PI / 180.0 * pressureBox->value()); /* pressure angle (converted to rad) */

    const double modulus = modulusBox->value();
    const double scale_factor = modulus / ev.c_modulus;
    const int    n1 = n1Box->value();
    const int    n2 = n2Box->value();
    const double rotation = rotateBox->value() * M_PI / 180.0;

    rotate_and_disp = rotate_and_disp
        .translate(center.x(), center.y())
        .rotateRadians(rotation);

    double phi_0 = 0.0;

    /* Build one tooth face */
    if (calcInterferenceBox->isChecked()
        && ev.cos2_p_angle > ev.dedendum_radius)
    {
            const int n3 = n3Box->value();
            double angle_2 = ev.find_common_phi_evo1();

            phi_0 = radius2arg(mod_evolute(angle_2, ev.alpha) / ev.cos_p_angle);

            double phi = 0.0,
                   delta = angle_2 / n3;
            for(int i = 0; i < n3; i++) {
                const QPointF point(scale_factor * ev.evo1(phi));
                first_tooth.push_back(point);
                polyline.push_back(Plug_VertexData(rotate_and_disp.map(point), 0.0));
                phi += delta;
            } /* for */
    } else if (ev.cos_p_angle > ev.dedendum_radius) {

        /* no interference calculation at all.  just draw the point at the
         * intersection of the root circle with the 0 press angle point. */

        QPointF root(scale_factor * ev.dedendum_radius * ev.cos_angle_0,
                     scale_factor * ev.dedendum_radius * ev.sin_angle_0);
        first_tooth.push_back(root);
        polyline.push_back(Plug_VertexData(rotate_and_disp.map(root), 0.0));
    }

    if (phi_0 < ev.phi_at_dedendum) phi_0 = ev.phi_at_dedendum;

    double phi = phi_0;

    /* if the carving has eaten some active part of the tooth dedendum face */
    if (phi < ev.p_angle - ev.angle_0) {
        double delta = (ev.p_angle - ev.angle_0 - phi) / n1;
        for (int i = 0; i < n1; ++i) {
            const QPointF point(scale_factor * ev.evo0(phi));
            first_tooth.push_back(point);
            polyline.push_back(Plug_VertexData(rotate_and_disp.map(point), 0.0));
            phi += delta;
        } /* for */
    }
    double phi_1 = radius2arg(ev.addendum_radius / ev.cos_p_angle);
    if (phi < phi_1) {
        double delta = (phi_1 - phi) / n2;
        for (int i = 0; i <= n2; ++i) {
            const QPointF point(scale_factor * ev.evo0(phi));
            first_tooth.push_back(point);
            polyline.push_back(Plug_VertexData(rotate_and_disp.map(point), 0.0));
            phi += delta;
        }
    }

    /* calculate the symmetric face from the original points */

    /* one half of pitch angular modulus = double of symmetry axis.
     * symmetry on an axis that passes through origin is calculated as follows:
     * x' = cos(2.0*axis_angle) * x + sin(2.0*axis_angle) * y
     * y' = sin(2.0*axis_angle) * x - cos(2.0*axis_angle) * y
     * (note: we don't use iterators as the array is growing as long as we
     * navigate it)
     */
    const double axis_angle_x_2 = M_PI / ev.n_teeth;
    const double axis_angle = axis_angle_x_2 / 2.0;
    const double cos_axis_angle_x_2 = cos(axis_angle_x_2);
    const double sin_axis_angle_x_2 = sin(axis_angle_x_2);

    /* remember size, as we don't want to duplicate next point */
    const double n_to_mirror = first_tooth.size();

    if (drawBothSidesOfToothBox->isChecked()) {
        /* symmetry axis point (at top of tooth) */
        QPointF mirror_point(scale_factor * ev.addendum_radius * cos(axis_angle),
                             scale_factor * ev.addendum_radius * sin(axis_angle));
        first_tooth.push_back(mirror_point);
        polyline.push_back(Plug_VertexData(rotate_and_disp.map(mirror_point), 0.0));

        /* for all points we have to mirror (all but the last one) */
        for (int i = n_to_mirror - 1; i >= 0; --i) {
            const QPointF& orig(first_tooth[i]);

            QPointF target(cos_axis_angle_x_2 * orig.x() + sin_axis_angle_x_2 * orig.y(),
                           sin_axis_angle_x_2 * orig.x() - cos_axis_angle_x_2 * orig.y());
            first_tooth.push_back(target);
            polyline.push_back(Plug_VertexData(rotate_and_disp.map(target), 0.0));
        } /* for */

        if (drawAllTeethBox->isChecked()) {
            /* symmetry axis point (at interteeth) */
            QPointF mirror_point2(scale_factor * ev.dedendum_radius * cos(axis_angle + axis_angle_x_2),
                                  scale_factor * ev.dedendum_radius * sin(axis_angle + axis_angle_x_2));
            first_tooth.push_back(mirror_point2);
            polyline.push_back(Plug_VertexData(rotate_and_disp.map(mirror_point2), 0.0));

            /* now, we have to rotate the tooth to get all the teeth missing. */
            for (int i = 1; i < ev.n_teeth; i++) {

                const double angle = M_PI * ev.c_modulus * i;
                const double cos_angle = cos(angle);
                const double sin_angle = sin(angle);

                for (std::vector<QPointF>::iterator it = first_tooth.begin();
                        it != first_tooth.end(); ++it)
                {
                    const QPointF& orig = *it;
                    polyline.push_back(Plug_VertexData(rotate_and_disp.map(QPointF(
                                        cos_angle * orig.x() - sin_angle * orig.y(),
                                        sin_angle * orig.x() + cos_angle * orig.y())),
                                                       0.0));
                } /* for */
            } /* for */
        }
    }

    QString lastLayer = doc->getCurrentLayer();

#define LAYER(fmt) do { \
            if (useLayersBox->isChecked()) { \
                char buffer[128]; \
                snprintf(buffer, sizeof buffer, \
                        "gear_M%6.4f_" fmt, modulus); \
                doc->setLayer(buffer); \
            } \
        } while(0)

    LAYER("shapes"); 
    doc->addPolyline(polyline,
            drawAllTeethBox->isChecked()
            && drawBothSidesOfToothBox->isChecked());

    if (drawPitchCircleBox->isChecked()) {
        LAYER("pitch_circles");
        doc->addCircle(&center, scale_factor);
    }

    if (drawAddendumCircleBox->isChecked()) {
        LAYER("addendums");
        doc->addCircle(&center, scale_factor * ev.addendum_radius);
    }

    if (drawRootCircleBox->isChecked()) {
        LAYER("dedendums");
        doc->addCircle(&center, scale_factor * ev.dedendum_radius);
    }

    if (drawBaseCircleBox->isChecked()) {
        LAYER("base_lines");
        doc->addCircle(&center, scale_factor * ev.cos_p_angle);
    }

    if (drawPressureLineBox->isChecked() || drawPressureLimitBox->isChecked()) {
        LAYER("action_lines");
        QPointF p1(scale_factor * cos(ev.p_angle + rotation) * ev.cos_p_angle,
                   scale_factor * sin(ev.p_angle + rotation) * ev.cos_p_angle),
                p2(scale_factor * cos(rotation),
                   scale_factor * sin(rotation));
        p1 += center; p2 += center;
        if (drawPressureLimitBox->isChecked())
            doc->addLine(&center, &p1);
        if (drawPressureLineBox->isChecked())
            doc->addLine(&p1, &p2);
    }

    if (useLayersBox->isChecked())
        doc->setLayer(lastLayer);

    writeSettings();
}

void lc_Geardlg::checkAccept()
{
    accept();
}

lc_Geardlg::~lc_Geardlg()
{
}

void lc_Geardlg::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
}

void lc_Geardlg::readSettings()
{

    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(430,140)).toSize();

#define R(var,toFunc, defval) do { \
        var ## Box->setValue(settings.value(#var, defval).toFunc()); \
    } while(0)
        
#define RB(var,defval) do { \
        var ## Box->setChecked(settings.value(#var, defval).toBool()); \
    } while (0)
    R(rotate, toDouble,         0.0 );
    R(nteeth, toInt,           20   );
    R(modulus, toDouble,        1.0 );
    R(pressure, toDouble,      20.0 );
    R(addendum, toDouble,       1.0 );
    R(dedendum, toDouble,       1.25);
    R(n1, toInt,               16   );
    R(n2, toInt,               16   );
    RB(drawAllTeeth,         true   );
    RB(drawBothSidesOfTooth, true   );
    RB(useLayers,            true   );
    RB(drawAddendumCircle,  false   );
    RB(drawPitchCircle,      true   );
    RB(drawBaseCircle,       true   );
    RB(drawRootCircle,      false   );
    RB(drawPressureLine,     true   );
    RB(drawPressureLimit,   false   );
    RB(calcInterference,    false   );
    R(n3, toInt,               16   );

    resize(size);
    move(pos);
}

void lc_Geardlg::writeSettings()
{
#define W(var, vfunc) do { \
        settings.setValue(#var, var##Box->vfunc()); \
    } while (0)
#define WN(var) W(var, value)
#define WB(var) W(var, isChecked)
    
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    WN(nteeth);
    WN(modulus);
    WN(pressure);
    WN(addendum);
    WN(dedendum);
    WN(n1);
    WN(n2);
    WB(drawAllTeeth);
    WB(drawBothSidesOfTooth);
    WB(useLayers);
    WB(drawAddendumCircle);
    WB(drawPitchCircle);
    WB(drawBaseCircle);
    WB(drawRootCircle);
    WB(drawPressureLine);
    WB(drawPressureLimit);
    WB(calcInterference);
    WN(n3);
}
