#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Qt .ts file batch translator using Ollama
Translates unfinished / empty entries in all .ts files in a folder
looking for ts files in a folder "translations/"
The ts file name is assumed to contain a locale code, such as librecad_zh.ts or librecad_zh_cn.ts
"""

import xml.etree.ElementTree as ET
import ollama
import re
from pathlib import Path
import sys
from typing import Dict, Tuple, Optional

# ────────────────────────────────────────────────
#  CONFIGURATION
# ────────────────────────────────────────────────

FOLDER_PATH     = "translations"          # ← change to your folder
#OLLAMA_MODEL    = "qwen3.5:122b"           # ← your model (qwen2.5:32b, llama3.1:70b, etc.)
#OLLAMA_MODEL    = "qwen3.5:latest"           # ← your model (qwen2.5:32b, llama3.1:70b, etc.)
OLLAMA_MODEL    = "qwen3.5:35b-a3b"           # ← your model (sweet spot for Apple Silicon m2max
FALLBACK_LANG   = "en"
PROGRESS_EVERY  = 100                     # print progress every N translations

# Translation memory: (source, context, comment) → translation
translation_cache: Dict[Tuple[str, str, Optional[str]], str] = {}

# Global counters
total_translated = 0
updated_files    = 0

# ────────────────────────────────────────────────
#  HELPERS
# ────────────────────────────────────────────────

def get_language_from_filename(path: Path) -> Optional[str]:
    """Extract language code like 'fr', 'zh_cn', 'es_hn' from filename"""
    stem = path.stem
    m = re.search(r'_([a-z]{2})(?:_([a-z]{2}))?$', stem, re.IGNORECASE)
    if not m:
        return None
    lang = m.group(1).lower()
    country = m.group(2)
    return f"{lang}_{country.lower()}" if country else lang


def update_ts_language_attribute(root: ET.Element, desired: str) -> bool:
    current = root.get("language")
    if current == desired:
        return False
    print(f"  Updating <TS language> : {current or '(missing)'} → {desired}")
    root.set("language", desired)
    return True


def should_translate(trans_elem: ET.Element) -> bool:
    """Decide whether this entry needs translation"""
    if trans_elem is None:
        return False

    typ = trans_elem.get("type")
    text = (trans_elem.text or "").strip()

    # Main rules – adjust thresholds / patterns as needed
    if typ == "unfinished":
        return True                         # ← force unfinished (even if partial text exists)

    if text == "":
        return True

    # Optional: also catch very suspicious short / placeholder translations
    if len(text) <= 4 and text in {"", "TODO", "???", "...", "[ ]"}:
        return True

    return False


def get_translation(
    source: str,
    context: str,
    comment: Optional[str],
    target_lang: str
) -> Optional[str]:
    key = (source, context, comment or "")
    if key in translation_cache:
        return translation_cache[key]

    prompt = f"""You are an expert CAD / LibreCAD / Qt application translator.
Translate the following English string into **{target_lang}**.

Rules:
- Use standard, commonly accepted CAD terminology in {target_lang}
- Menu items, buttons and actions should be short and natural (2–6 words max)
- Preserve all %1 %2 %n placeholders, & shortcuts, HTML tags, quotes
- Shortcut style: &File → &Fichier (adapt to target language)
- Return ONLY the translation – no quotes, no explanations, no extra text

Context: {context}
{ f"Developer note: {comment}" if comment else "" }

English:
{source}

Translation:
"""

    try:
        response = ollama.chat(
            model=OLLAMA_MODEL,
            messages=[{"role": "user", "content": prompt}],
            options={"temperature": 0.15}
        )
        result = response['message']['content'].strip()

        # Clean common model artifacts
        result = result.removeprefix("Translation:").removeprefix("翻譯：").strip()
        result = result.strip('"“”「」')

        translation_cache[key] = result
        print(f"translation: {len(translation_cache)}: {key} -> {result}")
        return result

    except Exception as e:
        print(f"  Ollama failed: {e}")
        return None


# ────────────────────────────────────────────────
#  MAIN PROCESSING FUNCTION
# ────────────────────────────────────────────────

def process_ts_file(path: Path) -> bool:
    global total_translated, updated_files

    print(f"\n→ {path}")

    try:
        tree = ET.parse(path)
        root = tree.getroot()

        lang_file = get_language_from_filename(path)
        lang_xml  = root.get("language")
        final_lang = lang_file or lang_xml or FALLBACK_LANG

        modified = update_ts_language_attribute(root, final_lang)

        print(f"  Language: {final_lang}  (file: {lang_file or '-'}, xml: {lang_xml or '-'} )")

        count_this_file = 0

        for ctx in root.findall("context"):
            ctx_name_el = ctx.find("name")
            ctx_name = ctx_name_el.text if ctx_name_el is not None else "Unknown"

            for msg in ctx.findall("message"):
                src_el = msg.find("source")
                if src_el is None or not (src_el.text or "").strip():
                    continue

                src_text = (src_el.text or "").strip()

                trans_el = msg.find("translation")
                if trans_el is None:
                    continue

                if should_translate(trans_el):
                    comment_el = msg.find("comment")
                    comment = comment_el.text if comment_el is not None else None

                    translated = get_translation(
                        src_text,
                        ctx_name,
                        comment,
                        final_lang
                    )

                    if translated:
                        trans_el.text = translated
                        if "type" in trans_el.attrib:
                            del trans_el.attrib["type"]
                        modified = True
                        count_this_file += 1
                        total_translated += 1

                        if total_translated % PROGRESS_EVERY == 0:
                            print(f"  [Progress] {total_translated} strings translated")

        if modified:
            tree.write(path, encoding="utf-8", xml_declaration=True)
            updated_files += 1
            print(f"  Saved — {count_this_file} strings updated in this file")
            return True

        print("  No changes needed")
        return False

    except ET.ParseError as e:
        print(f"  XML parse error: {e}")
        return False
    except Exception as e:
        print(f"  Failed: {e}")
        return False


def main():
    folder = Path(FOLDER_PATH).resolve()
    if not folder.is_dir():
        print(f"Error: not a directory → {folder}")
        sys.exit(1)

    files = sorted(folder.glob("**/*.ts"))
    if not files:
        print("No .ts files found.")
        return

    print(f"Found {len(files)} .ts files\n")

    for i, p in enumerate(files, 1):
        print(f"[{i}/{len(files)}]")
        process_ts_file(p)

    print("\n" + "═"*65)
    print("Finished!")
    print(f"  Files processed : {len(files)}")
    print(f"  Files modified  : {updated_files}")
    print(f"  Strings translated this run : {total_translated}")
    print(f"  Translation cache size      : {len(translation_cache)}")


if __name__ == "__main__":
    main()
