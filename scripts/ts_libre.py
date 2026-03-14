### translate ts files by local hosted libretranslate
### to start: 
###   pip install libretranslate
###   libretranslate --port 5000
###   python ts_libre.py 
### the python script will translate ts files in the subfolder translations/

import xml.etree.ElementTree as ET
import asyncio
import aiohttp
from pathlib import Path
from typing import List, Dict

# ────────────────────────────────────────────────
#  CONFIGURATION & LANGUAGE MAPPING
# ────────────────────────────────────────────────
FOLDER_PATH = "translations"
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

def get_api_lang(path: Path, root_lang: str) -> str:
    """Extracts and cleans the language code from the filename."""
    # Split by first underscore to get everything after 'librecad_'
    parts = path.stem.split('_', 1)
    if len(parts) < 2:
        return root_lang[:2] if root_lang else "en"
    
    raw_code = parts[1].lower()
    
    # Check manual mapping first, otherwise strip regional suffix (es_mx -> es)
    if raw_code in SPECIAL_MAPPING:
        return SPECIAL_MAPPING[raw_code]
    return raw_code.split('_')[0]

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
        async with session.post(LIBRE_URL, json=payload) as resp:
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
                print(f"  [Error] API {resp.status}: {await resp.text()}")
    except Exception as e:
        print(f"  [Error] Batch failed: {e}")

async def process_file(session: aiohttp.ClientSession, path: Path):
    tree = ET.parse(path)
    root = tree.getroot()
    
    # Determine the correct language code for the API
    target_lang = get_api_lang(path, root.get("language"))
    
    if target_lang == "en":
        return

    to_translate = []
    for msg in root.findall(".//message"):
        src_text = msg.findtext("source", "").strip()
        trans_el = msg.find("translation")
        
        # Collect if empty or marked 'unfinished'
        if trans_el is not None and (trans_el.get("type") == "unfinished" or not (trans_el.text or "").strip()):
            if src_text:
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
    files = sorted(list(Path(FOLDER_PATH).glob("*.ts")))
    if not files:
        print(f"No .ts files found in {FOLDER_PATH}")
        return

    async with aiohttp.ClientSession() as session:
        for file in files:
            await process_file(session, file)

    print(f"\nFINISHED! Total strings translated: {total_translated}")

if __name__ == "__main__":
    asyncio.run(main())
