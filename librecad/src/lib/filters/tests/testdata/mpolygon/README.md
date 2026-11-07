# MPOLYGON test fixtures

The nine DWGs in this directory exercise DRW_MPolygon::parseDwg across the
AcDbMPolygon versions ODA File Converter emits (AC1015 through AC1032). They
cover the ODA body layout decoded by LibreDWG and ACadSharp: a leading version
BS, is_closed/hasBulge/vertex loop records (no derived-boundary bit), a single
pattern BS without a second hstyle BS, and the fill-CMC / x-direction /
degenerate-path trailer.

## Provenance

The source DXFs were written with ezdxf 1.4.4 for the MPOLYGON regressions and
contain no imported drawing data. Each file name says what it holds.

Each DXF was converted to DWG with ODA File Converter 27.1:

```
ODAFileConverter <in-dir> <out-dir> ACAD2000|ACAD2004|ACAD2010|ACAD2018 DWG 0 1
```

The SHA-256 of each file is recorded next to the existing `mpolygon_solid.dwg`
row in `../PROVENANCE.md`.
