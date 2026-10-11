#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 LibreCAD contributors
"""Source inventory for the CAD Familiar migration; not runtime coverage."""
import argparse
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ACTION_FILE = 'librecad/src/ui/main/init/lc_actionfactory.cpp'
DOCK_FILE = 'librecad/src/ui/main/init/lc_widgetfactory.cpp'
COMMAND_FILE = 'librecad/src/cmd/lc_commandItems.h'


def inventory():
    actions, shortcuts, docks, commands = [], [], [], []
    function = ''
    for line_no, line in enumerate((ROOT / ACTION_FILE).read_text().splitlines(), 1):
        match = re.search(r'LC_ActionFactory::(\w+)\(', line)
        if match:
            function = match[1]
        match = re.match(r'\s*\{"([^"]+)",\s*(.+)', line)
        if not match:
            continue
        entry = {'id': match[1], 'definition': match[2].strip(), 'line': line_no}
        if function == 'setDefaultShortcuts':
            shortcuts.append(entry)
        elif function.startswith('create') and 'Actions' in function:
            entry['factory'] = function
            actions.append(entry)
    text = (ROOT / DOCK_FILE).read_text()
    for match in re.finditer(r'createDockWidget\(tr\("([^"]+)"\),\s*"([^"]+)"', text):
        docks.append({'id': match[2], 'title': match[1], 'line': text[:match.start()].count('\n') + 1})
    # Strip comments before parsing the static command table; translation
    # strings are intentionally excluded from the English-name list.
    text = (ROOT / COMMAND_FILE).read_text()
    text = re.sub(r'/\*.*?\*/', lambda m: '\n' * m[0].count('\n'), text, flags=re.S)
    text = re.sub(r'//[^\n]*', '', text)
    text = text[text.index('const LC_CommandItem g_commandList[]'):]
    for match in re.finditer(r'\{\s*(.*?)\s*(RS2::Action\w+)\s*\}', text, re.S):
        names = list(dict.fromkeys(re.findall(r'\{"([^"]+)",\s*LC_CommandText', match[1])))
        if names:
            commands.append({'action_type': match[2], 'english_names_and_aliases': names})
    return {
        'source_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
        'scope': 'Static factory entries and English command spellings; no runtime/parity claim. Conditional entries are retained. Dynamically generated actions and CAD category docks are not enumerated.',
        'sources': {'actions_and_shortcuts': ACTION_FILE, 'document_docks': DOCK_FILE, 'commands': COMMAND_FILE},
        'counts': {'action_entries': len(actions), 'default_shortcut_entries': len(shortcuts), 'document_docks': len(docks), 'command_entries': len(commands)},
        'actions': actions, 'default_shortcuts': shortcuts, 'document_docks': docks, 'commands': commands,
    }


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = inventory()
    if not result['actions'] or not result['commands'] or not result['document_docks']:
        raise SystemExit('Inventory unexpectedly empty; update the parser for source changes.')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result['counts']))
