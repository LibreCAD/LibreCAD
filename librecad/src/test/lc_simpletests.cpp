#include <iostream>
#include <cmath>
#include <fstream>
#include <QMenuBar>
#include "lc_simpletests.h"
#include "qc_applicationwindow.h"
#include "rs_graphic.h"
#include "rs_math.h"
#include "rs_arc.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_dimaligned.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimlinear.h"
#include "rs_dimradial.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_mtext.h"
#include "rs_point.h"
#include "rs_text.h"
#include "rs_entitycontainer.h"
#include "rs_layer.h"
#include "rs_graphicview.h"
#include "rs_debug.h"

LC_SimpleTests::LC_SimpleTests(QWidget *parent):
	QObject(parent)
{
	auto appWin=QC_ApplicationWindow::getAppWindow();
	QMenu* testMenu=appWin->menuBar()->addMenu(tr("De&bugging"));
	testMenu->setObjectName("Debugging");

		QAction* action = new QAction("Dump Entities", this);
		connect(action, SIGNAL(triggered()),
				this, SLOT(slotTestDumpEntities()));
		testMenu->addAction(action);

			action = new QAction("Dump Undo Info", this);
		connect(action, SIGNAL(triggered()),
				this, SLOT(slotTestDumpUndo()));
		testMenu->addAction(action);

		action = new QAction("Update Inserts", this);
		connect(action, SIGNAL(triggered()),
				this, SLOT(slotTestUpdateInserts()));
		testMenu->addAction(action);

			 action = new QAction("Draw Freehand", this);
		connect(action, SIGNAL(triggered()),
				this, SLOT(slotTestDrawFreehand()));
		testMenu->addAction(action);

			 action = new QAction("Draw Freehand", this);
		connect(action, SIGNAL(triggered()),
				this, SLOT(slotTestDrawFreehand()));
		testMenu->addAction(action);

		action = new QAction("Insert Block", this);

		connect(action, SIGNAL(triggered()),
				this, SLOT(slotTestInsertBlock()));
		testMenu->addAction(action);

		action = new QAction("Insert MText", this);
		connect(action, SIGNAL(triggered()),
				this, SLOT(slotTestInsertMText()));
		testMenu->addAction(action);

		action = new QAction("Insert Text", this);
		connect(action, SIGNAL(triggered()),
				this, SLOT(slotTestInsertText()));
		testMenu->addAction(action);

		action = new QAction(tr("Insert Image"), this);
		connect(action, SIGNAL(triggered()),
				this, SLOT(slotTestInsertImage()));
		testMenu->addAction(action);

		action = new QAction("Unicode", this);
		connect(action, SIGNAL(triggered()),
				this, SLOT(slotTestUnicode()));
		testMenu->addAction(action);

		action = new QAction("Insert Ellipse", this);
		connect(action, SIGNAL(triggered()),
				this, SLOT(slotTestInsertEllipse()));
		testMenu->addAction(action);

		action = new QAction("Math01", this);
		connect(action, SIGNAL(triggered()),
				this, SLOT(slotTestMath01()));
		testMenu->addAction(action);

		action = new QAction("Resize to 640x480", this);
		connect(action, SIGNAL(triggered()),
				this, SLOT(slotTestResize640()));
		testMenu->addAction(action);

		action = new QAction("Resize to 800x600", this);
		connect(action, SIGNAL(triggered()),
				this, SLOT(slotTestResize800()));
		testMenu->addAction(action);

		action = new QAction("Resize to 1024x768", this);
		connect(action, SIGNAL(triggered()),
				this, SLOT(slotTestResize1024()));
		testMenu->addAction(action);
}

/**
 * Testing function.
 */
void LC_SimpleTests::slotTestDumpEntities(RS_EntityContainer* d){

	int level = 0;
	std::ofstream dumpFile;
	if (d) {
		dumpFile.open("debug_entities.html", std::ios::app);
		++level;
	} else {
		d = QC_ApplicationWindow::getAppWindow()->getDocument();
		dumpFile.open("debug_entities.html");
		level = 0;
	}

	if (d) {
		if (level==0) {
			dumpFile << "<html>\n";
			dumpFile << "<body>\n";
		}

		for(auto e: *d){

			dumpFile << "<table border=\"1\">\n";
			dumpFile << "<tr><td>Entity: " << e->getId()
					 << "</td></tr>\n";

			dumpFile
					<< "<tr><td><table><tr>"
					<< "<td>VIS:" << e->isVisible() << "</td>"
					<< "<td>UND:" << e->isUndone() << "</td>"
					<< "<td>SEL:" << e->isSelected() << "</td>"
					<< "<td>TMP:" << e->getFlag(RS2::FlagTemp) << "</td>";
			QString lay = "NULL";
			if (e->getLayer()) {
				lay = e->getLayer()->getName();
			}
			dumpFile
					<< "<td>Layer: " << lay.toLatin1().data() << "</td>"
					<< "<td>Width: " << (int)e->getPen(false).getWidth() << "</td>"
					<< "<td>Parent: " << e->getParent()->getId() << "</td>"
					<< "</tr></table>";

			dumpFile
					<< "<tr><td>\n";

			switch (e->rtti()) {
			case RS2::EntityPoint: {
				RS_Point* p = (RS_Point*)e;
				dumpFile
						<< "<table><tr><td>"
						<< "<b>Point:</b>"
						<< "</td></tr>";
				dumpFile
						<< "<tr>"
						<< "<td>"
						<< p->getPos()
						<< "</td>"
						<< "</tr></table>";
			}
				break;

			case RS2::EntityLine: {
				RS_Line* l = (RS_Line*)e;
				dumpFile
						<< "<table><tr><td>"
						<< "<b>Line:</b>"
						<< "</td></tr>";
				dumpFile
						<< "<tr>"
						<< "<td>"
						<< l->getStartpoint()
						<< "</td>"
						<< "<td>"
						<< l->getEndpoint()
						<< "</td>"
						<< "</tr></table>";
			}
				break;

			case RS2::EntityArc: {
				RS_Arc* a = (RS_Arc*)e;
				dumpFile
						<< "<table><tr><td>"
						<< "<b>Arc:</b>"
						<< "</td></tr>";
				dumpFile
						<< "<tr>"
						<< "<td>Center: "
						<< a->getCenter()
						<< "</td>"
						<< "<td>Radius: "
						<< a->getRadius()
						<< "</td>"
						<< "<td>Angle 1: "
						<< a->getAngle1()
						<< "</td>"
						<< "<td>Angle 2: "
						<< a->getAngle2()
						<< "</td>"
						<< "<td>Startpoint: "
						<< a->getStartpoint()
						<< "</td>"
						<< "<td>Endpoint: "
						<< a->getEndpoint()
						<< "</td>"
						<< "<td>reversed: "
						<< (int)a->isReversed()
						<< "</td>"
						<< "</tr></table>";
			}
				break;

			case RS2::EntityCircle: {
				RS_Circle* c = (RS_Circle*)e;
				dumpFile
						<< "<table><tr><td>"
						<< "<b>Circle:</b>"
						<< "</td></tr>";
				dumpFile
						<< "<tr>"
						<< "<td>Center: "
						<< c->getCenter()
						<< "</td>"
						<< "<td>Radius: "
						<< c->getRadius()
						<< "</td>"
						<< "</tr></table>";
			}
				break;

			case RS2::EntityDimAligned: {
				RS_DimAligned* d = (RS_DimAligned*)e;
				dumpFile
						<< "<table><tr><td>"
						<< "<b>Dimension / Aligned:</b>"
						<< "</td></tr>";
				dumpFile
						<< "<tr>"
						<< "<td>"
						<< d->getDefinitionPoint()
						<< "</td>"
						<< "<td>"
						<< d->getExtensionPoint1()
						<< "</td>"
						<< "<td>"
						<< d->getExtensionPoint2()
						<< "</td>"
						<< "<td>Text: "
						<< d->getText().toLatin1().data()
						<< "</td>"
						<< "<td>Label: "
						<< d->getLabel().toLatin1().data()
						<< "</td>"
						<< "</tr></table>";
			}
				break;

			case RS2::EntityDimLinear: {
				RS_DimLinear* d = (RS_DimLinear*)e;
				dumpFile
						<< "<table><tr><td>"
						<< "<b>Dimension / Linear:</b>"
						<< "</td></tr>";
				dumpFile
						<< "<tr>"
						<< "<td>"
						<< d->getDefinitionPoint()
						<< "</td>"
						<< "<td>"
						<< d->getExtensionPoint1()
						<< "</td>"
						<< "<td>"
						<< d->getExtensionPoint2()
						<< "</td>"
						<< "<td>Text: "
						<< d->getText().toLatin1().data()
						<< "</td>"
						<< "<td>Label: "
						<< d->getLabel().toLatin1().data()
						<< "</td>"
						<< "</tr></table>";
			}
				break;

			case RS2::EntityInsert: {
				RS_Insert* i = (RS_Insert*)e;
				dumpFile
						<< "<table><tr><td>"
						<< "<b>Insert:</b>"
						<< "</td></tr>";
				dumpFile
						<< "<tr>"
						<< "<td>Insertion point:"
						<< i->getInsertionPoint()
						<< "</td>"
						<< "</tr></table>";
			}
				break;

			case RS2::EntityMText: {
				RS_MText* t = (RS_MText*)e;
				dumpFile
						<< "<table><tr><td>"
						<< "<b>Text:</b>"
						<< "</td></tr>";
				dumpFile
						<< "<tr>"
						<< "<td>Text:"
						<< t->getText().toLatin1().data()
						<< "</td>"
						<< "<td>Height:"
						<< t->getHeight()
						<< "</td>"
						<< "</tr></table>";
			}
				break;

			case RS2::EntityText: {
				RS_Text* t = (RS_Text*)e;
				dumpFile
						<< "<table><tr><td>"
						<< "<b>Text:</b>"
						<< "</td></tr>";
				dumpFile
						<< "<tr>"
						<< "<td>Text:"
						<< t->getText().toLatin1().data()
						<< "</td>"
						<< "<td>Height:"
						<< t->getHeight()
						<< "</td>"
						<< "</tr></table>";
			}
				break;

			case RS2::EntityHatch: {
				RS_Hatch* h = (RS_Hatch*)e;
				dumpFile
						<< "<table><tr><td>"
						<< "<b>Hatch:</b>"
						<< "</td></tr>";
				dumpFile
						<< "<tr>"
						<< "<td>Pattern:"
						<< h->getPattern().toLatin1().data()
						<< "</td>"
						<< "<td>Scale:"
						<< h->getScale()
						<< "</td>"
						<< "<td>Solid:"
						<< (int)h->isSolid()
						<< "</td>"
						<< "</tr></table>";
			}
				break;

			default:
				dumpFile
						<< "<tr><td>"
						<< "<b>Unknown Entity: " << e->rtti() << "</b>"
						<< "</td></tr>";
				break;
			}

			if (e->isContainer() || e->rtti()==RS2::EntityHatch) {
				RS_EntityContainer* ec = (RS_EntityContainer*)e;
				dumpFile << "<table><tr><td valign=\"top\">&nbsp;&nbsp;&nbsp;&nbsp;Contents:</td><td>\n";
				dumpFile.close();
				slotTestDumpEntities(ec);
				dumpFile.open("debug_entities.html", std::ios::app);
				dumpFile << "</td></tr></table>\n";
			}

			dumpFile
					<< "</td></tr>"
					<< "</table>\n"
					<< "<br><br>";
		}

		if (level==0) {
			dumpFile << "</body>\n";
			dumpFile << "</html>\n";
		} else {
			level--;
		}
	}
	RS_DEBUG->print("%s\n: end\n", __func__);
}

/**
 * Testing function.
 */
void LC_SimpleTests::slotTestDumpUndo() {
	RS_DEBUG->print("%s\n: begin\n", __func__);

	RS_Document* d = QC_ApplicationWindow::getAppWindow()->getDocument();
	if (d) {
		std::cout << *(RS_Undo*)d;
		std::cout << std::endl;
	}
	RS_DEBUG->print("%s\n: end\n", __func__);
}

/**
 * Testing function.
 */
void LC_SimpleTests::slotTestUpdateInserts() {
	RS_DEBUG->print("%s\n: begin\n", __func__);

	RS_Document* d = QC_ApplicationWindow::getAppWindow()->getDocument();
	if (d) {
		d->updateInserts();
	}
	RS_DEBUG->print("%s\n: end\n", __func__);
}

/**
 * Testing function.
 */
void LC_SimpleTests::slotTestDrawFreehand() {
	RS_DEBUG->print("%s\n: begin\n", __func__);

	//RS_Graphic* g = document->getMarking();
	/*

	   RS_ActionDrawLineFree* action =
		  new RS_ActionDrawLineFree(*document->getGraphic(),
									*graphicView);

	   for (int i=0; i<100; ++i) {

		   int posx = (random()%600);
		   int posy = (random()%400);

		   //QMouseEvent rsm1(posx, posy, LEFT);
		QMouseEvent rsm1(QEvent::MouseButtonPress,
						   QPoint(posx,posy),
						   RS2::LeftButton,
						   RS2::LeftButton);
		   action->mousePressEvent(&rsm1);

		   int speedx = 0;
		   int speedy = 0;

		   for (int k=0; k<100; ++k) {
			   int accx = (random()%40)-20;
			   int accy = (random()%40)-20;

			   speedx+=accx;
			   speedy+=accy;

			   posx+=speedx;
			   posy+=speedy;

			   //QMouseEvent rsm2(posx, posy, LEFT);

			QMouseEvent rsm2(QEvent::MouseMove,
						   QPoint(posx,posy),
						   RS2::LeftButton,
						   RS2::LeftButton);
			   action->mouseMoveEvent(&rsm2);
		   }

		   action->mouseReleaseEvent(NULL);

		   slotFileSave();
	   }

	   delete action;
	*/
	RS_DEBUG->print("%s\n: end\n", __func__);
}

/**
 * Testing function.
 */
void LC_SimpleTests::slotTestInsertBlock() {
	RS_DEBUG->print("%s\n: begin\n", __func__);
	auto appWin=QC_ApplicationWindow::getAppWindow();

	RS_Document* d = appWin->getDocument();
	if (d && d->rtti()==RS2::EntityGraphic) {
		RS_Graphic* graphic = (RS_Graphic*)d;
		if (graphic==NULL) {
			return;
		}

		graphic->addLayer(new RS_Layer("default"));
		RS_Block* block = new RS_Block(graphic, RS_BlockData("debugblock",
															 RS_Vector(0.0,0.0), true));

		RS_Line* line;
		RS_Arc* arc;
		RS_Circle* circle;

		// Add one red line:
		line = new RS_Line{block, {0.,0.}, {50.,0.}};
		line->setLayerToActive();
		line->setPen(RS_Pen(RS_Color(255, 0, 0),
							RS2::Width01,
							RS2::SolidLine));
		block->addEntity(line);

		// Add one line with attributes from block:
		line = new RS_Line{block, {50.,0.}, {50.,50.}};
		line->setPen(RS_Pen(RS_Color(RS2::FlagByBlock),
							RS2::WidthByBlock,
							RS2::LineByBlock));
		block->addEntity(line);

		// Add one arc with attributes from block:
		RS_ArcData d({50.,0.},
					 50.0, M_PI_2, M_PI,
					 false);
		arc = new RS_Arc(block, d);
		arc->setPen(RS_Pen(RS_Color(RS2::FlagByBlock),
						   RS2::WidthByBlock,
						   RS2::LineByBlock));
		block->addEntity(arc);

		// Add one blue circle:
		circle = new RS_Circle(block, {{20.0,15.0}, 12.5});
		circle->setLayerToActive();
		circle->setPen(RS_Pen(RS_Color(0, 0, 255),
							  RS2::Width01,
							  RS2::SolidLine));
		block->addEntity(circle);


		graphic->addBlock(block);



		RS_Insert* ins;
		RS_InsertData insData("debugblock",
							  RS_Vector(0.0,0.0),
							  RS_Vector(1.0,1.0), 0.0,
							  1, 1, RS_Vector(0.0, 0.0),
							  NULL, RS2::NoUpdate);

		// insert one magenta instance of the block (original):
		ins = new RS_Insert(graphic, insData);
		ins->setLayerToActive();
		ins->setPen(RS_Pen(RS_Color(255, 0, 255),
						   RS2::Width02,
						   RS2::SolidLine));
		ins->update();
		graphic->addEntity(ins);

		// insert one green instance of the block (rotate):
		insData = RS_InsertData("debugblock",
								RS_Vector(-50.0,20.0),
								RS_Vector(1.0,1.0), M_PI/6.,
								1, 1, RS_Vector(0.0, 0.0),
								NULL, RS2::NoUpdate);
		ins = new RS_Insert(graphic, insData);
		ins->setLayerToActive();
		ins->setPen(RS_Pen(RS_Color(0, 255, 0),
						   RS2::Width02,
						   RS2::SolidLine));
		ins->update();
		graphic->addEntity(ins);

		// insert one cyan instance of the block (move):
		insData = RS_InsertData("debugblock",
								RS_Vector(10.0,20.0),
								RS_Vector(1.0,1.0), 0.0,
								1, 1, RS_Vector(0.0, 0.0),
								NULL, RS2::NoUpdate);
		ins = new RS_Insert(graphic, insData);
		ins->setLayerToActive();
		ins->setPen(RS_Pen(RS_Color(0, 255, 255),
						   RS2::Width02,
						   RS2::SolidLine));
		ins->update();
		graphic->addEntity(ins);

		// insert one blue instance of the block:
		for (double a=0.0; a<360.0; a+=45.0) {
			insData = RS_InsertData("debugblock",
									RS_Vector(60.0,0.0),
									RS_Vector(2.0/5,2.0/5), RS_Math::deg2rad(a),
									1, 1, RS_Vector(0.0, 0.0),
									NULL, RS2::NoUpdate);
			ins = new RS_Insert(graphic, insData);
			ins->setLayerToActive();
			ins->setPen(RS_Pen(RS_Color(0, 0, 255),
							   RS2::Width05,
							   RS2::SolidLine));
			ins->update();
			graphic->addEntity(ins);
		}

		// insert an array of yellow instances of the block:
		insData = RS_InsertData("debugblock",
								RS_Vector(-100.0,-100.0),
								RS_Vector(0.2,0.2), M_PI/6.0,
								6, 4, RS_Vector(100.0, 100.0),
								NULL, RS2::NoUpdate);
		ins = new RS_Insert(graphic, insData);
		ins->setLayerToActive();
		ins->setPen(RS_Pen(RS_Color(255, 255, 0),
						   RS2::Width01,
						   RS2::SolidLine));
		ins->update();
		graphic->addEntity(ins);


		RS_GraphicView* v = appWin->getGraphicView();
		if (v) {
			v->redraw();
		}
	}
	RS_DEBUG->print("%s\n: end\n", __func__);
}

/**
 * Testing function.
 */
void LC_SimpleTests::slotTestInsertEllipse() {
	RS_DEBUG->print("%s\n: begin\n", __func__);
	auto appWin=QC_ApplicationWindow::getAppWindow();

	RS_Document* d = appWin->getDocument();
	if (d) {
		RS_Graphic* graphic = (RS_Graphic*)d;
		if (!graphic) {
			return;
		}

		RS_Ellipse* ellipse;
		RS_Line* line;

		for (double a=0.; a<2.*M_PI; a+=0.1) {
			RS_Vector v = RS_Vector::polar(50., a);
			double xp = 1000.*a;

			ellipse = new RS_Ellipse(graphic,
			{{xp,0.}, v,
			 0.5,
			 0., 2.*M_PI,
			 false}
									 );

			ellipse->setPen(RS_Pen(RS_Color(255, 0, 255),
								   RS2::Width01,
								   RS2::SolidLine));

			graphic->addEntity(ellipse);
			//graphic->addEntity(new RS_Point(graphic, ellipse->getMax()));
			//graphic->addEntity(new RS_Point(graphic, ellipse->getMin()));

			line = new RS_Line{graphic, {xp, 0.}, RS_Vector{xp, 0.}+v};
			line->setPen(RS_Pen(RS_Color(128, 128, 128),
								RS2::Width01,
								RS2::SolidLine));
			graphic->addEntity(line);


			/*
					 for (double mx=-60.0; mx<60.0; mx+=1.0) {
						 //for (double mx=0.0; mx<1.0; mx+=2.5) {
						 RS_VectorSolutions sol = ellipse->mapX(xp + mx);
						 //graphic->addEntity(new RS_Point(graphic,
						 //                   sol.vector2 + RS_Vector(a*500.0, 0.0)));
						 //graphic->addEntity(new RS_Point(graphic,
						 //                   sol.vector3 + RS_Vector(a*500.0, 0.0)));
						 //graphic->addEntity(new RS_Point(graphic,
						 //                   sol.vector4 + RS_Vector(a*500.0, 0.0)));

						 line = new RS_Line(graphic,
											RS_LineData(RS_Vector(xp+mx,-50.0),
														RS_Vector(xp+mx,50.0)));
						 line->setPen(RS_Pen(RS_Color(60, 60, 60),
											 RS2::Width01,
											 RS2::SolidLine));
						 graphic->addEntity(line);

						 graphic->addEntity(new RS_Point(graphic,
														 sol.get(0)));
					 }
			*/
		}


		// different minor/minor relations
		/*
			  double x, y;
			  for (y=-250.0; y<=250.0; y+=50.0) {
				  for (x=-250.0; x<=250.0; x+=50.0) {
					  RS_Vector v(x, y);

					  ellipse = new RS_Ellipse(graphic,
											   v,
											   RS_Vector((x/5+50.0)/2.0, 0.0),
										 fabs(x/y),
											   0.0, 2*M_PI,
											   false);

				ellipse->setPen(RS_Pen(RS_Color(255, 255, 0),
									   RS2::Width01,
									   RS2::DashDotLine));

					  graphic->addEntity(ellipse);
					  graphic->addEntity(new RS_Point(graphic, ellipse->getMax()));
					  graphic->addEntity(new RS_Point(graphic, ellipse->getMin()));

				ellipse = new RS_Ellipse(graphic,
											   v + RS_Vector(750.0, 0.0),
											   RS_Vector((x/5+50.0)/2.0, 0.0),
											   fabs(x/y),
											   2*M_PI, 0.0,
											   true);

					  graphic->addEntity(ellipse);
					  graphic->addEntity(new RS_Point(graphic, ellipse->getMax()));
					  graphic->addEntity(new RS_Point(graphic, ellipse->getMin()));
				  }
			  }
		*/

		/*
			  // different rotation angles:
			  double rot;
			  for (rot=0.0; rot<=2*M_PI+0.1; rot+=(M_PI/8)) {
				  ellipse = new RS_Ellipse(graphic,
										   RS_Vector(rot*200, 500.0),
										   RS_Vector(50.0, 0.0).rotate(rot),
										   0.3,
										   0.0, 2*M_PI,
										   false);
				  graphic->addEntity(ellipse);
				  graphic->addEntity(new RS_Point(graphic, ellipse->getMax()));
				  graphic->addEntity(new RS_Point(graphic, ellipse->getMin()));
			  }


			  // different arc angles:
			  double a1, a2;
			  for (rot=0.0; rot<=2*M_PI+0.1; rot+=(M_PI/8)) {
				  for (a1=0.0; a1<=2*M_PI+0.1; a1+=(M_PI/8)) {
					  for (a2=a1+M_PI/8; a2<=2*M_PI+a1+0.1; a2+=(M_PI/8)) {
						  ellipse = new RS_Ellipse(graphic,
												   RS_Vector(-500.0-a1*200.0-5000.0*rot,
															 500.0-a2*200.0),
												   RS_Vector(50.0, 0.0).rotate(rot),
												   0.3,
												   a1, a2,
												   false);
						  graphic->addEntity(ellipse);
						  graphic->addEntity(new RS_Point(graphic, ellipse->getMax()));
						  graphic->addEntity(new RS_Point(graphic, ellipse->getMin()));
					  }
				  }
			  }
		*/

		RS_GraphicView* v = appWin->getGraphicView();
		if (v) {
			v->redraw();
		}
	}
	RS_DEBUG->print("%s\n: end\n", __func__);
}

/**
 * Testing function.
 */
void LC_SimpleTests::slotTestInsertMText() {
	RS_DEBUG->print("%s\n: begin\n", __func__);

	RS_Document* d = QC_ApplicationWindow::getAppWindow()->getDocument();
	if (d) {
		RS_Graphic* graphic = (RS_Graphic*)d;
		if (graphic==NULL) {
			return;
		}

		RS_MText* text;
		RS_MTextData textData;

		textData = RS_MTextData(RS_Vector(10.0,10.0),
								10.0, 100.0,
								RS_MTextData::VATop,
								RS_MTextData::HALeft,
								RS_MTextData::LeftToRight,
								RS_MTextData::Exact,
								1.0,
								"LibreCAD",
								"iso",
								0.0);
		text = new RS_MText(graphic, textData);

		text->setLayerToActive();
		text->setPen(RS_Pen(RS_Color(255, 0, 0),
							RS2::Width01,
							RS2::SolidLine));
		graphic->addEntity(text);
	}
	RS_DEBUG->print("%s\n: end\n", __func__);
}

/**
 * Testing function.
 */
void LC_SimpleTests::slotTestInsertText() {
	RS_DEBUG->print("%s\n: begin\n", __func__);


	RS_Document* d = QC_ApplicationWindow::getAppWindow()->getDocument();
	if (d) {
		RS_Graphic* graphic = (RS_Graphic*)d;
		if (graphic==NULL) {
			return;
		}

		RS_Text* text;
		RS_TextData textData;

		textData = RS_TextData(RS_Vector(10.0,10.0),RS_Vector(10.0,10.0),
							   10.0, 1.0,
							   RS_TextData::VABaseline,
							   RS_TextData::HALeft,
							   RS_TextData::None,
							   "LibreCAD",
							   "iso",
							   0.0);
		text = new RS_Text(graphic, textData);

		text->setLayerToActive();
		text->setPen(RS_Pen(RS_Color(255, 0, 0),
							RS2::Width01,
							RS2::SolidLine));
		graphic->addEntity(text);

		/*
			  double x, y;
			  for (y=-250.0; y<=250.0; y+=50.0) {
				  for (x=-250.0; x<=250.0; x+=50.0) {
					  RS_Vector v(x, y);

					  textData = RS_TextData(v,
											 10.0, 100.0,
											 RS2::VAlignTop,
											 RS2::HAlignLeft,
											 RS2::LeftToRight,
											 RS2::Exact,
											 1.0,
											 "Andrew",
											 "normal",
											 0.0);

					  text = new RS_Text(graphic, textData);

					  text->setLayerToActive();
					  text->setPen(RS_Pen(RS_Color(255, 0, 0),
										  RS2::Width01,
										  RS2::SolidLine));
					  graphic->addEntity(text);
				  }
			  }

			  RS_Line* line;
			  for (x=0.0; x<M_PI*2.0; x+=0.2) {
				  RS_Vector v(600.0+cos(x)*50.0, 0.0+sin(x)*50.0);

				  line = new RS_Line(graphic,
									 RS_LineData(RS_Vector(600.0,0.0),
												 v));
				  line->setLayerToActive();
				  line->setPenToActive();
				  graphic->addEntity(line);

				  textData = RS_TextData(v,
										 5.0, 50.0,
										 RS2::VAlignTop,
										 RS2::HAlignLeft,
										 RS2::LeftToRight,
										 RS2::Exact,
										 1.0,
										 "Andrew",
										 "normal",
										 x);

				  text = new RS_Text(graphic, textData);

				  text->setLayerToActive();
				  text->setPen(RS_Pen(RS_Color(255, 0, 0),
									  RS2::Width01,
									  RS2::SolidLine));
				  graphic->addEntity(text);
			  }

			  RS_SolidData solidData = RS_SolidData(RS_Vector(5.0, 10.0),
													RS_Vector(25.0, 15.0),
													RS_Vector(15.0, 30.0));

			  RS_Solid* s = new RS_Solid(graphic, solidData);

			  s->setLayerToActive();
			  s->setPen(RS_Pen(RS_Color(255, 255, 0),
							   RS2::Width01,
							   RS2::SolidLine));
			  graphic->addEntity(s);

			  RS_GraphicView* v = getGraphicView();
			  if (v) {
				  v->redraw();
			  }
		*/
	}
	RS_DEBUG->print("%s\n: end\n", __func__);
}

/**
 * Testing function.
 */
void LC_SimpleTests::slotTestInsertImage() {
	RS_DEBUG->print("%s\n: begin\n", __func__);

	RS_Document* d = QC_ApplicationWindow::getAppWindow()->getDocument();
	if (d) {
		RS_Graphic* graphic = (RS_Graphic*)d;
		if (graphic==NULL) {
			return;
		}

		RS_Image* image;
		RS_ImageData imageData;

		imageData = RS_ImageData(0, RS_Vector(50.0,30.0),
								 RS_Vector(0.5,0.5),
								 RS_Vector(-0.5,0.5),
								 RS_Vector(640,480),
								 "/home/andrew/data/image.png",
								 50, 50, 0);
		image = new RS_Image(graphic, imageData);

		image->setLayerToActive();
		image->setPen(RS_Pen(RS_Color(255, 0, 0),
							 RS2::Width01,
							 RS2::SolidLine));
		graphic->addEntity(image);
	}
	RS_DEBUG->print("%s\n: end\n", __func__);
}

/**
 * Testing function.
 */
void LC_SimpleTests::slotTestUnicode() {
	RS_DEBUG->print("%s\n: begin\n", __func__);
	auto appWin= QC_ApplicationWindow::getAppWindow();

	appWin->slotFileOpen("./fonts/unicode.cxf", RS2::FormatCXF);
	RS_Document* d =appWin->getDocument();
	if (d) {
		RS_Graphic* graphic = (RS_Graphic*)d;
		if (graphic==NULL) {
			return;
		}

		RS_Insert* ins;

		int col;
		int row;
		QChar uCode;       // e.g. 65 (or 'A')
		QString strCode;   // unicde as string e.g. '[0041] A'

		graphic->setAutoUpdateBorders(false);

		for (col=0x0000; col<=0xFFF0; col+=0x10) {
			printf("col: %X\n", col);
			for (row=0x0; row<=0xF; row++) {
				//printf("  row: %X\n", row);

				uCode = QChar(col+row);
				//printf("  code: %X\n", uCode.unicode());

				strCode.setNum(uCode.unicode(), 16);
				while (strCode.length()<4) {
					strCode="0"+strCode;
				}
				strCode = "[" + strCode + "] " + uCode;

				if (graphic->findBlock(strCode)) {
					RS_InsertData d(strCode,
									RS_Vector(col/0x10*20.0,row*20.0),
									RS_Vector(1.0,1.0), 0.0,
									1, 1, RS_Vector(0.0, 0.0),
									NULL, RS2::NoUpdate);
					ins = new RS_Insert(graphic, d);
					ins->setLayerToActive();
					ins->setPen(RS_Pen(RS_Color(255, 255, 255),
									   RS2::Width01,
									   RS2::SolidLine));
					ins->update();
					graphic->addEntity(ins);
				}
			}
		}
		graphic->setAutoUpdateBorders(true);
		graphic->calculateBorders();
	}
	RS_DEBUG->print("%s\n: end\n", __func__);
}

/**
 * Testing function.
 */
void LC_SimpleTests::slotTestMath01() {
	RS_DEBUG->print("%s\n: begin\n", __func__);
	auto appWin=QC_ApplicationWindow::getAppWindow();
	RS_Document* d = appWin->getDocument();
	if (d) {
		RS_Graphic* graphic = (RS_Graphic*)d;
		if (!graphic) {
			return;
		}

		// axis
		graphic->addEntity(new RS_Line{graphic,
									   {0.,0.}, {2*M_PI,0.}});
		graphic->addEntity(new RS_Line{graphic, {0., -1.}, {0., 1.}});

		// cos
		double a;
		double x = RS_Math::deg2rad(59.0);
		double x_0 = RS_Math::deg2rad(60.0);
		for (a=0.01; a<2*M_PI; a+=0.01) {
			// cos curve:
			RS_Line* line = new RS_Line{graphic, {a-0.01, cos(a-0.01)},
			{a, cos(a)}};
			graphic->addEntity(line);

			// tangent:
			graphic->addEntity(new RS_Line{graphic,
										   {a-0.01,cos(x_0)-sin(x_0)*(a-0.01-x_0)},
										   {a,cos(x_0)-sin(x_0)*(a-x_0)}});
		}

		// 59.0 deg
		graphic->addEntity(new RS_Line{graphic, {x,0.}, {x,1.}});

		// 60.0 deg
		graphic->addEntity(new RS_Line{graphic, {x_0,0.}, {x_0,1.}});

		// tangent
		//graphic->addEntity(new RS_Line(graphic,
		//                   RS_Vector(0.0,cos(x_0)-sin(x_0)*(0.0-x_0)),
		//                   RS_Vector(6.0,cos(x_0)-sin(x_0)*(6.0-x_0))));


		RS_GraphicView* v = appWin->getGraphicView();
		if (v) {
			v->redraw();
		}
	}
	RS_DEBUG->print("%s\n: end\n", __func__);
}

/**
 * Testing function.
 */
void LC_SimpleTests::slotTestResize640() {
	RS_DEBUG->print("%s\n: begin\n", __func__);
	QC_ApplicationWindow::getAppWindow()->resize(640, 480);
	QC_ApplicationWindow::getAppWindow()->update();
	RS_DEBUG->print("%s\n: end\n", __func__);
}

/**
 * Testing function.
 */
void LC_SimpleTests::slotTestResize800() {
	RS_DEBUG->print("%s\n: begin\n", __func__);
	QC_ApplicationWindow::getAppWindow()->resize(800, 600);
	QC_ApplicationWindow::getAppWindow()->update();
	RS_DEBUG->print("%s\n: end\n", __func__);
}

/**
 * Testing function.
 */
void LC_SimpleTests::slotTestResize1024() {
	RS_DEBUG->print("%s\n: begin\n", __func__);
	QC_ApplicationWindow::getAppWindow()->resize(1024, 768);
	QC_ApplicationWindow::getAppWindow()->update();
	RS_DEBUG->print("%s\n: end\n", __func__);
}
