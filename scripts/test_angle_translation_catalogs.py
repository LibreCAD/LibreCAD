#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 LibreCAD.org

"""Check the translated angle reports without building the application."""

from collections import Counter
from pathlib import Path
import re
import unittest
import xml.etree.ElementTree as ET


REPO_ROOT = Path(__file__).resolve().parents[1]
ANGLE_CONTEXTS = {
    "LC_ActionInfo3PointsAngle",
    "LC_ActionInteractivePickAngle",
    "RS_ActionInfoAngle",
}
DERIVED_LABELS = ("Complementary:", "Supplementary:", "Explementary angle:")


def numbered_arguments(text: str) -> Counter:
    return Counter(re.findall(r"%L?[0-9]+", text))


class AngleTranslationCatalogTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.catalogs = {}
        for path in sorted((REPO_ROOT / "librecad/ts").glob("librecad_*.ts")):
            messages = []
            for context in ET.parse(path).getroot().findall("context"):
                if context.findtext("name") not in ANGLE_CONTEXTS:
                    continue
                for message in context.findall("message"):
                    source = message.findtext("source") or ""
                    translation = message.find("translation")
                    if translation is not None and translation.get("type") in {
                        "vanished", "obsolete"
                    }:
                        continue
                    if (source.strip() in DERIVED_LABELS
                            or "Alternative:" in source
                            or "Explementary angle:" in source):
                        messages.append(message)
            cls.catalogs[path.name] = messages

    def test_angle_catalogs_include_all_derived_labels(self) -> None:
        self.assertTrue(any(self.catalogs.values()))
        for catalog, messages in self.catalogs.items():
            # Older Spanish regional catalogs inherit these messages from es.
            if not messages:
                continue
            with self.subTest(catalog=catalog):
                self.assertEqual(12, len(messages))
                for message in messages:
                    source = message.findtext("source")
                    self.assertNotIn("Alternative:", source)
                    if "\n" in source:
                        self.assertIn("Explementary angle:", source)
                    else:
                        self.assertIn(source.strip(), DERIVED_LABELS)

    def test_translations_preserve_numbered_arguments(self) -> None:
        for catalog, messages in self.catalogs.items():
            for message in messages:
                translation = message.find("translation")
                if translation is None:
                    continue
                source = message.findtext("source") or ""
                text = "".join(translation.itertext())
                if translation.get("type") == "unfinished" and not text.strip():
                    continue
                with self.subTest(catalog=catalog, source=source):
                    self.assertTrue(text.strip())
                    self.assertEqual(numbered_arguments(source), numbered_arguments(text))

    def test_cursor_and_report_labels_agree(self) -> None:
        for catalog, messages in self.catalogs.items():
            labels = {name: [] for name in DERIVED_LABELS}
            for message in messages:
                translation = message.find("translation")
                if translation is None:
                    continue
                text = "".join(translation.itertext())
                if translation.get("type") == "unfinished" and not text.strip():
                    continue
                if "\n" in (message.findtext("source") or ""):
                    lines = text.splitlines()
                    with self.subTest(catalog=catalog):
                        self.assertGreaterEqual(len(lines), 4)
                    if len(lines) < 4:
                        continue
                    for index, name in enumerate(DERIVED_LABELS, start=1):
                        labels[name].append(lines[index].replace(f"%{index + 1}", "").strip())
                else:
                    labels[message.findtext("source").strip()].append(text.strip())
            for name, values in labels.items():
                if values:
                    with self.subTest(catalog=catalog, angle=name):
                        self.assertEqual(1, len(set(values)), values)

    def test_derived_angle_names_are_distinct(self) -> None:
        for catalog, messages in self.catalogs.items():
            labels = {}
            for message in messages:
                source = (message.findtext("source") or "").strip()
                if source not in DERIVED_LABELS:
                    continue
                text = message.findtext("translation") or ""
                if text.strip():
                    labels[source] = text.strip()
            if labels:
                with self.subTest(catalog=catalog):
                    self.assertEqual(3, len(set(labels.values())), labels)


if __name__ == "__main__":
    unittest.main()
