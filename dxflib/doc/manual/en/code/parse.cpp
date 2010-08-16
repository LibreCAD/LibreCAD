MyDxfFilter f;
DL_Dxf dxf;
if (!dxf.in("drawing.dxf", &f)) {
	std::cerr << "drawing.dxf could not be opened.\n";
}
