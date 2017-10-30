/*****************************************************************************/
/*  gear.cpp - plugin gear for LibreCAD                                      */
/*                                                                           */
/*  Copyright (C) 2016 Cédric Bosdonnat cedric@bosdonnat.fr                  */
/*  Edited by 2017 Luis Colorado <luiscoloradourcola@gmail.com>              */
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

#include "document_interface.h"
#include "gear.h"

#define GEAR_TYPE_SPUR int(0)
#define GEAR_TYPE_RING int(1)

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

void LC_Gear::execComm(Document_Interface *doc,
                        QWidget *parent, QString cmd)
{
    Q_UNUSED(doc);
    Q_UNUSED(cmd);
    QPointF center;

    if (!doc->getPoint(&center, QString("select center")))
        return;

    lc_Geardlg pdt(parent, &center);
    int result =  pdt.exec();
    if (result == QDialog::Accepted)
        pdt.processAction(doc);
}

/*****************************/

lc_Geardlg::lc_Geardlg(QWidget *parent, QPointF *center) :  QDialog(parent)
{
    this->center = center;

    setWindowTitle(tr("Draw a gear"));
    QLabel *label;

    QGridLayout *mainLayout = new QGridLayout;

    int i = 0;
    label = new QLabel(tr("Rotation angle"));
    mainLayout->addWidget(label, i, 0);
    rotateBox = new QDoubleSpinBox();
    rotateBox->setMinimum(-360.0);
    rotateBox->setMaximum(360.0);
    rotateBox->setSingleStep(1.0);
    rotateBox->setDecimals(6);
    mainLayout->addWidget(rotateBox, i, 1);
    i++;

    label = new QLabel(tr("Number of teeth"));
    mainLayout->addWidget(label, i, 0);
    nteethBox = new QSpinBox();
    nteethBox->setMinimum(1);
    nteethBox->setMaximum(2000);
    nteethBox->setSingleStep(1);
    mainLayout->addWidget(nteethBox, i, 1);
    i++;

    label = new QLabel(tr("Modulus"));
    mainLayout->addWidget(label, i, 0);
    modulusBox = new QDoubleSpinBox();
    modulusBox->setMinimum(1.0e-10);
    modulusBox->setMaximum(1.0e+10); /* this is huge */
    modulusBox->setSingleStep(0.001);
    modulusBox->setDecimals(6);
    mainLayout->addWidget(modulusBox, i, 1);
    i++;

    label = new QLabel(tr("Pressure angle (deg)"));
    mainLayout->addWidget(label, i, 0);
    pressureBox = new QDoubleSpinBox();
    pressureBox->setMinimum(0.01);
    pressureBox->setMaximum(89.99);
    pressureBox->setSingleStep(0.01);
    pressureBox->setDecimals(5);
    mainLayout->addWidget(pressureBox, i, 1);
    i++;

    label = new QLabel(tr("Addendum(rel. to modulus)"));
    mainLayout->addWidget(label, i, 0);
    addendumBox = new QDoubleSpinBox();
    addendumBox->setMinimum(0.0);
    addendumBox->setMaximum(10.0); /* ten times the modulus is far too large */
    addendumBox->setSingleStep(0.1);
    addendumBox->setDecimals(5);
    mainLayout->addWidget(addendumBox, i, 1);
    i++;

    label = new QLabel(tr("Dedendum(rel. to modulus)"));
    mainLayout->addWidget(label, i, 0);
    dedendumBox = new QDoubleSpinBox();
    dedendumBox->setMinimum(0.0);
    dedendumBox->setMaximum(10.0);
    dedendumBox->setSingleStep(0.1);
    dedendumBox->setDecimals(5);
    mainLayout->addWidget(dedendumBox, i, 1);
    i++;

    label = new QLabel(tr("Number of segments (dedendum)"));
    mainLayout->addWidget(label, i, 0);
    n1Box = new QSpinBox();
    n1Box->setMinimum(1);
    n1Box->setMaximum(1024);
    n1Box->setSingleStep(1);
    mainLayout->addWidget(n1Box, i, 1);
    i++;

    label = new QLabel(tr("Number of segments (addendum)"));
    mainLayout->addWidget(label, i, 0);
    n2Box = new QSpinBox();
    n2Box->setMinimum(1);
    n2Box->setMaximum(1024);
    n2Box->setSingleStep(1);
    mainLayout->addWidget(n2Box, i, 1);
    i++;

    drawAddendumCircleBox = new QCheckBox("Draw addendum circle?", this);
    mainLayout->addWidget(drawAddendumCircleBox, i, 0);
    drawPitchCircleBox = new QCheckBox("Draw pitch circle?", this);
    mainLayout->addWidget(drawPitchCircleBox, i, 1);
    i++;
    drawBaseCircleBox = new QCheckBox("Draw base circle?", this);
    mainLayout->addWidget(drawBaseCircleBox, i, 0);
    drawRootCircleBox = new QCheckBox("Draw root circle?", this);
    mainLayout->addWidget(drawRootCircleBox, i, 1);
    i++;
    drawPressureLineBox = new QCheckBox("Draw pressure line?", this);
    mainLayout->addWidget(drawPressureLineBox, i, 0);
    drawPressureLimitBox = new QCheckBox("Draw pressure limits?", this);
    mainLayout->addWidget(drawPressureLimitBox, i, 1);
    i++;


    QHBoxLayout *loaccept = new QHBoxLayout;
    QPushButton *acceptbut = new QPushButton(tr("Accept"));
    loaccept->addStretch();
    loaccept->addWidget(acceptbut);
    mainLayout->addLayout(loaccept, i, 0);

    QPushButton *cancelbut = new QPushButton(tr("Cancel"));
    QHBoxLayout *locancel = new QHBoxLayout;
    locancel->addWidget(cancelbut);
    locancel->addStretch();
    mainLayout->addLayout(locancel, i, 1);
    i++;

    setLayout(mainLayout);
    readSettings();

    connect(cancelbut, SIGNAL(clicked()), this, SLOT(reject()));
    connect(acceptbut, SIGNAL(clicked()), this, SLOT(checkAccept()));
}

/* calculate the offset angle to rotate the gear so the
 * tooth first face pitch point lies on the X axis. */
static double offset(const double p_angle)
{
    return tan(p_angle) - p_angle;
}

/* calculate the radius of a point in canonical evoluta
 * whose radius is given. */
static double radius2arg(const double radius)
{
    return sqrt(radius * radius - 1.0);
}

/* canonical evolute is generated by a 1.0 radius circle.
 * We consider it the next complex function:
 * (1.0 - i*phi) * exp(i*phi) 
 */
static double re_evolute(const double phi)
{
    return cos(phi) + phi * sin(phi);
}

static double im_evolute(const double phi)
{
    return sin(phi) - phi * cos(phi);
}

/* modulus of evolute at point phi */
static double mod_evolute(const double phi)
{
    return sqrt(1.0 + phi*phi);
}

void lc_Geardlg::processAction(Document_Interface *doc)
{
    Q_UNUSED(doc);

    std::vector<Plug_VertexData> polyline;
    std::vector<QPointF> first_tooth;

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

    const int    n_teeth = nteethBox->value();
    const double addendum = addendumBox->value();
    const double dedendum = dedendumBox->value();
    const double modulus = modulusBox->value();
    const double c_modulus = 2.0/n_teeth; /* canonical modulus (on a 1.0 pitch circle radius) */
    const double p_angle = M_PI / 180.0 * pressureBox->value();
    const double cos_p_angle = cos(p_angle);
    const double scale_factor = cos_p_angle * modulus / c_modulus;
    const double off_rot = offset(p_angle); /* rot off. to get the pitch point aligned to X axis */
    const double cos_off_rot = cos(off_rot);
    const double sin_off_rot = sin(off_rot);
    const double pitch_radius = mod_evolute(p_angle + off_rot);
    const double dedendum_radius = pitch_radius - c_modulus * dedendum / cos_p_angle;
    const double addendum_radius = pitch_radius + c_modulus * addendum / cos_p_angle;
    const double phi_at_dedendum = (dedendum_radius >= 1.0)
                                 ? radius2arg(dedendum_radius)
                                 : 0.0;
    const double phi_at_addendum = radius2arg(addendum_radius);
    const int    n1 = n1Box->value();
    const int    n2 = n2Box->value();

    /* Build one tooth face */
    if (dedendum_radius < 1.0) {
        QPointF root( scale_factor * dedendum_radius * cos_off_rot,
                     -scale_factor * dedendum_radius * sin_off_rot);
        first_tooth.push_back(root);
    }

    int i;
    const int N = n1 + n2;
    double phi;
    double delta = (p_angle + off_rot - phi_at_dedendum) / n1;
    for (i = 0, phi = phi_at_dedendum; i <= N; ++i, phi += delta) {
        QPointF point( scale_factor * re_evolute(phi),
                       scale_factor * im_evolute(phi)),
                rot_point( cos_off_rot * point.x() + sin_off_rot * point.y(),
                          -sin_off_rot * point.x() + cos_off_rot * point.y());
        first_tooth.push_back(rot_point);
        if (i == n1) { /* change delta to continue until we have all points */
            delta = (phi_at_addendum - p_angle - off_rot) / n2;
        }
    } /* for */

    /* calculate the symmetric face from the original points */

    /* one half of pitch angular modulus = double of symmetry axis.
     * symmetry on an axis that passes through origin is calculated as follows:
     * x' = cos(2.0*axis_angle) * x + sin(2.0*axis_angle) * y
     * y' = sin(2.0*axis_angle) * x - cos(2.0*axis_angle) * y
     * (note: we don't use iterators as the array is growing as long as we
     * navigate it)
     */
    const double axis_angle_x_2 = M_PI / n_teeth;
    const double axis_angle = axis_angle_x_2 / 2.0;
    const double cos_axis_angle_x_2 = cos(axis_angle_x_2);
    const double sin_axis_angle_x_2 = sin(axis_angle_x_2);
    first_tooth.push_back(QPointF(scale_factor * addendum_radius * cos(axis_angle),
                                  scale_factor * addendum_radius * sin(axis_angle)));
    for (i = first_tooth.size() - 1; i >= 0; --i) {
        const QPointF& orig(first_tooth[i]);

        QPointF target(cos_axis_angle_x_2 * orig.x() + sin_axis_angle_x_2 * orig.y(),
                       sin_axis_angle_x_2 * orig.x() - cos_axis_angle_x_2 * orig.y());
        first_tooth.push_back(target);
    } /* for */
    first_tooth.push_back(QPointF(scale_factor * dedendum_radius * cos(axis_angle + axis_angle_x_2),
                                  scale_factor * dedendum_radius * sin(axis_angle + axis_angle_x_2)));

    /* now, we have to rotate the tooth to get all the teeth missing.
     * XXX: The first point is going to be rotated 0.0 radians, which makes
     * the transformation useless, but it's only done once. That avoids to
     * repeat the code to copy the first tooth without transformation. */
    const double rotation = rotateBox->value() * M_PI / 180.0;
    for (i = 0; i < n_teeth; i++) {
        const double angle = M_PI * c_modulus * i;
        const double cos_angle = cos(angle + rotation);
        const double sin_angle = sin(angle + rotation);

        /* again, we dont use iterator as it.end() can be changing as we add
         * elements to it */
        for (std::vector<QPointF>::iterator it = first_tooth.begin();
                it != first_tooth.end(); ++it)
        {
            const QPointF& orig = *it;
            polyline.push_back(Plug_VertexData(
                        QPointF(cos_angle * orig.x() - sin_angle * orig.y() + center->x(),
                                sin_angle * orig.x() + cos_angle * orig.y() + center->y()),
                                               0.0));
        } /* for */
    } /* for */

    doc->addPolyline(polyline, true);

    QString lastLayer = doc->getCurrentLayer();

    doc->setLayer(QString("gear_pitch_circles"));

    if (drawPitchCircleBox->isChecked())
        doc->addCircle(center, scale_factor * pitch_radius);

    doc->setLayer(QString("gear_addendums"));

    if (drawAddendumCircleBox->isChecked())
        doc->addCircle(center, scale_factor * addendum_radius);

    doc->setLayer(QString("gear_dedendums"));

    if (drawRootCircleBox->isChecked())
        doc->addCircle(center, scale_factor * dedendum_radius);

    doc->setLayer(QString("gear_base_lines"));

    if (drawBaseCircleBox->isChecked())
        doc->addCircle(center, scale_factor * pitch_radius * cos_p_angle);

    doc->setLayer(QString("gear_action_lines"));

    if (drawPressureLineBox->isChecked()) {
        QPointF p1(scale_factor * cos(p_angle + rotation),
                   scale_factor * sin(p_angle + rotation)),
                p2(scale_factor * pitch_radius * cos(rotation),
                   scale_factor * pitch_radius * sin(rotation));
        p1 += *center; p2 += *center;
        doc->addLine(center, &p1);
        doc->addLine(&p1, &p2);
    }

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
    writeSettings();
    QWidget::closeEvent(event);
}


void lc_Geardlg::readSettings()
{
    QString str;
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LibreCAD", "gear_plugin");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(430,140)).toSize();

    nteethBox->setValue(settings.value("teeth", int(20)).toInt());
    modulusBox->setValue(settings.value("modulus", double(1.0)).toDouble());
    pressureBox->setValue(settings.value("pressure", double(20.0)).toDouble());
    addendumBox->setValue(settings.value("addendum", double(1.0)).toDouble());
    dedendumBox->setValue(settings.value("dedendum", double(1.25)).toDouble());
    n1Box->setValue(settings.value("n1", int(8)).toInt());
    n2Box->setValue(settings.value("n2", int(8)).toInt());
    drawAddendumCircleBox->setChecked(settings.value("draw_addendum", bool(false)).toBool());
    drawPitchCircleBox->setChecked(settings.value("draw_pitch", bool(true)).toBool());
    drawBaseCircleBox->setChecked(settings.value("draw_base", bool(true)).toBool());
    drawRootCircleBox->setChecked(settings.value("draw_root", bool(false)).toBool());
    drawPressureLineBox->setChecked(settings.value("draw_pressure_line", bool(false)).toBool());
    drawPressureLimitBox->setChecked(settings.value("draw_pressure_limit", bool(false)).toBool());

    resize(size);
    move(pos);
}

void lc_Geardlg::writeSettings()
 {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LibreCAD", "gear_plugin");
    settings.setValue("pos", pos());
    settings.setValue("size", size());

    settings.setValue("teeth", nteethBox->value());
    settings.setValue("modulus", modulusBox->value());
    settings.setValue("pressure", pressureBox->value());
    settings.setValue("addendum", addendumBox->value());
    settings.setValue("dedendum", dedendumBox->value());
    settings.setValue("n1", n1Box->value());
    settings.setValue("n2", n2Box->value());
    settings.setValue("draw_addendum", bool(drawAddendumCircleBox->isChecked()));
    settings.setValue("draw_pitch", bool(drawPitchCircleBox->isChecked()));
    settings.setValue("draw_base", bool(drawBaseCircleBox->isChecked()));
    settings.setValue("draw_root", bool(drawRootCircleBox->isChecked()));
    settings.setValue("draw_pressure_line", bool(drawPressureLineBox->isChecked()));
    settings.setValue("draw_pressure_limit", bool(drawPressureLimitBox->isChecked()));
 }
