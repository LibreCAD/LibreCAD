#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Qt .ts file batch translator using a local Ollama model.

Translates entries marked `<translation type="unfinished">` (and empty
plurals inside `<message numerus="yes">`) in every .ts file under one
or more directories. The target language is derived from the filename
suffix (librecad_zh_cn.ts -> zh_cn, plugins_es_mx.ts -> es_mx).

Defaults to the two tracked translation roots in the LibreCAD repo:
  librecad/ts/   plugins/ts/

English source locales (librecad_en.ts, plugins_en_au.ts, ...) are
skipped unless --include-english is passed.

Runtime: the script auto-runs under ~/.venv/ (Python venv that has the
`ollama` package installed). If invoked with a different interpreter
that lacks `ollama`, it re-execs itself via $HOME/.venv/bin/python.
Override the venv path with the LC_TS_VENV env var, or pass --no-venv
to disable the re-exec entirely.

Setup once:
  python3 -m venv ~/.venv && ~/.venv/bin/pip install ollama

Usage:
  scripts/ts_ollama.py [paths...] [options]
  scripts/ts_ollama.py --dry-run
  scripts/ts_ollama.py librecad/ts --locales de,fr --limit 50
"""

import os
import sys
from pathlib import Path


def _reexec_under_venv() -> None:
    """Switch interpreter to ~/.venv/bin/python when needed.

    Invoked before any heavy imports. We re-exec only when:
      - the user did not pass --no-venv,
      - the current interpreter is not already the venv python,
      - the venv python exists and is executable.
    """
    if "--no-venv" in sys.argv:
        sys.argv.remove("--no-venv")
        return

    venv_path = Path(os.environ.get("LC_TS_VENV", str(Path.home() / ".venv")))
    venv_python = venv_path / "bin" / "python"
    if not venv_python.exists():
        return  # let the script run; will fail loudly later if `ollama` missing

    try:
        if Path(sys.executable).resolve() == venv_python.resolve():
            return  # already inside the venv
    except OSError:
        pass

    os.execv(str(venv_python), [str(venv_python), str(Path(__file__).resolve()), *sys.argv[1:]])


_reexec_under_venv()

import argparse
import re
import shutil
import subprocess
import xml.etree.ElementTree as ET
from typing import Dict, List, Optional, Tuple

DEFAULT_PATHS = ["librecad/ts", "plugins/ts"]
DEFAULT_MODEL = "translategemma:27b"
FALLBACK_LANG = "en"

# Cache key includes target_lang so the same English source can map to
# different translations across locales.
TranslationKey = Tuple[str, str, str, str]  # (source, context, comment, lang)
translation_cache: Dict[TranslationKey, str] = {}

stats = {"translated": 0, "files_modified": 0, "files_skipped": 0}


def repo_root() -> Path:
    try:
        out = subprocess.check_output(
            ["git", "rev-parse", "--show-toplevel"],
            stderr=subprocess.DEVNULL, text=True,
        ).strip()
        return Path(out)
    except Exception:
        return Path.cwd()


# Filename suffix examples handled:
#   librecad_de.ts          -> de
#   librecad_zh_cn.ts       -> zh_cn
#   plugins_pt_br.ts        -> pt_br
#   librecad_en_au.ts       -> en_au
_LANG_RE = re.compile(r"_([a-z]{2})(?:_([a-z]{2}))?$", re.IGNORECASE)


def language_from_filename(path: Path) -> Optional[str]:
    """Return the locale in Qt-conventional form (xx or xx_YY)."""
    m = _LANG_RE.search(path.stem)
    if not m:
        return None
    lang = m.group(1).lower()
    region = m.group(2)
    return f"{lang}_{region.upper()}" if region else lang


def is_english(lang: str) -> bool:
    low = lang.lower()
    return low == "en" or low.startswith("en_")


def update_ts_language_attribute(root: ET.Element, desired: str) -> bool:
    if root.get("language") == desired:
        return False
    print(f"  Updating <TS language>: {root.get('language') or '(missing)'} -> {desired}")
    root.set("language", desired)
    return True


def pending_targets(msg: ET.Element) -> List[ET.Element]:
    """
    Return the list of elements that still need text. Empty list means
    the message is already done. For plain messages this is the
    <translation> itself; for numerus messages it's the empty
    <numerusform> children.
    """
    trans = msg.find("translation")
    if trans is None:
        return []

    if msg.get("numerus") == "yes":
        forms = trans.findall("numerusform")
        if not forms:
            return []
        empty = [f for f in forms if not (f.text or "").strip()]
        if trans.get("type") == "unfinished" or empty:
            return empty if empty else []
        return []

    typ = trans.get("type")
    text = (trans.text or "").strip()
    if typ == "unfinished":
        return [trans]
    if text == "":
        return [trans]
    return []


def get_translation(
    source: str, context: str, comment: Optional[str], target_lang: str,
    model: str,
) -> Optional[str]:
    key: TranslationKey = (source, context, comment or "", target_lang)
    if key in translation_cache:
        return translation_cache[key]

    prompt = f"""You are an expert CAD / LibreCAD / Qt application translator.
Translate the following English string into **{target_lang}**.

Rules:
- Use standard, commonly accepted CAD terminology in {target_lang}
- Menu items, buttons and actions should be short and natural (2-6 words max)
- Preserve all %1 %2 %n placeholders, & shortcuts, HTML tags, quotes
- Shortcut style: &File -> &Fichier (adapt to target language)
- Return ONLY the translation - no quotes, no explanations, no extra text

Context: {context}
{ f"Developer note: {comment}" if comment else "" }

English:
{source}

Translation:
"""

    try:
        import ollama  # imported lazily so --dry-run doesn't require it
        resp = ollama.chat(
            model=model,
            messages=[{"role": "user", "content": prompt}],
            options={"temperature": 0.15},
        )
        result = resp["message"]["content"].strip()
        result = result.removeprefix("Translation:").strip()
        result = result.strip('"“”「」')
    except Exception as e:
        print(f"  Ollama failed: {e}")
        return None

    translation_cache[key] = result
    return result


def atomic_save(tree: ET.ElementTree, path: Path) -> bool:
    tmp = path.with_suffix(path.suffix + ".tmp")
    tree.write(tmp, encoding="utf-8", xml_declaration=True)
    try:
        ET.parse(tmp)  # validate
    except ET.ParseError as e:
        print(f"  XML validation FAILED for {path}: {e}")
        try: tmp.unlink()
        except FileNotFoundError: pass
        return False
    os.replace(tmp, path)
    return True


def process_file(path: Path, lang_override: Optional[str], args) -> None:
    print(f"\n-> {path}")

    try:
        tree = ET.parse(path)
    except ET.ParseError as e:
        print(f"  XML parse error: {e}; skipping")
        stats["files_skipped"] += 1
        return

    root = tree.getroot()
    file_lang = language_from_filename(path)
    final_lang = lang_override or file_lang or root.get("language") or FALLBACK_LANG

    if is_english(final_lang) and not args.include_english:
        print(f"  skipping English source locale ({final_lang})")
        stats["files_skipped"] += 1
        return

    if args.locales:
        low = final_lang.lower()
        if low not in args.locales and low.split("_")[0] not in args.locales:
            print(f"  skipping locale {final_lang} (not in --locales)")
            stats["files_skipped"] += 1
            return

    print(f"  Language: {final_lang}  (file:{file_lang or '-'} xml:{root.get('language') or '-'})")
    modified = update_ts_language_attribute(root, final_lang)

    # Count pending up front for dry-run reporting
    pending_messages: List[Tuple[ET.Element, ET.Element, List[ET.Element]]] = []
    for ctx in root.findall("context"):
        for msg in ctx.findall("message"):
            src_el = msg.find("source")
            if src_el is None or not (src_el.text or "").strip():
                continue
            targets = pending_targets(msg)
            if targets:
                pending_messages.append((ctx, msg, targets))

    pending_count = sum(len(t) for _, _, t in pending_messages)
    print(f"  Pending: {len(pending_messages)} messages, {pending_count} targets")

    if args.dry_run or pending_count == 0:
        if modified and not args.dry_run:
            backup_and_save(tree, path, args)
        return

    if not args.no_backup:
        bak = path.with_suffix(path.suffix + ".bak")
        if not bak.exists():
            shutil.copy2(path, bak)

    count_in_file = 0
    last_ctx: Optional[ET.Element] = None
    saved_dirty = False

    for ctx, msg, targets in pending_messages:
        if args.limit and count_in_file >= args.limit:
            print(f"  --limit {args.limit} reached; stopping for this file")
            break

        # Save when we cross a context boundary
        if last_ctx is not None and ctx is not last_ctx and saved_dirty:
            if atomic_save(tree, path):
                saved_dirty = False
            else:
                restore_backup(path, args)
                return
        last_ctx = ctx

        ctx_name_el = ctx.find("name")
        ctx_name = ctx_name_el.text if ctx_name_el is not None else "Unknown"
        src_text = (msg.find("source").text or "").strip()
        comment_el = msg.find("comment")
        comment = comment_el.text if comment_el is not None else None

        translated = get_translation(
            src_text, ctx_name, comment, final_lang, args.model
        )
        if not translated:
            continue

        for target in targets:
            target.text = translated

        trans = msg.find("translation")
        if trans is not None and trans.get("type") == "unfinished":
            # Only drop the unfinished marker if every form/text is now filled
            if msg.get("numerus") == "yes":
                forms = trans.findall("numerusform")
                if forms and all((f.text or "").strip() for f in forms):
                    del trans.attrib["type"]
            else:
                if (trans.text or "").strip():
                    del trans.attrib["type"]

        modified = True
        saved_dirty = True
        count_in_file += 1
        stats["translated"] += 1

        if stats["translated"] % args.progress == 0:
            print(f"  [progress] {stats['translated']} total translated, "
                  f"cache size {len(translation_cache)}")

    if modified:
        if atomic_save(tree, path):
            stats["files_modified"] += 1
            print(f"  saved - {count_in_file} new translations")
        else:
            restore_backup(path, args)


def backup_and_save(tree: ET.ElementTree, path: Path, args) -> None:
    if not args.no_backup:
        bak = path.with_suffix(path.suffix + ".bak")
        if not bak.exists():
            shutil.copy2(path, bak)
    if atomic_save(tree, path):
        stats["files_modified"] += 1


def restore_backup(path: Path, args) -> None:
    if args.no_backup:
        print(f"  WARNING: validation failed and --no-backup set; {path} may be corrupt")
        return
    bak = path.with_suffix(path.suffix + ".bak")
    if bak.exists():
        shutil.copy2(bak, path)
        print(f"  restored from {bak}")


def collect_files(paths: List[Path]) -> List[Path]:
    files: List[Path] = []
    for p in paths:
        if p.is_dir():
            files.extend(sorted(p.glob("**/*.ts")))
        elif p.is_file() and p.suffix == ".ts":
            files.append(p)
        else:
            print(f"warning: skipping {p} (not a .ts file or directory)")
    # Stable de-dup
    seen, unique = set(), []
    for f in files:
        rp = f.resolve()
        if rp not in seen:
            seen.add(rp)
            unique.append(f)
    return unique


def parse_args(argv=None):
    p = argparse.ArgumentParser(
        description="Translate unfinished entries in Qt .ts files using Ollama.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument("paths", nargs="*", help="Directories or .ts files (default: tracked TS roots)")
    p.add_argument("--model", default=DEFAULT_MODEL, help=f"Ollama model (default: {DEFAULT_MODEL})")
    p.add_argument("--include-english", action="store_true", help="Translate English locales too")
    p.add_argument("--locales", default="", help="Comma-separated allow-list, e.g. de,fr,zh_cn")
    p.add_argument("--limit", type=int, default=0, help="Max translations per file (0 = no limit)")
    p.add_argument("--dry-run", action="store_true", help="Report counts without calling LLM or writing")
    p.add_argument("--no-backup", action="store_true", help="Skip writing <file>.bak before saving")
    p.add_argument("--progress", type=int, default=100, help="Print progress every N translations")
    p.add_argument("--lang", default=None,
                   help="Override target language (use only when running on a single file)")
    p.add_argument("--no-venv", action="store_true",
                   help="Do not re-exec under ~/.venv (handled before argparse runs)")
    args = p.parse_args(argv)
    args.locales = {s.strip().lower() for s in args.locales.split(",") if s.strip()}
    return args


def main(argv=None) -> int:
    args = parse_args(argv)

    root = repo_root()
    raw_paths = args.paths or DEFAULT_PATHS
    resolved: List[Path] = []
    for s in raw_paths:
        candidate = Path(s)
        if not candidate.is_absolute():
            candidate = (root / s).resolve() if (root / s).exists() else candidate.resolve()
        if not candidate.exists():
            print(f"error: not found: {candidate}")
            return 1
        resolved.append(candidate)

    if args.lang and len(resolved) != 1 or (args.lang and resolved[0].is_dir()):
        if args.lang and (len(resolved) != 1 or resolved[0].is_dir()):
            print("error: --lang requires exactly one .ts file argument")
            return 1

    files = collect_files(resolved)
    if not files:
        print("no .ts files found")
        return 1

    print(f"found {len(files)} .ts files")
    if args.dry_run:
        print("[dry-run mode - no Ollama calls, no writes]")

    for i, f in enumerate(files, 1):
        print(f"\n[{i}/{len(files)}]", end=" ")
        process_file(f, args.lang, args)

    print("\n" + "=" * 65)
    print(f"  files processed     : {len(files)}")
    print(f"  files modified      : {stats['files_modified']}")
    print(f"  files skipped       : {stats['files_skipped']}")
    print(f"  strings translated  : {stats['translated']}")
    print(f"  cache size          : {len(translation_cache)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
