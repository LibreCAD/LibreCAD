import xml.etree.ElementTree as ET
import asyncio
import aiohttp
from pathlib import Path
from typing import List, Dict

# ────────────────────────────────────────────────
#  CONFIGURATION
# ────────────────────────────────────────────────
FOLDER_PATH = "translations"
LIBRE_URL = "http://localhost:5000/translate"
BATCH_SIZE = 50       # LibreTranslate handles lists efficiently
CONCURRENT_TASKS = 4  
API_KEY = ""          # Leave empty if running locally without keys

# Global Counter
total_translated = 0

async def translate_batch(session: aiohttp.ClientSession, batch: List[Dict], target_lang: str):
    """Sends a batch of strings to the LibreTranslate API."""
    global total_translated
    
    # Extract just the text for the API request
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
                # LibreTranslate returns an array of strings when given an array
                translated_texts = result_data.get("translatedText", [])
                
                # If it's a single string response (some versions), wrap it
                if isinstance(translated_texts, str):
                    translated_texts = [translated_texts]

                # Map results back to XML
                for i, text in enumerate(translated_texts):
                    if i < len(batch):
                        batch[i]['el'].text = text.strip()
                        if "type" in batch[i]['el'].attrib:
                            del batch[i]['el'].attrib["type"]
                
                count = len(translated_texts)
                total_translated += count
                print(f"  [Progress] +{count} strings (Total: {total_translated})")
            else:
                print(f"  [Error] API returned status {resp.status}: {await resp.text()}")
                    
    except Exception as e:
        print(f"  [Error] Batch failed: {e}")

async def process_file(session: aiohttp.ClientSession, path: Path):
    tree = ET.parse(path)
    root = tree.getroot()
    
    # Language detection (e.g., librecad_fr.ts -> fr)
    lang = path.stem.split('_')[-1].lower() if '_' in path.stem else root.get("language", "en")
    
    to_translate = []
    for ctx in root.findall("context"):
        ctx_name = ctx.findtext("name", "Unknown")
        for msg in ctx.findall("message"):
            src_text = msg.findtext("source", "").strip()
            trans_el = msg.find("translation")
            
            if trans_el is not None and (trans_el.get("type") == "unfinished" or not (trans_el.text or "").strip()):
                if src_text:
                    to_translate.append({"src": src_text, "el": trans_el})

    if not to_translate:
        return

    print(f"\n→ File: {path.name} ({len(to_translate)} strings)")

    chunks = [to_translate[i:i + BATCH_SIZE] for i in range(0, len(to_translate), BATCH_SIZE)]
    semaphore = asyncio.Semaphore(CONCURRENT_TASKS)

    async def sem_task(chunk):
        async with semaphore:
            await translate_batch(session, chunk, lang)

    await asyncio.gather(*(sem_task(chunk) for chunk in chunks))

    # Save XML
    tree.write(path, encoding="utf-8", xml_declaration=True)

async def main():
    files = sorted(list(Path(FOLDER_PATH).glob("*.ts")))
    if not files:
        print(f"No .ts files found in {FOLDER_PATH}")
        return

    print(f"Starting LibreTranslate Batch Processor...")
    print(f"Endpoint: {LIBRE_URL} | Batch Size: {BATCH_SIZE}")
    print("-" * 50)

    async with aiohttp.ClientSession() as session:
        for file in files:
            await process_file(session, file)

    print("-" * 50)
    print(f"FINISHED! Total strings translated: {total_translated}")

if __name__ == "__main__":
    asyncio.run(main())
