"""Independent ezdxf 1.4.3 embedded ATTRIB/ATTDEF native-scope fixture."""
# Copyright (C) 2026 LibreCAD.org
# Copyright (C) 2026 Dongxu Li
# SPDX-License-Identifier: GPL-2.0-or-later
import sys
import ezdxf

BODY = r"embedded \H1.5x;body\Psecond"
OUTER = [(1000, "outer-owned"), (1071, 17)]
PROFILE = [(1000, "opaque outer profile"), (1070, 999)]


def generate(path):
    ezdxf.options.write_fixed_meta_data = True
    doc = ezdxf.new("R2018")
    doc.appids.new("OuterScope")
    doc.appids.new("LibreCADText")
    doc.styles.new("OUTER_STYLE", dxfattribs={"font": "txt"})
    doc.styles.new("EMBED_STYLE", dxfattribs={"font": "txt"})
    block = doc.blocks.new("NATIVE_SCOPE")
    definition = block.add_attdef("DEF_TAG", insert=(0, 0), text="definition fallback")
    reference = doc.modelspace().add_blockref("NATIVE_SCOPE", insert=(5, 7))
    attribute = reference.add_attrib("TAG", "attribute fallback", insert=(5, 7))
    for entity in (definition, attribute):
        mtext = doc.modelspace().add_mtext(BODY, dxfattribs={
            "insert": (5, 7, 0), "char_height": 2, "width": 20,
            "style": "EMBED_STYLE", "attachment_point": 1,
            "flow_direction": 3, "line_spacing_style": 2,
            "line_spacing_factor": 1.5,
        })
        entity.set_mtext(mtext, graphic_properties=False)
        doc.modelspace().delete_entity(mtext)
        entity.dxf.text = "OUTER-FALLBACK"
        entity.dxf.style = "OUTER_STYLE"
        entity.set_xdata("OuterScope", OUTER)
        entity.set_xdata("LibreCADText", PROFILE)
    doc.saveas(path)
    check(path)


def check(path):
    doc = ezdxf.readfile(path)
    found = {}
    for entity in doc.entitydb.values():
        if not entity.is_alive or entity.dxftype() not in ("ATTRIB", "ATTDEF"):
            continue
        kind = entity.dxftype()
        assert kind not in found, kind
        assert entity.has_embedded_mtext_entity, kind
        assert entity.virtual_mtext_entity().text == BODY, kind
        assert entity.virtual_mtext_entity().dxf.style == "EMBED_STYLE", kind
        assert entity.dxf.text == "OUTER-FALLBACK", kind
        assert entity.dxf.style == "OUTER_STYLE", kind
        assert list(entity.get_xdata("OuterScope")) == OUTER, kind
        assert list(entity.get_xdata("LibreCADText")) == PROFILE, kind
        found[kind] = entity.dxf.handle
    assert set(found) == {"ATTRIB", "ATTDEF"}, found
    errors = doc.audit().errors
    assert not errors, errors
    print(f"ezdxf {ezdxf.__version__}: native embedded text and outer namespaces verified: {found}")


if __name__ == "__main__":
    if len(sys.argv) != 3 or sys.argv[1] not in ("generate", "check"):
        raise SystemExit("usage: fixture.py generate|check FILE.dxf")
    globals()[sys.argv[1]](sys.argv[2])
