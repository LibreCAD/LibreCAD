#!/usr/bin/env python3
"""Apply authoritative Lao CAD glossary to LibreCAD .ts translation files.

Edits <translation> text only; preserves XML structure, locations, and
placeholders. Implements deep-review fixes (Absolute family, Dimension
compounds, Layer hierarchy, Undo errors, Relative typos, Snap synonyms).

  python3 scripts/lao_cad_glossary_apply.py librecad/ts/librecad_lo.ts
  python3 scripts/lao_cad_glossary_apply.py plugins/ts/plugins_lo.ts
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

# Exact English source → preferred Lao (menu labels, snap names, core nouns).
EXACT_SOURCE: dict[str, str] = {
    "Layer": "ເລເຢີ",
    "Layers": "ເລເຢີ",
    "layer": "ເລເຢີ",
    "layers": "ເລເຢີ",
    "Entity": "ອົງປະກອບ",
    "Entities": "ອົງປະກອບ",
    "entity": "ອົງປະກອບ",
    "entities": "ອົງປະກອບ",
    "Block": "ບລັອກ",
    "Blocks": "ບລັອກ",
    "Dimension": "ມິຕິ",
    "Dimensions": "ມິຕິ",
    "Polyline": "ເສັ້ນຕໍ່ເນື່ອງ",
    "polyline": "ເສັ້ນຕໍ່ເນື່ອງ",
    "Spline": "ເສັ້ນໂຄ້ງສະປຼາຍ",
    "spline": "ເສັ້ນໂຄ້ງສະປຼາຍ",
    "Hatch": "ລວດລາຍ",
    "hatch": "ລວດລາຍ",
    "Insert": "ແຊກ",
    "Snap": "ເກາະຈັບ",
    "Grid": "ຕາຕະລາງ",
    "Viewport": "ວິວພອດ",
    "UCS": "UCS",
    "Undo": "ເລີກທຳ",
    "Redo": "ເຮັດຄືນ",
    "&Undo": "&ເລີກທຳ",
    "&Redo": "&ເຮັດຄືນ",
    "Absolute": "ແທ້ຈິງ",
    "Relative": "ສຳພັນ",
    "Absolute:": "ແທ້ຈິງ:",
    "Relative:": "ສຳພັນ:",
    "Absolute Pos": "ຕຳແໜ່ງແທ້ຈິງ",
    "Center": "ໃຈກາງ",
    "Center (large)": "ເສັ້ນໃຈກາງ (ໃຫຍ່)",
    "Center (small)": "ເສັ້ນໃຈກາງ (ນ້ອຍ)",
    "Tangential": "ສຳຜັດ",
    "Tangental": "ສຳຜັດ",  # EN typo in sources
    "Tangential arc": "ເສັ້ນໂຄ້ງສຳຜັດ",
    "Endpoint": "ຈຸດປາຍ",
    "Middle": "ຈຸດກາງ",
    "Intersection": "ຈຸດຕັດ",
    "Nearest": "ໃກ້ທີ່ສຸດ",
    "Perpendicular": "ຕັ້ງສາກ",
    "Tangent": "ສຳຜັດ",
    "Node": "ໂຫນດ",
    "On Entity": "ທີ່ອົງປະກອບ",
    "On entity": "ທີ່ອົງປະກອບ",
    "Snap On Entity": "ເກາະຈັບທີ່ອົງປະກອບ",
    "Snap on Entity": "ເກາະຈັບທີ່ອົງປະກອບ",
    "File": "ໄຟລ໌",
    "Edit": "ແກ້ໄຂ",
    "View": "ມຸມມອງ",
    "Draw": "ແຕ້ມ",
    "Modify": "ປັບປ່ຽນ",
    "Tools": "ເຄື່ອງມື",
    "Help": "ຊ່ວຍເຫຼືອ",
    "Options": "ຕົວເລືອກ",
    "Preferences": "ການຕັ້ງຄ່າ",
    "Save": "ບັນທຶກ",
    "Open": "ເປີດ",
    "Close": "ປິດ",
    "Cancel": "ຍົກເລີກ",
    "Delete": "ລຶບ",
    "Copy": "ຄັດລອກ",
    "Paste": "ວາງ",
    "Cut": "ຕັດ",
    "Select": "ເລືອກ",
    "Line": "ເສັ້ນ",
    "Circle": "ວົງມົນ",
    "Arc": "ເສັ້ນໂຄ້ງ",
    "Ellipse": "ວົງລີ",
    "Point": "ຈຸດ",
    "Text": "ຂໍ້ຄວາມ",
    "Radius": "ລັດສະໝີ",
    "Diameter": "ເສັ້ນຜ່ານສູນກາງ",
    "Angle": "ມຸມ",
    "Length": "ຄວາມຍາວ",
    "Distance": "ໄລຍະຫ່າງ",
    "Scale": "ມາດຕາສ່ວນ",
    "Move": "ຍ້າຍ",
    "Rotate": "ໝຸນ",
    "Mirror": "ສະທ້ອນ",
    "Offset": "ອອບເຊັດ",
    "Trim": "ຕັດຂອບ",
    "Extend": "ຂະຫຍາຍ",
    "Explode": "ແຍກ",
    "Export": "ສົ່ງອອກ",
    "Import": "ນຳເຂົ້າ",
    "Print": "ພິມ",
    "Zoom In": "ຊູມເຂົ້າ",
    "Zoom Out": "ຊູມອອກ",
    "Auto Zoom": "ຊູມອັດຕະໂນມັດ",
    "Zoom Window": "ຊູມໜ້າຕ່າງ",
    "Coordinates": "ພິກັດ",
    "Library": "ຄັງສະໝຸດ",
    "Attributes": "ຄຸນລັກສະນະ",
    "Properties": "ຄຸນສົມບັດ",
    "Align": "ຈັດວາງ",
    "POLYLINE": "ເສັ້ນຕໍ່ເນື່ອງ",
    "SPLINE": "ເສັ້ນໂຄ້ງສະປຼາຍ",
    "SPLINEPOINTS": "ຈຸດເສັ້ນໂຄ້ງສະປຼາຍ",
    "HATCH": "ລວດລາຍ",
    "Empty Entity": "ອົງປະກອບຫວ່າງ",
    "Nothing to undo!": "ບໍ່ມີຫຍັງໃຫ້ເລີກທຳ!",
    "Nothing to redo!": "ບໍ່ມີຫຍັງໃຫ້ເຮັດຄືນ!",
    "Show absolute coordinate": "ສະແດງພິກັດແທ້ຈິງ",
    "Show relative coordinate": "ສະແດງພິກັດສຳພັນ",
    "Absolute Coordinates (Cartesian)": "ພິກັດແທ້ຈິງ (ຄາທີຊຽນ)",
    "Absolute Cordinates (Polar)": "ພິກັດແທ້ຈິງ (ໂພລາ)",
    "Relative Coordinates (Cartesian)": "ພິກັດສຳພັນ (ຄາທີຊຽນ)",
    "Relative Coordinates (Polar)": "ພິກັດສຳພັນ (ໂພລາ)",
    "Relative angle": "ມຸມສຳພັນ",
    "Regenerate Dimensions": "ສ້າງມິຕິໃໝ່ຄືນ",
    "&Dimension Styles": "&ຮູບແບບມິຕິ",
    "Dimension Styles Export": "ສົ່ງອອກຮູບແບບມິຕິ",
    "Dimension Styles Import": "ນຳເຂົ້າຮູບແບບມິຕິ",
    "Dimension Styles Export Error": "ຂໍ້ຜິດພາດໃນການສົ່ງອອກຮູບແບບມິຕິ",
    "Dimension Styles Import Error": "ຂໍ້ຜິດພາດໃນການນຳເຂົ້າຮູບແບບມິຕິ",
    # Score-pass: delete / centerline / snap UI / paper command aliases
    "OK": "ຕົກລົງ",
    "&OK": "&ຕົກລົງ",
    "Remove": "ລຶບ",
    "Centerline": "ເສັ້ນໃຈກາງ",
    "Free Snap": "ເກາະຈັບອິດສະຫຼະ",
    "Snap:": "ເກາະຈັບ:",
    "Start Snap:": "ເກາະຈັບເລີ່ມຕົ້ນ:",
    "End Snap:": "ເກາະຈັບສິ້ນສຸດ:",
    "Snap shift": "ເລື່ອນເກາະຈັບ",
    "Tick snap:": "ເກາະຈັບຂີດໝາຍ:",
    "Snap Selection": "ການເລືອກເກາະຈັບ",
    "Snap to Adjacent Dim": "ເກາະຈັບໃສ່ມິຕິຂ້າງຄຽງ",
    # Keep paper-size command tokens in English (do not invent)
    "arch a": "arch a",
    "arch b": "arch b",
    "arch c": "arch c",
    "arch d": "arch d",
    "arch e": "arch e",
    "Legal": "Legal",
    "legal": "legal",
    # Plugin metric labels
    "length": "ຄວາມຍາວ",
    "radius": "ລັດສະໝີ",
    "circumference": "ເສັ້ນຮອບວົງ",
    "area": "ເນື້ອທີ່",
    "places": "ຕຳແໜ່ງ",  # divide plugin: division count, not decimal places
}

EXACT_SOURCE_CI: dict[str, str] = {k.lower(): v for k, v in EXACT_SOURCE.items()}

# Global phrase replacements (longest first). Safe across contexts.
PHRASE_REPLACEMENTS: list[tuple[str, str]] = [
    # Undo family
    ("ບໍ່ສາມາດຍ້ອນກັບໄດ້", "ບໍ່ສາມາດຍົກເລີກໄດ້"),
    ("ບໍ່ມີຫຍັງໃຫ້ຍ້ອນກັບ", "ບໍ່ມີຫຍັງໃຫ້ຍົກເລີກ"),
    ("ຍ້ອນກັບໄດ້", "ຍົກເລີກໄດ້"),
    ("ໃຫ້ຍ້ອນກັບ", "ໃຫ້ຍົກເລີກ"),
    ("ເລີກເຮັດການແຕ້ມ", "ຍົກເລີກການແຕ້ມ"),
    ("ເລີກເຮັດສຳລັບ", "ຍົກເລີກສຳລັບ"),
    ("ເລີກເຮັດຈຸດ", "ຍົກເລີກຈຸດ"),
    ("ເລີກເຮັດ", "ຍົກເລີກ"),
    ("ບໍ່ສາມາດເລີກເຮັດ", "ບໍ່ສາມາດຍົກເລີກ"),
    # Absolute coordinate family (not "fully complete")
    ("ສຳມະບູນ", "ແທ້ຈິງ"),
    ("ຕຳແໜ່ງສຳບູນ", "ຕຳແໜ່ງແທ້ຈິງ"),
    ("ພິກັດສຳບູນ", "ພິກັດແທ້ຈິງ"),
    ("ຈຸດອ້າງອີງສຳບູນ", "ຈຸດອ້າງອີງແທ້ຈິງ"),
    ("ຈຸດສູນສຳບູນ", "ຈຸດສູນແທ້ຈິງ"),
    ("ແບບສຳບູນ", "ແບບແທ້ຈິງ"),
    ("WCS ສຳບູນ", "WCS ແທ້ຈິງ"),
    ("ສຳບູນ:", "ແທ້ຈິງ:"),
    ("ສຳບູນ: (", "ແທ້ຈິງ: ("),
    # Relative typos (NOT for tangent — see transform_translation)
    # Do not globally map ສຳພັດ → ສຳພັນ (collides with tangent ສຳຜັດ).
    ("ມູມສຳພັນ", "ມຸມສຳພັນ"),
    ("ມູມສໍາພັນ", "ມຸມສຳພັນ"),
    # Dimension compounds (size → dimensioning)
    ("ເສັ້ນບອກຂະໜາດ", "ເສັ້ນມິຕິ"),
    ("ເສັ້ນໂຄ້ງຂະໜາດ", "ເສັ້ນໂຄ້ງມິຕິ"),
    ("ເສັ້ນຂະໜາດ", "ເສັ້ນມິຕິ"),
    ("ຂໍ້ຄວາມຂະໜາດ", "ຂໍ້ຄວາມມິຕິ"),
    ("ຮູບແບບຂະໜາດ", "ຮູບແບບມິຕິ"),
    ("ສ້າງຂະໜາດໃໝ່", "ສ້າງມິຕິໃໝ່"),
    ("ອົງປະກອບຂະໜາດ", "ອົງປະກອບມິຕິ"),
    ("ຂະໜາດ (Dimensions)", "ມິຕິ"),
    # Layer variants
    ("ຊັ້ນວຽກຂອງວັດຖຸ", "ເລເຢີຂອງອົງປະກອບ"),
    ("ຊັ້ນດັ້ງເດີມ", "ເລເຢີດັ້ງເດີມ"),
    ("ຊັ້ນຕົ້ນສະບັບ", "ເລເຢີຕົ້ນສະບັບ"),
    ("ຊັ້ນວຽກ", "ເລເຢີ"),
    ("ຊັ້ນວາງ", "ເລເຢີ"),
    ("ຊັ້ນງານ", "ເລເຢີ"),
    ("ລາຍການຊັ້ນ", "ລາຍການເລເຢີ"),
    ("ລຳດັບຊັ້ນ", "ລຳດັບເລເຢີ"),
    ("ລະດັບຊັ້ນ", "ລະດັບເລເຢີ"),
    ("ຊື່ຊັ້ນ", "ຊື່ເລເຢີ"),
    ("ຊັ້ນສຳຮອງ", "ເລເຢີສຳຮອງ"),
    ("ຂອງຊັ້ນ", "ຂອງເລເຢີ"),
    ("ເລຢີ", "ເລເຢີ"),
    ("ເລເຍີ", "ເລເຢີ"),
    ("ເລເຢີ (Layer)", "ເລເຢີ"),
    ("ຊັ້ນ (Layers)", "ເລເຢີ"),
    ("ບລັອກ (Blocks)", "ບລັອກ"),
    # Entity loanwords
    ("ອອບເຈັກ", "ອົງປະກອບ"),
    ("ເອນຕິຕີ", "ອົງປະກອບ"),
    ("ວັດຖຸ", "ອົງປະກອບ"),
    # Spline
    ("ສະພລາຍ", "ສະປຼາຍ"),
    ("ສະປາຍ", "ສະປຼາຍ"),
    ("ເສັ້ນໂຄ້ງສະປຼາຍສະປຼາຍ", "ເສັ້ນໂຄ້ງສະປຼາຍ"),
    # Do NOT globally strip ເສັ້ນໃຈກາງ → ໃຈກາງ (breaks Centerline labels).
    # Snap "Center" is handled via EXACT_SOURCE / Center (large|small) rules.
    ("ພິກັດສົມບູນ", "ພິກັດແທ້ຈິງ"),
    # Snap synonym → glossary term
    ("ການດຶງເຂົ້າຫາ (snap)", "ເກາະຈັບ"),
    ("ການດຶງເຂົ້າຫາ", "ເກາະຈັບ"),
    ("ຈຸດດຶງເຂົ້າຫາ", "ຈຸດເກາະຈັບ"),
    ("ພິກັດການດຶງເຂົ້າຫາ", "ພິກັດເກາະຈັບ"),
    ("ສະແນັບ", "ເກາະຈັບ"),
    # Orthography
    ("ກໍານົດ", "ກຳນົດ"),
    ("ໂຫລດ", "ໂຫຼດ"),
    # Hatch / grid
    ("ລວດລາຍ (Hatch)", "ລວດລາຍ"),
    ("ລວດລາຍ (HATCH)", "ລວດລາຍ"),
    ("ເສັ້ນຕາຕະລາງ", "ຕາຕະລາງ"),
    ("ເສັ້ນຜ່ານໃຈກາງ", "ເສັ້ນຜ່ານສູນກາງ"),
    ("  ", " "),
]

# Dimension-specific replacements only when EN source mentions dimension.
DIM_PHRASES: list[tuple[str, str]] = [
    ("ເສັ້ນບອກຂະໜາດ", "ເສັ້ນມິຕິ"),
    ("ເສັ້ນໂຄ້ງຂະໜາດ", "ເສັ້ນໂຄ້ງມິຕິ"),
    ("ເສັ້ນຂະໜາດ", "ເສັ້ນມິຕິ"),
    ("ຂໍ້ຄວາມຂະໜາດ", "ຂໍ້ຄວາມມິຕິ"),
    ("ຮູບແບບຂະໜາດ", "ຮູບແບບມິຕິ"),
    ("ສ້າງຂະໜາດໃໝ່", "ສ້າງມິຕິໃໝ່"),
    ("ອົງປະກອບຂະໜາດ", "ອົງປະກອບມິຕິ"),
    ("ກ່ຽວກັບຂະໜາດ", "ກ່ຽວກັບມິຕິ"),
    ("ລະບຸຂະໜາດ", "ລະບຸມິຕິ"),
    ("ເລືອກຂະໜາດ", "ເລືອກມິຕິ"),
    ("ຂະໜາດເສັ້ນ", "ມິຕິເສັ້ນ"),
    ("ຂະໜາດແນວ", "ມິຕິແນວ"),
]

# Absolute-specific (EN has absolute) — avoid breaking "fully complete" ສົມບູນ
ABS_PHRASES: list[tuple[str, str]] = [
    ("ສຳມະບູນ", "ແທ້ຈິງ"),
    ("ຕຳແໜ່ງສຳບູນ", "ຕຳແໜ່ງແທ້ຈິງ"),
    ("ພິກັດສຳບູນ", "ພິກັດແທ້ຈິງ"),
    ("ຈຸດອ້າງອີງສຳບູນ", "ຈຸດອ້າງອີງແທ້ຈິງ"),
    ("ແບບສຳບູນ", "ແບບແທ້ຈິງ"),
    ("WCS ສຳບູນ", "WCS ແທ້ຈິງ"),
    ("ສຳບູນ:", "ແທ້ຈິງ:"),
    ("ສຳບູນ", "ແທ້ຈິງ"),  # after longer phrases
]

# Layer-specific residual ຊັ້ນ when EN mentions layer
LAYER_PHRASES: list[tuple[str, str]] = [
    ("ລາຍການຊັ້ນ", "ລາຍການເລເຢີ"),
    ("ລຳດັບຊັ້ນ", "ລຳດັບເລເຢີ"),
    ("ລະດັບຊັ້ນ", "ລະດັບເລເຢີ"),
    ("ຊື່ຊັ້ນ", "ຊື່ເລເຢີ"),
    ("ຊັ້ນສຳຮອງ", "ເລເຢີສຳຮອງ"),
    ("ຊັ້ນດັ້ງເດີມ", "ເລເຢີດັ້ງເດີມ"),
    ("ຂອງຊັ້ນ", "ຂອງເລເຢີ"),
    ("ໃນຊັ້ນ", "ໃນເລເຢີ"),
    ("ແມ່ນຊັ້ນ", "ແມ່ນເລເຢີ"),
    ("ວ່າຊັ້ນ", "ວ່າເລເຢີ"),
    ("ສຳລັບຊັ້ນ", "ສຳລັບເລເຢີ"),
    ("ເປັນຊັ້ນ", "ເປັນເລເຢີ"),
    ("ມີຊັ້ນ", "ມີເລເຢີ"),
    ("ຊັ້ນໂດຍ", "ເລເຢີໂດຍ"),
    ("ຊັ້ນແບບ", "ເລເຢີແບບ"),
    ("ຊັ້ນໃຫ້", "ເລເຢີໃຫ້"),
    ("ຊັ້ນນັ້ນ", "ເລເຢີນັ້ນ"),
    ("ຊັ້ນ ", "ເລເຢີ "),
    (" ຊັ້ນ", " ເລເຢີ"),
]

# Plugin test-tip cleanup: source ends with "test tip..."
TEST_TIP_TR = "ຄຳແນະນຳ (ສຳລັບການທົດສອບ)"


def unescape_src(s: str) -> str:
    return (
        s.replace("&amp;", "&")
        .replace("&lt;", "<")
        .replace("&gt;", ">")
        .replace("&quot;", '"')
        .replace("&apos;", "'")
    )


def apply_list(tr: str, pairs: list[tuple[str, str]]) -> str:
    for old, new in pairs:
        if old in tr:
            tr = tr.replace(old, new)
    return tr


def transform_translation(source: str, translation: str) -> str:
    src = unescape_src(source).strip()
    if not translation.strip():
        return translation

    # Exact glossary
    if src in EXACT_SOURCE:
        return EXACT_SOURCE[src]
    if src.startswith("&") and src[1:] in EXACT_SOURCE:
        return "&" + EXACT_SOURCE[src[1:]]
    if len(src) < 40 and src.lower() in EXACT_SOURCE_CI:
        return EXACT_SOURCE_CI[src.lower()]

    # Plugin / stub test tips
    if re.search(r"test tip\.\.\.$", src, re.I):
        return TEST_TIP_TR

    tr = translation
    tr = apply_list(tr, PHRASE_REPLACEMENTS)

    # --- Source-keyword compound rules (deep review D1–D5) ---
    if re.search(r"\babsolute\b", src, re.I):
        tr = apply_list(tr, ABS_PHRASES)
        # bare leftovers after colon forms
        if tr.strip() in ("ສຳບູນ", "ສົມບູນ", "ສຳເລັດ") and re.fullmatch(
            r"absolute[:\s].*|absolute", src, re.I
        ):
            tr = "ແທ້ຈິງ" + (":" if src.rstrip().endswith(":") else "")
        if re.search(r"absolute\s*:", src, re.I) and tr.strip().startswith("ສຳບູນ"):
            tr = tr.replace("ສຳບູນ", "ແທ້ຈິງ", 1)

    if re.search(r"\bdimensions?\b", src, re.I):
        tr = apply_list(tr, DIM_PHRASES)
        if tr.strip() == "ຂະໜາດ":
            tr = "ມິຕິ"
        # remaining bare size word in short dim UI
        if len(src) < 80 and "ຂະໜາດ" in tr and "ທຽບ" not in tr:
            # avoid "size relative to screen"
            if not re.search(r"\bsize\b", src, re.I) or re.search(
                r"dimension", src, re.I
            ):
                tr = tr.replace("ຂະໜາດ", "ມິຕິ")

    if re.search(r"\blayers?\b", src, re.I):
        tr = apply_list(tr, LAYER_PHRASES)

    if re.search(r"\bundo\b", src, re.I):
        # Verb/menu Undo = ເລີກທຳ; reserve ຍົກເລີກ for Cancel/dialog dismiss.
        tr = tr.replace("ຍ້ອນກັບ", "ເລີກທຳ")
        tr = tr.replace("ເລີກເຮັດ", "ເລີກທຳ")
        if re.fullmatch(r"&?Undo", src):
            tr = "&ເລີກທຳ" if src.startswith("&") else "ເລີກທຳ"
        elif tr.strip() in ("ກັບຄືນ", "ຍົກເລີກ"):
            tr = "ເລີກທຳ"
        # Common undo messages that still used Cancel term
        if "Nothing to undo" in src or "nothing to undo" in src.lower():
            tr = tr.replace("ຍົກເລີກ", "ເລີກທຳ")
        if re.search(r"cannot undo|to undo|undone", src, re.I):
            # Keep Cancel wording only when source says cancel; undo capability → ເລີກທຳ
            if "cancel" not in src.lower():
                tr = tr.replace("ຍົກເລີກ", "ເລີກທຳ")

    if re.search(r"\bredo\b", src, re.I):
        tr = tr.replace("ເຮັດຊ້ຳ", "ເຮັດຄືນ")

    # Delete / remove: glossary spelling ລຶບ (not ລົບ). Keep ລົບ for minus/negative/antialias.
    if re.search(r"delet|remov|eras", src, re.I):
        tr = tr.replace("ລົບ", "ລຶບ")

    # Tangential / tangental (CAD) → ສຳຜັດ  (never ສຳພັນ "relative")
    if re.search(r"tangent|tangental", src, re.I):
        tr = tr.replace("ເສັ້ນໂຄ້ງສຳພັນ", "ເສັ້ນໂຄ້ງສຳຜັດ")
        tr = tr.replace("ວົງມົນສຳພັນ", "ວົງມົນສຳຜັດ")
        tr = tr.replace("ສຳພັດ", "ສຳຜັດ")  # common misspelling of tangent
        if re.fullmatch(r"[Tt]angential|[Tt]angental", src):
            tr = "ສຳຜັດ"
        elif "ສຳພັນ" in tr and "ສຳຜັດ" not in tr:
            tr = tr.replace("ສຳພັນ", "ສຳຜັດ")
        return re.sub(r"  +", " ", tr)

    if re.search(r"\brelative\b", src, re.I):
        # only fix relative-typo forms; do not touch ສຳຜັດ
        tr = tr.replace("ສໍາພັດ", "ສຳພັນ")
        if tr.strip() in ("ສຳພັດ", "ຄວາມສຳພັນ"):
            tr = "ສຳພັນ"

    if re.fullmatch(r"Center \(large\)", src):
        tr = "ເສັ້ນໃຈກາງ (ໃຫຍ່)"
    elif re.fullmatch(r"Center \(small\)", src):
        tr = "ເສັ້ນໃຈກາງ (ນ້ອຍ)"
    elif re.fullmatch(r"[Cc]enterline", src):
        tr = "ເສັ້ນໃຈກາງ"
    elif re.fullmatch(r"[Cc]enter", src):
        tr = "ໃຈກາງ"
    elif re.search(r"centerline|center line", src, re.I):
        tr = tr.replace("ເສັ້ນແກນກາງ", "ເສັ້ນໃຈກາງ")

    if re.search(r"\bsnap\b", src, re.I):
        tr = tr.replace("ການດຶງເຂົ້າຫາ", "ເກາະຈັບ")
        tr = tr.replace("ດຶງເຂົ້າຫາ", "ເກາະຈັບ")
        tr = tr.replace("ສະແນັບ", "ເກາະຈັບ")
        if re.search(r"adjacent.*dim|dim.*adjacent", src, re.I):
            tr = tr.replace("ຂະໜາດ", "ມິຕິ")

    if re.search(r"\bentit", src, re.I):
        tr = tr.replace("ອອບເຈັກ", "ອົງປະກອບ")
        tr = tr.replace("ເອນຕິຕີ", "ອົງປະກອບ")
        tr = tr.replace("ວັດຖຸ", "ອົງປະກອບ")

    if re.search(r"\bspline", src, re.I):
        tr = tr.replace("ສະພລາຍ", "ສະປຼາຍ").replace("ສະປາຍ", "ສະປຼາຍ")
        tr = re.sub(r"\bSpline\b", "ເສັ້ນໂຄ້ງສະປຼາຍ", tr)
        tr = re.sub(r"\(spline\)", "", tr)
        tr = re.sub(r"  +", " ", tr).strip()

    # collapse spaces again
    tr = re.sub(r"  +", " ", tr)
    return tr


MESSAGE_RE = re.compile(r"(<message(?:\s[^>]*)?>)(.*?)(</message>)", re.S)
SOURCE_RE = re.compile(r"<source>(.*?)</source>", re.S)
TRANS_RE = re.compile(
    r"(<translation)(\s+type=\"[^\"]*\")?(\s*>)(.*?)(</translation>)"
    r"|(<translation)(\s+type=\"[^\"]*\")?(\s*/>)",
    re.S,
)


def process_message(body: str) -> tuple[str, bool]:
    sm = SOURCE_RE.search(body)
    if not sm:
        return body, False
    source = sm.group(1)
    changed = False

    def repl_trans(m: re.Match) -> str:
        nonlocal changed
        if m.group(6):
            return m.group(0)
        open_tag, typ, mid, content, close = (
            m.group(1),
            m.group(2) or "",
            m.group(3),
            m.group(4),
            m.group(5),
        )
        if 'type="vanished"' in typ or 'type="obsolete"' in typ:
            return m.group(0)
        new_content = transform_translation(source, content)
        if new_content != content:
            changed = True
        return f"{open_tag}{typ}{mid}{new_content}{close}"

    new_body, _ = TRANS_RE.subn(repl_trans, body)
    return new_body, changed


def process_file(path: Path) -> int:
    text = path.read_text(encoding="utf-8")
    text2 = re.sub(
        r'<TS version="2\.1" language="lo(?:_LA)?">',
        '<TS version="2.1" language="lo_LA">',
        text,
        count=1,
    )
    changes = 0 if text2 == text else 1
    text = text2

    out_parts: list[str] = []
    pos = 0
    for m in MESSAGE_RE.finditer(text):
        out_parts.append(text[pos : m.start()])
        open_m, body, close_m = m.group(1), m.group(2), m.group(3)
        new_body, ch = process_message(body)
        if ch:
            changes += 1
        out_parts.append(open_m + new_body + close_m)
        pos = m.end()
    out_parts.append(text[pos:])
    path.write_text("".join(out_parts), encoding="utf-8")
    return changes


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__)
        return 2
    total = 0
    for p in argv[1:]:
        path = Path(p)
        n = process_file(path)
        total += n
        print(f"{path}: {n} message(s)/header updated")
    print(f"total change events: {total}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
