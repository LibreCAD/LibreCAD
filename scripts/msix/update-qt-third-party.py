#!/usr/bin/env python3
# ********************************************************************************
# This file is part of the LibreCAD project, a 2D CAD program
#
# Copyright (C) 2026 LibreCAD.org
# Copyright (C) 2026 Dongxu Li (github.com/dxli)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
# USA.
# ********************************************************************************

"""Writes licenses/qt-third-party.txt from Qt's own attribution data.

The Windows packages ship Qt's DLLs and plugins, which contain third-party
code (PCRE2, HarfBuzz, libpng, libjpeg-turbo, FreeType, libtiff, libwebp,
...) whose licences ask for their notices to go with every binary copy.  Qt
records each component in a qt_attribution.json beside its sources; this
script reads those of the modules and plugins the packages ship, for one Qt
release, and writes each component's copyright and full licence text.

Run it again with the Qt version build-all.yml installs whenever that
changes; on the 2.2.1 branch that is Qt 5.15.2:

    python3 scripts/msix/update-qt-third-party.py --tag v5.15.2

The list of attribution files depends on the major version of the tag.
"""

import argparse
import hashlib
import json
import pathlib
import posixpath
import re
import sys
import urllib.error
import urllib.request

# qt_attribution.json files of code built into the Qt DLLs and plugins that
# windeployqt deploys for LibreCAD on Windows (Core, Gui, Widgets, Network,
# PrintSupport, Svg, the windows platform plugin, the image format and icon
# engine plugins).  Unix-, macOS-, Android-, SQL-, D-Bus- and
# test-only components are left out.
ATTRIBUTIONS_QT6 = {
    'qtbase': [
        'src/3rdparty/D3D12MemoryAllocator/qt_attribution.json',
        'src/3rdparty/VulkanMemoryAllocator/qt_attribution.json',
        'src/3rdparty/blake2/qt_attribution.json',
        'src/3rdparty/double-conversion/qt_attribution.json',
        'src/3rdparty/easing/qt_attribution.json',
        'src/3rdparty/emoji-segmenter/qt_attribution.json',
        'src/3rdparty/freetype/qt_attribution.json',
        'src/3rdparty/harfbuzz-ng/qt_attribution.json',
        'src/3rdparty/icc/qt_attribution.json',
        'src/3rdparty/libjpeg/qt_attribution.json',
        'src/3rdparty/libpng/qt_attribution.json',
        'src/3rdparty/libpsl/qt_attribution.json',
        'src/3rdparty/md4/qt_attribution.json',
        'src/3rdparty/md4c/qt_attribution.json',
        'src/3rdparty/md5/qt_attribution.json',
        'src/3rdparty/pcre2/qt_attribution.json',
        'src/3rdparty/pixman/qt_attribution.json',
        'src/3rdparty/rfc6234/qt_attribution.json',
        'src/3rdparty/sha1/qt_attribution.json',
        'src/3rdparty/sha3/qt_attribution.json',
        'src/3rdparty/siphash/qt_attribution.json',
        'src/3rdparty/tinycbor/qt_attribution.json',
        'src/3rdparty/wintab/qt_attribution.json',
        'src/3rdparty/zlib/qt_attribution.json',
        'src/corelib/kernel/qt_attribution.json',
        'src/corelib/mimetypes/3rdparty/qt_attribution.json',
        'src/corelib/text/qt_attribution.json',
        'src/gui/opengl/qt_attribution.json',
        'src/gui/painting/qt_attribution.json',
        'src/gui/rhi/qt_attribution.json',
        'src/gui/text/qt_attribution.json',
        'src/gui/vulkan/qt_attribution.json',
    ],
    'qtimageformats': [
        'src/3rdparty/libtiff/qt_attribution.json',
        'src/3rdparty/libwebp/qt_attribution.json',
    ],
    'qtsvg': [
        'src/svg/qt_attribution.json',
    ],
}

# The same for Qt 5.15, which the 2.2.1 branch builds with: Core, Gui,
# Widgets, PrintSupport, Svg, ANGLE (libEGL.dll and libGLESv2.dll), the
# windows platform plugin, the Windows print support, the windowsvista
# style, the image format and icon engine plugins.  Left out besides the
# kinds above: pixman (ARM NEON only) and corelib/kernel (macOS only).
ATTRIBUTIONS_QT5 = {
    'qtbase': [
        'src/3rdparty/VulkanMemoryAllocator/qt_attribution.json',
        'src/3rdparty/angle/qt_attribution.json',
        'src/3rdparty/double-conversion/qt_attribution.json',
        'src/3rdparty/easing/qt_attribution.json',
        'src/3rdparty/freebsd/qt_attribution.json',
        'src/3rdparty/freetype/qt_attribution.json',
        'src/3rdparty/harfbuzz/qt_attribution.json',
        'src/3rdparty/harfbuzz-ng/qt_attribution.json',
        'src/3rdparty/iaccessible2/qt_attribution.json',
        'src/3rdparty/icc/qt_attribution.json',
        'src/3rdparty/libjpeg/qt_attribution.json',
        'src/3rdparty/libpng/qt_attribution.json',
        'src/3rdparty/md4/qt_attribution.json',
        'src/3rdparty/md4c/qt_attribution.json',
        'src/3rdparty/md5/qt_attribution.json',
        'src/3rdparty/pcre2/qt_attribution.json',
        'src/3rdparty/rfc6234/qt_attribution.json',
        'src/3rdparty/sha1/qt_attribution.json',
        'src/3rdparty/sha3/qt_attribution.json',
        'src/3rdparty/tinycbor/qt_attribution.json',
        'src/3rdparty/wintab/qt_attribution.json',
        'src/3rdparty/zlib/qt_attribution.json',
        'src/corelib/codecs/qt_attribution.json',
        'src/corelib/io/qt_attribution.json',
        'src/corelib/text/qt_attribution.json',
        'src/gui/opengl/qt_attribution.json',
        'src/gui/painting/qt_attribution.json',
        'src/gui/text/qt_attribution.json',
        'src/gui/vulkan/qt_attribution.json',
    ],
    'qtimageformats': [
        'src/3rdparty/libtiff/qt_attribution.json',
        'src/3rdparty/libwebp/qt_attribution.json',
    ],
    'qtsvg': [
        'src/svg/qt_attribution.json',
    ],
}

RULE = '=' * 78


def fetch(cache, repo, tag, path):
    target = cache / repo / tag / path
    if not target.exists():
        url = f'https://raw.githubusercontent.com/qt/{repo}/{tag}/{path}'
        with urllib.request.urlopen(url, timeout=60) as response:
            data = response.read()
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    return target.read_bytes().decode('utf-8', errors='replace')


def as_list(value):
    if value is None:
        return []
    return value if isinstance(value, list) else [value]


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--tag', required=True, help='Qt release tag, such as v5.15.2 or v6.9.3')
    parser.add_argument('--output', default=str(pathlib.Path(__file__).resolve().parents[2] / 'licenses' / 'qt-third-party.txt'))
    parser.add_argument('--cache', default=None, help='folder for the downloaded files (default: a temporary one)')
    args = parser.parse_args()

    if args.cache:
        cache = pathlib.Path(args.cache)
    else:
        import tempfile
        cache = pathlib.Path(tempfile.mkdtemp(prefix='qt-attribution-'))

    major = re.match(r'v?(\d+)\.', args.tag)
    if not major or major.group(1) not in ('5', '6'):
        sys.exit(f'--tag must name a Qt 5 or Qt 6 release, such as v5.15.2: {args.tag}')
    qt5 = major.group(1) == '5'
    attributions = ATTRIBUTIONS_QT5 if qt5 else ATTRIBUTIONS_QT6

    components = []
    for repo, paths in attributions.items():
        for path in paths:
            # Some of Qt's files hold raw tabs inside strings.
            entries = json.loads(fetch(cache, repo, args.tag, path), strict=False)
            for entry in as_list(entries):
                license_files = as_list(entry.get('LicenseFile')) + as_list(entry.get('LicenseFiles'))
                license_paths = [posixpath.normpath(posixpath.join(posixpath.dirname(path), name))
                                 for name in license_files]
                if not license_paths and qt5:
                    # Qt 5 has no LICENSES/ folder.  The components of the list
                    # above without a file of their own (MD4, MD5, SHA-1,
                    # ANGLE's MurmurHash) are public domain and have no licence
                    # text; any other stops here.
                    if entry.get('License') != 'Public Domain':
                        sys.exit(f'{repo}/{path}: {entry.get("Name")} has no licence file and is not public domain')
                elif not license_paths:
                    # Without a file of its own, the component's SPDX
                    # identifiers name texts in the repository's LICENSES/.
                    identifiers = [token for token in re.findall(r'[A-Za-z0-9.+-]+', entry.get('LicenseId', ''))
                                   if token not in ('AND', 'OR', 'WITH')]
                    license_paths = [f'LICENSES/{identifier}.txt' for identifier in identifiers]
                texts = []
                for license_path in license_paths:
                    try:
                        texts.append(fetch(cache, repo, args.tag, license_path).strip('\n'))
                    except urllib.error.HTTPError as error:
                        sys.exit(f'{repo}/{license_path} for {entry.get("Name")}: {error}')
                components.append({
                    'name': entry.get('Name', entry.get('Id', path)),
                    'version': entry.get('Version', ''),
                    'homepage': entry.get('Homepage', ''),
                    'licence': entry.get('License', ''),
                    'licence_id': entry.get('LicenseId', ''),
                    'copyright': [line for item in as_list(entry.get('Copyright')) for line in str(item).splitlines()],
                    'texts': texts,
                    'source': f'{repo}/{path}',
                })

    components.sort(key=lambda c: c['name'].lower())
    out = []
    out.append(f'Third-party code in the Qt libraries shipped with LibreCAD (Qt {args.tag.lstrip("v")})')
    out.append('')
    out.append('LibreCAD\'s Windows packages contain Qt\'s DLLs and plugins. Besides Qt\'s')
    out.append('own code, those DLLs and plugins contain the components below, each under')
    if qt5:
        out.append('its own licence. The Windows installers also carry opengl32sw.dll, Qt\'s')
        out.append('build of Mesa llvmpipe, which is listed at')
        out.append('https://doc.qt.io/qt-5/qt-attribution-llvmpipe.html instead.')
    else:
        out.append('its own licence. The x64 installer also carries opengl32sw.dll, Qt\'s build')
        out.append('of Mesa llvmpipe, which is listed at')
        out.append('https://doc.qt.io/qt-6/qt-attribution-llvmpipe.html instead.')
    out.append('This file is generated by scripts/msix/update-qt-third-party.py from the')
    out.append('qt_attribution.json files of the Qt sources.')
    out.append('')
    for component in components:
        out.append(RULE)
        title = component['name'] + (f' {component["version"]}' if component['version'] else '')
        out.append(title)
        if component['homepage']:
            out.append(component['homepage'])
        licence = component['licence']
        if component['licence_id']:
            licence = f'{licence} ({component["licence_id"]})' if licence else component['licence_id']
        out.append(f'Licence: {licence}')
        out.extend(component['copyright'])
        out.append(RULE)
        out.append('')
    seen = {}
    out.append('')
    out.append('LICENCE TEXTS')
    out.append('')
    for component in components:
        for text in component['texts']:
            digest = hashlib.sha256(text.encode('utf-8')).hexdigest()
            out.append(RULE)
            out.append(f'{component["name"]}')
            out.append(RULE)
            if digest in seen:
                out.append(f'Same text as for {seen[digest]} above.')
            else:
                seen[digest] = component['name']
                out.append(text)
            out.append('')

    output = pathlib.Path(args.output)
    output.write_text('\n'.join(out).rstrip('\n') + '\n', encoding='utf-8', newline='\n')
    print(f'Wrote {output}: {len(components)} components, {len(seen)} distinct licence texts')
    return 0


if __name__ == '__main__':
    sys.exit(main())
