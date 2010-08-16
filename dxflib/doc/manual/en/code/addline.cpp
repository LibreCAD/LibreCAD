void MyDxfFilter::addLine(const DL_LineData& d) {
    std::cout << "Line: " << d.x1 << "/" << d.y1
    << " " << d.x2 << "/" << d.y2 << std::endl;
}
