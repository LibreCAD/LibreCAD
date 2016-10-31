/*****************************************************************************/
/*  gear.cpp - plugin gear for LibreCAD                                    */
/*                                                                           */
/*  Copyright (C) 2016 Cédric Bosdonnat cedric@bosdonnat.fr                  */
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

    label = new QLabel(tr("Number of teeth"));
    mainLayout->addWidget(label, 0, 0);
    nteethBox = new QSpinBox();
    nteethBox->setMinimum(1);
    nteethBox->setMaximum(500);
    nteethBox->setSingleStep(1);
    mainLayout->addWidget(nteethBox, 0, 1);

    label = new QLabel(tr("Circular pitch"));
    mainLayout->addWidget(label, 1, 0);
    pitchBox = new QDoubleSpinBox();
    pitchBox->setMinimum(0.0);
    pitchBox->setMaximum(999999.999);
    pitchBox->setSingleStep(0.001);
    mainLayout->addWidget(pitchBox, 1, 1);

    label = new QLabel(tr("Pressure angle (deg)"));
    mainLayout->addWidget(label, 2, 0);
    pressureBox = new QDoubleSpinBox();
    pressureBox->setMinimum(0.0);
    pressureBox->setMaximum(45.0);
    pressureBox->setSingleStep(0.01);
    mainLayout->addWidget(pressureBox, 2, 1);

    label = new QLabel(tr("Addendum"));
    mainLayout->addWidget(label, 3, 0);
    addendumBox = new QDoubleSpinBox();
    addendumBox->setMinimum(0.0);
    addendumBox->setMaximum(9999.999);
    addendumBox->setSingleStep(0.001);
    mainLayout->addWidget(addendumBox, 3, 1);

    label = new QLabel(tr("Dedendum"));
    mainLayout->addWidget(label, 4, 0);
    dedendumBox = new QDoubleSpinBox();
    dedendumBox->setMinimum(0.0);
    dedendumBox->setMaximum(9999.999);
    dedendumBox->setSingleStep(0.001);
    mainLayout->addWidget(dedendumBox, 4, 1);

    label = new QLabel(tr("Type"));
    mainLayout->addWidget(label, 5, 0);
    typeBox = new QComboBox();
    typeBox->addItem(tr("Spur"), GEAR_TYPE_SPUR);
    typeBox->addItem(tr("Ring"), GEAR_TYPE_RING);
    mainLayout->addWidget(typeBox, 5, 1);

    QHBoxLayout *loaccept = new QHBoxLayout;
    QPushButton *acceptbut = new QPushButton(tr("Accept"));
    loaccept->addStretch();
    loaccept->addWidget(acceptbut);
    mainLayout->addLayout(loaccept, 6, 0);

    QPushButton *cancelbut = new QPushButton(tr("Cancel"));
    QHBoxLayout *locancel = new QHBoxLayout;
    locancel->addWidget(cancelbut);
    locancel->addStretch();
    mainLayout->addLayout(locancel, 6, 1);

    setLayout(mainLayout);
    readSettings();

    connect(cancelbut, SIGNAL(clicked()), this, SLOT(reject()));
    connect(acceptbut, SIGNAL(clicked()), this, SLOT(checkAccept()));
    connect(pitchBox, SIGNAL(valueChanged(double)), this, SLOT(pitchChanged(double)));
}

/** Computes the points of the involute.

    The generated points are corresponding to the involute
    of a circle centered on (0,0) with the base point being at
    the 0 angle. These points will then need to be translated
    and rotated for each tooth.
  */
static
std::vector< QPointF > computeInvolute( double diameter,
                                        double max,
                                        double min,
                                        int direction )
{
    std::vector< QPointF > involute;

    double u = 0;
    double r = 0;

    while ( r < max ) {
        QPointF p (diameter * (cos(u) + u * sin(u)) / 2,
                   diameter * (sin(u) - u * cos(u)) / 2);

        r = sqrt(pow(p.x(), 2) + pow(p.y(), 2));

        if (r >= min && r <= max)
            involute.push_back(p);

        u += direction * 0.01;
    }

    return involute;
}

static
std::vector<QPointF> createTooth(int nteeth,
                                 double circular_pitch,
                                 double pressure_angle,
                                 double addendum,
                                 double dedendum)
{
    std::vector<QPointF> tooth;

    double modulus = circular_pitch / M_PI;
    double pitch_diameter = nteeth * modulus;
    double base_radius = pitch_diameter * cos( pressure_angle ) / 2.0;
    double delta_angle = 2.0 * M_PI / nteeth;

    double top = pitch_diameter / 2.0 + addendum;
    double bottom = pitch_diameter / 2.0 - dedendum;

    std::vector<QPointF> involute = computeInvolute(base_radius * 2.0,
                                                    top, bottom, 1);

    std::vector<QPointF> rinvolute = computeInvolute(base_radius * 2.0,
                                                     top, bottom, -1);

    /* Draw one side */
    for (std::vector<QPointF>::iterator it = involute.begin();
         it < involute.end(); ++it ) {
        tooth.push_back(*it);
    }

    /* Draw the mirroring side */
    double tooth_angle = delta_angle * 0.75;

    QTransform trans;
    trans.rotateRadians(tooth_angle);
    for (std::vector< QPointF >::reverse_iterator it = rinvolute.rbegin();
         it < rinvolute.rend(); ++it)
        tooth.push_back((*it) * trans);

    return tooth;
}

static
QPointF mirror(QPointF src, QPointF axis1, QPointF axis2)

{
    QPointF direction = axis2 - axis1;
    QPointF v = src - axis1;

    double axisAngle = atan2(direction.y(), direction.x());
    double vAngle = atan2(v.y(), v.x());

    double angle = axisAngle - vAngle;
    double dist = sqrt(v.x() * v.x() + v.y() * v.y());

    QPointF dstVector(dist * cos(axisAngle + angle),
                      dist * sin(axisAngle + angle));
    return axis1 + dstVector;
}

void lc_Geardlg::processAction(Document_Interface *doc)
{
    Q_UNUSED(doc);

    std::vector<Plug_VertexData> polyline;

    int nteeth = nteethBox->value();
    double circular_pitch = pitchBox->value();
    double pressure_angle = M_PI / 180.0 * pressureBox->value();
    double addendum = addendumBox->value();
    double dedendum = dedendumBox->value();
    double delta_angle = 2.0 * M_PI / nteeth;
    int type = typeBox->currentIndex();

    /* Build one tooth */
    std::vector<QPointF> tooth = createTooth(nteeth,
                                             circular_pitch,
                                             pressure_angle,
                                             addendum,
                                             dedendum);

    double modulus = circular_pitch / M_PI;
    double pitch_diameter = nteeth * modulus;
    double tooth_angle = delta_angle * 0.75;
    double root_radius = pitch_diameter / 2.0 - dedendum;
    if (type == GEAR_TYPE_RING)
        root_radius = pitch_diameter / 2.0 + dedendum;
    QPointF axis1(pitch_diameter / 2.0, 0.0);
    QPointF axis2(pitch_diameter * cos(tooth_angle) / 2.0,
                  pitch_diameter * sin(tooth_angle) / 2.0);

    /* Create all other teeth by rotating the first one */
    for (int i = 0; i < nteeth; i++) {
        double angle_i = delta_angle * i;
        QTransform trans;
        trans.translate(center->x(), center->y());
        trans.rotateRadians(angle_i);

        QPointF pt(root_radius, 0.0);
        polyline.push_back(Plug_VertexData(pt * trans, 0.0));

        for (std::vector<QPointF>::iterator it = tooth.begin();
             it < tooth.end(); ++it) {
            QPointF pt = *it;
            if (type == GEAR_TYPE_RING) {
                pt = mirror(pt, axis1, axis2);
            }
            polyline.push_back(Plug_VertexData(pt * trans, 0.0));
        }

        pt = QPointF(root_radius * cos(tooth_angle),
                     root_radius * sin(tooth_angle));
        polyline.push_back(Plug_VertexData(pt * trans, 0.0));

        double step_angle = delta_angle * 0.25 / 10.0;
        for (size_t j = 0; j < 10; j++) {
            double ptAngle = tooth_angle + step_angle * (double)j;
            pt = QPointF(root_radius * cos(ptAngle),
                         root_radius * sin(ptAngle));
            polyline.push_back(Plug_VertexData(pt * trans, 0.0));
        }
    }

    doc->addPolyline(polyline, true);
}

void lc_Geardlg::checkAccept()
{
    accept();
}

void lc_Geardlg::pitchChanged(double pitch)
{
    double modulus = pitch / M_PI;

    /* Forced update of the addendum and dedendum. All user
     * changes are lost since the pitch could be drastically
     * different. */
    addendumBox->setValue(modulus);
    dedendumBox->setValue(modulus * 6.0 / 5.0);
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
    double pitch = settings.value("pitch", double(15.0)).toDouble();
    pitchBox->setValue(pitch);
    pressureBox->setValue(settings.value("pressure", double(20.0)).toDouble());
    pitchChanged(pitch);

    resize(size);
    move(pos);
 }

void lc_Geardlg::writeSettings()
 {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LibreCAD", "gear_plugin");
    settings.setValue("pos", pos());
    settings.setValue("size", size());

    settings.setValue("teeth", nteethBox->value());
    settings.setValue("pitch", pitchBox->value());
    settings.setValue("pressure", pressureBox->value());
 }
