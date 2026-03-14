"""
PURPOSE:
    Automates the translation of Qt Linguist (.ts) files using a local or remote 
    LibreTranslate API instance. It specifically handles regional locale codes 
    (e.g., zh_TW, es_MX) by mapping them to API-compatible ISO codes and 
    processes only "unfinished" or empty translation tags in batches.

DEPENDENCIES:
    - python 3.7+
    - aiohttp: For asynchronous HTTP requests (pip install aiohttp)
    - LibreTranslate: A running instance (http://localhost:5000)
    - xml.etree.ElementTree: Standard library for XML parsing

USAGE:
    1. Ensure LibreTranslate is running locally: `libretranslate --update-models`
    2. Place this script in the directory scripts/ in the LibreCAD source
       folder
    3. The script targets files at: ../librecad/ts/*.ts
    4. Run the script: `python ts_libre.py`
"""

import xml.etree.ElementTree as ET
import asyncio
import aiohttp
from pathlib import Path
from typing import List, Dict

# ────────────────────────────────────────────────
#  CONFIGURATION & LANGUAGE MAPPING
# ────────────────────────────────────────────────
# Get the folder where THIS script is located
SCRIPT_DIR = Path(__file__).parent
# Resolve the relative path: ../librecad/ts/
TS_FOLDER = (SCRIPT_DIR / ".." / "librecad" / "ts").resolve()

LIBRE_URL = "http://localhost:5000/translate"
BATCH_SIZE = 50
CONCURRENT_TASKS = 4
API_KEY = ""

# Map specific filename codes to LibreTranslate-supported codes
SPECIAL_MAPPING = {
    "zh_tw": "zt",  # Traditional Chinese
    "zh_cn": "zh",  # Simplified Chinese
    "pt_br": "pt",  # Portuguese (Brazil)
}

def get_api_lang(path: Path) -> str:
    """
    Extracts the language code from filenames like 'librecad_zh_tw.ts'.
    Returns 'zt' for zh_tw, 'es' for es_mx, etc.
    """
    # Remove 'librecad_' prefix and '.ts' suffix
    name_core = path.stem.replace("librecad_", "").lower()
    
    # 1. Check for manual mapping
    if name_core in SPECIAL_MAPPING:
        return SPECIAL_MAPPING[name_core]
    
    # 2. Handle regional codes by taking the first part (es_mx -> es)
    return name_core.split('_')[0]

# Global Counter
total_translated = 0

async def translate_batch(session: aiohttp.ClientSession, batch: List[Dict], target_lang: str):
    global total_translated
    texts = [item['src'] for item in batch]
    
    payload = {
        "q": texts,
        "source": "en",
        "target": target_lang,
        "format": "text",
        "api_key": API_KEY
    }

    try:
        async with session.post(LIBRE_URL, json=payload, timeout=30) as resp:
            if resp.status == 200:
                result_data = await resp.json()
                translated_texts = result_data.get("translatedText", [])
                
                if isinstance(translated_texts, str):
                    translated_texts = [translated_texts]

                for i, text in enumerate(translated_texts):
                    if i < len(batch):
                        batch[i]['el'].text = text.strip()
                        if "type" in batch[i]['el'].attrib:
                            del batch[i]['el'].attrib["type"]
                
                count = len(translated_texts)
                total_translated += count
                print(f"  [Progress] +{count} (Total: {total_translated})")
            else:
                error_msg = await resp.text()
                print(f"  [Error] API {resp.status} for '{target_lang}': {error_msg}")
    except Exception as e:
        print(f"  [Error] Batch failed: {e}")

async def process_file(session: aiohttp.ClientSession, path: Path):
    try:
        tree = ET.parse(path)
    except ET.ParseError:
        print(f"  [Error] Could not parse XML: {path.name}")
        return

    root = tree.getroot()
    target_lang = get_api_lang(path)
    
    # Skip source language (English)
    if target_lang == "en":
        return

    to_translate = []
    for msg in root.findall(".//message"):
        src_text = msg.findtext("source", "").strip()
        trans_el = msg.find("translation")
        
        if trans_el is not None:
            is_unfinished = trans_el.get("type") == "unfinished"
            is_empty = not (trans_el.text or "").strip()
            
            if src_text and (is_unfinished or is_empty):
                to_translate.append({"src": src_text, "el": trans_el})

    if not to_translate:
        return

    print(f"\n→ File: {path.name} (API Code: {target_lang}) | {len(to_translate)} strings")

    chunks = [to_translate[i:i + BATCH_SIZE] for i in range(0, len(to_translate), BATCH_SIZE)]
    semaphore = asyncio.Semaphore(CONCURRENT_TASKS)

    async def sem_task(chunk):
        async with semaphore:
            await translate_batch(session, chunk, target_lang)

    await asyncio.gather(*(sem_task(chunk) for chunk in chunks))
    tree.write(path, encoding="utf-8", xml_declaration=True)

async def main():
    if not TS_FOLDER.exists():
        print(f"Error: Folder not found at {TS_FOLDER}")
        return

    files = sorted(list(TS_FOLDER.glob("*.ts")))
    if not files:
        print(f"No .ts files found in {TS_FOLDER}")
        return

    print(f"Scanning: {TS_FOLDER}")
    async with aiohttp.ClientSession() as session:
        for file in files:
            await process_file(session, file)

    print(f"\nFINISHED! Total strings translated: {total_translated}")

if __name__ == "__main__":
    asyncio.run(main())
