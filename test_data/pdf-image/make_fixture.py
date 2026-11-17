#!/usr/bin/env python3
"""Writes red-square.pdf, one 100 x 100 pt page filled red, and pdf-image.dxf,
one 100 x 100 unit IMAGE entity that shows it. The DXF half needs ezdxf
(pip install ezdxf), a development-only tool; CI uses the committed files."""
import ezdxf

content = b"1 0 0 rg 0 0 100 100 re f\n"
objects = [
    b"<< /Type /Catalog /Pages 2 0 R >>",
    b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
    b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 100 100] /Contents 4 0 R /Resources << >> >>",
    b"<< /Length %d >>\nstream\n" % len(content) + content + b"endstream",
]
pdf = bytearray(b"%PDF-1.4\n%\xe2\xe3\xcf\xd3\n")
offsets = []
for number, body in enumerate(objects, 1):
    offsets.append(len(pdf))
    pdf += b"%d 0 obj\n" % number + body + b"\nendobj\n"
xref = len(pdf)
pdf += b"xref\n0 %d\n0000000000 65535 f \n" % (len(objects) + 1)
pdf += b"".join(b"%010d 00000 n \n" % offset for offset in offsets)
pdf += b"trailer\n<< /Size %d /Root 1 0 R >>\nstartxref\n%d\n%%%%EOF\n" % (len(objects) + 1, xref)
with open("red-square.pdf", "wb") as out:
    out.write(pdf)

doc = ezdxf.new("R2000")
image_def = doc.add_image_def(filename="red-square.pdf", size_in_pixel=(100, 100))
doc.modelspace().add_image(image_def=image_def, insert=(0, 0), size_in_units=(100, 100))
doc.saveas("pdf-image.dxf")