#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import xml.etree.ElementTree as ET
import ollama
import re
from pathlib import Path
import sys
from typing import Dict, Tuple, Optional

# ==================== CONFIG ====================
FOLDER_PATH = "translations"          # ← change to your folder
OLLAMA_MODEL = "qwen2.5:14b"          # or "qwen3.5:122b", etc.
FALLBACK_LANG = "en"
PROGRESS_INTERVAL = 100               # print progress every N translated strings

# Translation memory (for consistency during one run)
translation_memory: Dict[Tuple[str, str, Optional[str]], str] = {}

# Global counters
total_translated = 0
total_files = 0
updated_files = 0

# ================================================

def extract_language_from_filename(filepath: Path) -> str | None:
    stem = filepath.stem
    m = re.search(r'_([a-z]{2})(?:_([a-z]{2}))?$', stem, re.IGNORECASE)
    if not m:
        return None
    lang = m.group(1).lower()
    country = m.group(2)
    if country:
        return f"{lang}_{country.lower()}"
    return lang


def set_ts_language_attribute(root: ET.Element, desired_lang: str) -> bool:
    current = root.get("language")
    if current == desired_lang:
        return False
    print(f"  Updating language: {current or '(missing)'} → {desired_lang}")
    root.set("language", desired_lang)
    return True


def translate_text(
    text: str,
    context_name: str,
    dev_comment: Optional[str],
    target_lang: str
) -> Optional[str]:
    if not text or text.strip() == "":
        return ""

    key = (text, context_name, dev_comment or "")
    if key in translation_memory:
        return translation_memory[key]

    prompt = f"""你是一位专业的 CAD / LibreCAD / Qt 应用本地化工程师。
请将以下英文字符串翻译成 **{target_lang}** 语言，要求：

- 使用该语言最自然的表达 + CAD 领域常用术语
- 菜单/按钮/动作名尽量简洁 (2–6 个字符/词)
- 保留 %1 %2 %n 占位符、&快捷键、HTML、引号等
- 快捷键形式 &File → &文件 (根据目标语言调整)
- 只返回翻译结果，不要加任何说明或引号

上下文: {context_name}
{ f"注释: {dev_comment}" if dev_comment else "" }

原文:
{text}

翻译：
"""

    try:
        resp = ollama.chat(
            model=OLLAMA_MODEL,
            messages=[{"role": "user", "content": prompt}],
            options={"temperature": 0.12}
        )
        result = resp['message']['content'].strip()
        result = result.replace('翻译：', '').strip().strip('"“”「」')

        translation_memory[key] = result
        print(f"translation: {key} -> {result}")
        return result
    except Exception as e:
        print(f"  Ollama error: {e}")
        return None


def print_progress():
    global total_translated
    print(f"  [Progress] {total_translated} strings translated so far")


def process_one_ts_file(filepath: Path) -> bool:
    global total_translated, updated_files

    print(f"\n→ {filepath}")

    try:
        tree = ET.parse(filepath)
        root = tree.getroot()

        lang_from_file = extract_language_from_filename(filepath)
        lang_in_xml = root.get("language")

        final_lang = lang_from_file or lang_in_xml or FALLBACK_LANG

        modified = set_ts_language_attribute(root, final_lang)

        print(f"  Language: {final_lang} "
              f"(file: {lang_from_file or '-'}, xml: {lang_in_xml or '-'} )")

        count_this_file = 0

        for context in root.findall("context"):
            ctx_name_elem = context.find("name")
            ctx_name = ctx_name_elem.text if ctx_name_elem is not None else "Unknown"

            for msg in context.findall("message"):
                source = msg.find("source")
                if not source or not source.text:
                    continue

                trans = msg.find("translation")
                if not trans:
                    print("translation not found")
                    continue

                if trans.get("type") == "unfinished" or not trans.text or not trans.text.strip():
                    comment_elem = msg.find("comment")
                    comment = comment_elem.text if comment_elem is not None else None

                    translated = translate_text(
                        source.text,
                        ctx_name,
                        comment,
                        final_lang
                    )

                    if translated:
                        trans.text = translated
                        if "type" in trans.attrib:
                            del trans.attrib["type"]
                        modified = True
                        count_this_file += 1
                        total_translated += 1

                        # Progress reporting
                        if total_translated % PROGRESS_INTERVAL == 0:
                            print_progress()

        if modified:
            tree.write(
                filepath,
                encoding="utf-8",
                xml_declaration=True,
                method="xml"
            )
            updated_files += 1
            print(f"  Saved – {count_this_file} new translations in this file")
            return True
        else:
            print("  No changes")
            return False

    except ET.ParseError as e:
        print(f"  XML parse error: {e}")
        return False
    except Exception as e:
        print(f"  Failed: {e}")
        return False


def main():
    global total_files

    folder = Path(FOLDER_PATH).resolve()
    if not folder.is_dir():
        print(f"Error: not a directory → {folder}")
        sys.exit(1)

    ts_files = sorted(folder.glob("**/*.ts"))
    total_files = len(ts_files)

    if total_files == 0:
        print("No .ts files found.")
        return

    print(f"Found {total_files} .ts files")
    print(f"Will report progress every {PROGRESS_INTERVAL} translated strings\n")

    for i, path in enumerate(ts_files, 1):
        print(f"[{i}/{total_files}]")
        process_one_ts_file(path)

    print("\n" + "═"*70)
    print(f"Finished!")
    print(f"• Processed files     : {total_files}")
    print(f"• Files with changes  : {updated_files}")
    print(f"• Total strings translated this run : {total_translated}")
    print(f"• Unique translation cache entries  : {len(translation_memory)}")
    print_progress()  # final report


if __name__ == "__main__":
    main()
