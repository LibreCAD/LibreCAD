/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 * ********************************************************************************
 */

# Test fixture provenance

The committed files listed below are authored or generated for LibreCAD's
libdxfrw tests. They are distributed under LibreCAD's GPL-2.0-or-later
license. No newly added DWG or DXF file in this PR is an external corpus copy.

## Hand-authored DXF

These files contain only small ASCII records written for the named regression:

| File | SHA-256 |
| --- | --- |
| `dxf/block_record_preview_r2007.dxf` | `ac47c4691a04be79f65f9a0f94f029556df4b2afb072abb42828619e6c66d75e` |
| `dxf/classes_raw_entity_r2007.dxf` | `5f510094f3427d4ebf1aeffee930c24aa9fd56caed3f4582846fedb4d5f0783f` |
| `dxf/eed_binary_r2007.dxf` | `ff32b882f51e8fd46ca040a86ed2755f7eeab40553e43205c3e1765f5d71f434` |
| `dxf/raw_control_groups_r2007.dxf` | `228f3748e568f137aebe76307b969e365339510bd4e3520abbeab0f3c8b1a682` |


## Hand-authored DXF with a CJK codepage

Unlike the ASCII fixtures above, these three carry non-ASCII bytes on purpose:
each declares a `$DWGCODEPAGE` and stores its layer name and TEXT string in
that codepage, so a reader has to select the right converter and decode them.
They were written for this repository - no drawing data is imported from
anywhere - and cover the two tables that no corpus file reaches:

| File | Codepage | Contents |
| --- | --- | --- |
| `dxf/big5_traditional.dxf` | ANSI_950 | layer 圖層, text 中文測試 |
| `dxf/big5_hkscs.dxf` | ANSI_950 | layer 龘䒑, text Ê̄䒑𠀡 — an hkscs sequence standing for two code points, an hkscs row, and a supplementary code point |
| `dxf/uhc_korean.dxf` | ANSI_949 | layer 도면, text 한글도면 |

| File | SHA-256 |
| --- | --- |
| `dxf/big5_traditional.dxf` | `d44c3da8a87f100f63e044b72a28cab077932ae4e5771c5f8617e5cd4b243442` |
| `dxf/big5_hkscs.dxf` | `694de552f02b928dcb3fefc595d3e38f0510f4312d40e6b4df7a83913ca4c437` |
| `dxf/uhc_korean.dxf` | `9c5d6f81e5331f01e29400f9a5e2df80d1921a116612d4a5852eda9889c0cc18` |

## Locally generated DXF LTYPE table fragments

`dxf/ltype_table_ac1009.dxf` and `dxf/ltype_table_ac1015_ac1021.dxf` hold the
LTYPE table (`0 TABLE` through `0 ENDTAB`, LF line endings) that RS_FilterDXFRW
wrote for a new, empty drawing at commit 86d2c9325 in the R12 DXF format and in
the R2000 and R2007 DXF formats; the R2000 and R2007 tables are byte-identical,
so one fragment serves both. They are table fragments, not drawings, contain no
imported data, and were produced by the export that the test "A new drawing
writes the built-in LTYPE table as before" in `dxf_roundtrip_tests.cpp`
repeats; it compares the current writer's table against them with `\r`
stripped on both sides.

| File | SHA-256 |
| --- | --- |
| `dxf/ltype_table_ac1009.dxf` | `e2006e5de4fdf66290b246579504f1471c5f18f5ed8f00564633a2ab44c7a58f` |
| `dxf/ltype_table_ac1015_ac1021.dxf` | `12e8e1c1c84ca803d1dc34fdf0c77c4181ab9fc01e779d0402dd050e9b40f353` |

## Generated DXF (hatch moments)

`dxf/hatch_moments_spline_lens_10_5.dxf` was written for this repository with
ezdxf 1.4.4 (`ezdxf.new("R2010")`): one SOLID HATCH whose edge path is a
clamped degree-2 SPLINE with the control points (9, 5), (9.5, 6.2),
(10.5, 6.2) and (11, 5), knots 0, 0, 0, 0.5, 1, 1, 1, closed by a LINE from
(11, 5) back to (9, 5). It contains no imported drawing data.

| File | SHA-256 |
| --- | --- |
| `dxf/hatch_moments_half_ellipse_10_5.dxf` | `e3a10523a3dedd8eee0ee3297fbf1424b3e6f0544c4087c632f1008a8d61999b` |
| `dxf/hatch_moments_half_ellipse_rot30_10_5.dxf` | `929bfc5e1f5539c4470bf2e73a0126e2346327a1ebc6dc78a7af05c92d332fed` |
| `dxf/hatch_moments_spline_lens_10_5.dxf` | `0237038997a1d12aca93a441e06a12985976b7dd0615bba9715e8976145f6345` |

## Generated DXF (hatch density)

Sixty synthetic DXF scenes used by the hidden hatch regeneration-determinism
regression test. Each is a small polyline boundary with one or two pattern
hatches; they contain no imported drawing data. Produced by `common.py` in
the hatch-fixes research tree, using ezdxf.

| File | SHA-256 |
| --- | --- |
| `hatch/density/L_400_wall40__ansi31__0.25.dxf` | `d36aa351e6696f167b7b23705b3a800f6f74b14dfc30f4fbe5bb8e1c79717c22` |
| `hatch/density/L_400_wall40__ansi31__1.dxf` | `68298d661546d35fabcebe68b30694429740738aad495adefe388aa22c1e6269` |
| `hatch/density/L_400_wall40__ar-conc__0.25.dxf` | `bda2d7d875999fe797957c8e6e462caa659122b02e2a3842ba38e3dfbfd9c91f` |
| `hatch/density/L_400_wall40__ar-conc__1.dxf` | `3c12403c776076e9b71a524a7451864e815ce7884cec5682c797dcd53f993212` |
| `hatch/density/L_400_wall40__brick__0.25.dxf` | `ebe9243f307504fcf009b422e36fe7a74b549f5b215a687833a562f13bf0247f` |
| `hatch/density/L_400_wall40__brick__1.dxf` | `8402301f298f7fe37f1f48bb30c2ff7adbffb57f9b61815121c71c8068b0250e` |
| `hatch/density/L_400_wall40__hex__0.25.dxf` | `d53fb4e2ec95e2a9057257e53e051218eb4166681d1069f1b2e0d31992eb87ff` |
| `hatch/density/L_400_wall40__hex__1.dxf` | `23b9ea57a4e61c760c2cf2a782cb838b5f8ab1b4d6078d5aa0dc42dd99ae2cbf` |
| `hatch/density/L_400_wall40__paisley__0.25.dxf` | `c9da0f4b0f71146522407f8474dba9a11a5fb2749a765fbb5ce57ac712e0f147` |
| `hatch/density/L_400_wall40__paisley__1.dxf` | `5a2553dc10e7b194d4b051fae3b96da9b58e9d59f60a4326bc64667a0b7bfa27` |
| `hatch/density/L_400_wall40__sand__0.25.dxf` | `5a5c619e8834a084637b41a8e20b6fd92ab849af60ad8bd5048d3989f665e16c` |
| `hatch/density/L_400_wall40__sand__1.dxf` | `878ec3c0d13e7865e5ff3c82154aa8c68bf86d7214e15eec36405834085a06c5` |
| `hatch/density/annulus_sq__ansi31__0.25.dxf` | `55d0c6ddba32868c81eb281d449f74e88ea0a556ce83124936d89e6810d0506e` |
| `hatch/density/annulus_sq__ansi31__1.dxf` | `1fc39e3b18a94f63b5c9399034756766524cb3231a6737d4cc213c9f0ab92bf5` |
| `hatch/density/annulus_sq__ar-conc__0.25.dxf` | `398b0bede5d2d4d4aea86f1b64eb0ffa222b94413fa2a30e56ae7977213f3654` |
| `hatch/density/annulus_sq__ar-conc__1.dxf` | `81eb85fb942cde7bd44dff0f1b5ea832bda4656e245f0cb795b0befbcf9bb715` |
| `hatch/density/annulus_sq__brick__0.25.dxf` | `51405ef3b2de0a9278f32d6089dcd684666794e98d162de00e68de78502a38b7` |
| `hatch/density/annulus_sq__brick__1.dxf` | `b48b4692ddaec319d572233a9eaa53e28ce57532642207d65974969ba0f320c7` |
| `hatch/density/annulus_sq__hex__0.25.dxf` | `b47c06fb2d703ff31c49742b7e5d0cf880b383b7f672a29b13288c21f9340c43` |
| `hatch/density/annulus_sq__hex__1.dxf` | `06ddf1b2da3ba72b76512c6222fc5ed44abe23e7896e8d9ee6c2249c3142e752` |
| `hatch/density/annulus_sq__paisley__0.25.dxf` | `1b97c73303a2274b47d1fa7497a05b28608a506715b069126ce9e4e7188583eb` |
| `hatch/density/annulus_sq__paisley__1.dxf` | `69f8bdc7a61a44ca975a929c03ebb9a307ce8758063c7d5c07387a9038f0f753` |
| `hatch/density/annulus_sq__sand__0.25.dxf` | `3487bbd5317b16e63499dd2d1376c51eec13ccd7c510ac273eae61843bfb7e23` |
| `hatch/density/annulus_sq__sand__1.dxf` | `76f08767dc53f32791d307e0995f500c61ab2248a303535a41ba0b3576c930b2` |
| `hatch/density/circle_r200__ansi31__0.25.dxf` | `290ba21c818fb081d3b7f3e555c67b3936a3d2f3956203bb518f51e854309cae` |
| `hatch/density/circle_r200__ansi31__1.dxf` | `2878bda2e21a1798343c0694bb873a1a388835e8f62b17a5aff4f0b8d78249d8` |
| `hatch/density/circle_r200__ar-conc__0.25.dxf` | `a59c07511f0106f0192c80829156568a64dd0a7d7c1b6fe786cd09891bc1d64f` |
| `hatch/density/circle_r200__ar-conc__1.dxf` | `8f38f99f49c7efc2999399c3dd485bb7ea5f2f65930a8090427980264c4c871f` |
| `hatch/density/circle_r200__brick__0.25.dxf` | `1969a3be39204792156dafa4fb640faca077b28934d2109fecd50bf26b3ad476` |
| `hatch/density/circle_r200__brick__1.dxf` | `656706661594417d5e108f204a767133c2fa61011ad9766211964533815ae4d5` |
| `hatch/density/circle_r200__hex__0.25.dxf` | `71ddd73e6d1f9073b053df80fe639cdeb1c9735d26682d7ee6ce4bf2f670d6e7` |
| `hatch/density/circle_r200__hex__1.dxf` | `774f0b8600979213ff925e65d0fafe2ad30822aacab5eb061bfda6e80b3604fe` |
| `hatch/density/circle_r200__paisley__0.25.dxf` | `49a9fdfe555fb6de6dc47150de7014c17f5caaf9309cfcc8a7878bbfbbf0d0fd` |
| `hatch/density/circle_r200__paisley__1.dxf` | `216f8b9ea8ff3498f187fcd4894c237244a8856b56a74735c5aa9300c88683f0` |
| `hatch/density/circle_r200__sand__0.25.dxf` | `329e0330386b0706abb072f2f66dd9f6ee82199fdf6c08663dd8255794f4f338` |
| `hatch/density/circle_r200__sand__1.dxf` | `c6e78b6ab26e50627b9b516259cdea26aeb71cbf667257444b8ec411137e8727` |
| `hatch/density/comb_10teeth__ansi31__0.25.dxf` | `e595c10be0157f2d2c818edfe51651ab8d45b7f43c4655dc3671708c2373db8e` |
| `hatch/density/comb_10teeth__ansi31__1.dxf` | `e88a77ca5b58d5198d71bd5ea8c13a11c0a728057af7ccd5c91b755f527cdb17` |
| `hatch/density/comb_10teeth__ar-conc__0.25.dxf` | `66fad0181176b4eb92814585d8aa884fdc707ae82ab93444dc31bccc35c97487` |
| `hatch/density/comb_10teeth__ar-conc__1.dxf` | `557dda78cbc61bc428114780a7c818cbb3b654442ccd84d1cfdd413446f17f0a` |
| `hatch/density/comb_10teeth__brick__0.25.dxf` | `ef177b08e35cdf04897ee1e6acf6f454e314231ac7d84f145566ebb53c131db7` |
| `hatch/density/comb_10teeth__brick__1.dxf` | `db207ee4598a5b2bf54db3f272a6900095e38a3f2d74526ee9159c2d6708b054` |
| `hatch/density/comb_10teeth__hex__0.25.dxf` | `1ef3501942288b16678c3ecbac97ff586d2766b7802b648092a2d87cd2e08bfd` |
| `hatch/density/comb_10teeth__hex__1.dxf` | `aeb6757f8ff113bab21b4fd7654559c88946f39705750c3e813d8189fa1c29c2` |
| `hatch/density/comb_10teeth__paisley__0.25.dxf` | `1ff9a9bb78b71f7122c56ace475e4657c6cb20f30fcaef064d5cf2c0901ce13a` |
| `hatch/density/comb_10teeth__paisley__1.dxf` | `8b015884fb69cdd34e406f583117a6aea0774f0555ff057ffdb68726b738070a` |
| `hatch/density/comb_10teeth__sand__0.25.dxf` | `e7779687033590eb404b517213496a70b6771a360a898a2705e0064971519fc2` |
| `hatch/density/comb_10teeth__sand__1.dxf` | `6f5c9cbbb711f90d574a43a95007be47412fd6bc7b2b4f30ee3afdf746be9e9a` |
| `hatch/density/rect500x300__ansi31__0.25.dxf` | `87dfde8afafed7a0c7767531f5467dadf05bcf325760c7ba22f67556521f15cd` |
| `hatch/density/rect500x300__ansi31__1.dxf` | `768667539190aa604e9981c18688eb6515e28fee2a868df3ffe79d893974867a` |
| `hatch/density/rect500x300__ar-conc__0.25.dxf` | `e0aebe9a57889f78999b7245320619767ba017c0078f9a10074eb60957b1e511` |
| `hatch/density/rect500x300__ar-conc__1.dxf` | `15407a2b3c25d4eef8f65fdda31378cf8b5b94076e0c73307aa49201c21d6c2a` |
| `hatch/density/rect500x300__brick__0.25.dxf` | `74b3a0031a3e0461f69a477cfa512efec3314edb4eaa79df7a5364139426cc9b` |
| `hatch/density/rect500x300__brick__1.dxf` | `8b4e3ecba4c4d934e715cd665d6b151f6afa9a30087608e95d42f8eb29359944` |
| `hatch/density/rect500x300__hex__0.25.dxf` | `0527ffd5993e8b57ebdebcd522cb499fc935075cf18135a5c0522b760511bf3e` |
| `hatch/density/rect500x300__hex__1.dxf` | `44c271a646207e3d386705906bdb37eaa827552922ab16c1525337a32b98b087` |
| `hatch/density/rect500x300__paisley__0.25.dxf` | `e62588a74d7d8a0c33b7636490d08261551366a25d45997b8633b45911939260` |
| `hatch/density/rect500x300__paisley__1.dxf` | `48f2cf66ce5a3df1b032b87db0b690543b95f54fd3f9c180866ea27dda163b5d` |
| `hatch/density/rect500x300__sand__0.25.dxf` | `7f27f8997f1d22a7c3eceba722acda907f6e6d96c29157fdd9195dbbd53e72d0` |
| `hatch/density/rect500x300__sand__1.dxf` | `c92a6bec915d31792314fbd033aa460f3a336217c69442873b8de60dc74d4404` |


## Hatch angle units fixtures

Fixtures that the hatch-angle-units regression tests mutate at run time to
exercise every writer-header class. They contain no imported drawing data.

- `hatch/lc_dxfrw063_ansi31_45.dxf` is a byte copy of a real LibreCAD 2.x export
  (dxfrw 0.6.3): one ANSI31 HATCH over a 100 x 60 polyline loop with
  `52 = 0.7853981633974483` (pi/4 stored raw as legacy radians). The hatch
  angle test rewrites the first `999` comment and the value after `52` to make
  the file match each row of its header-detection table.
- `hatch/oda_hatch_deg_def_ac1032.dwg` is a byte copy of an ezdxf HATCH written
  through ODA File Converter at AC1032 (R2018), with `52 = 45` and `53 = 90`.
  It guards that the DWG path keeps the stored angle in radians irrespective of
  the file's writer header.

| File | SHA-256 |
| --- | --- |
| `hatch/lc_dxfrw063_ansi31_45.dxf` | `8f55be0dad04a6f2670642b724fe082b95802c672d167b6db108ece40ab6593d` |
| `hatch/oda_hatch_deg_def_ac1032.dwg` | `5e5e168b393c5ac2f94add40fc30f1cf206b5c70851249caaebf17e98813569d` |

## Locally generated DWG

These files were generated by the libdxfrw `dwgRW` writer from the test
interfaces in `dwg_write_smoke_tests.cpp`. They do not contain imported drawing
data:

| File | SHA-256 |
| --- | --- |
| `ordinary_enc_AC1015.dwg` | `80690dc45b2e32b4d378df4be860c51c6b3c952400aa762eb73a654e93a728dd` |
| `ordinary_enc_AC1018.dwg` | `981b982b7e6414e5dd6cd6b33a88c8e385d14f298b125faba4c94e80dec15f96` |
| `ordinary_enc_AC1021.dwg` | `00b4f1e9757236304c2f7c17b56c12c7ee217f8bc8cb9fdc9b97cda39ac3d136` |
| `ordinary_enc_AC1027.dwg` | `a0ebf245e570bf0dc337c696330b7ea883feaebb7c4e781de8d77ac723815f83` |
| `ordinary_enc_ac1027_ansi932.dwg` | `449a386ccc677dcb93848f583d812afcfd0194631973406394e782233c463aba` |

The AC1021 fixture was regenerated after correcting the R2007 control-handle
stream boundaries, file-header protection, and section-name termination. Its source remains the
`dwgRW preserves ordinary entity ENC side channels by version` test interface.

The larger LibreDWG `TS1.dwg` and modeler examples remain external-only. Set
`LIBRECAD_EXTERNAL_DWG_FIXTURE_DIR` to a LibreDWG test-data root to run the
opt-in tests that consume them; they are intentionally absent from this
repository.

## Multi-line attribute fixtures

`lc_multiline_attrib_ac1032.dwg` is an ODA File Converter 27.1.0 output
(DXF to DWG, `ACAD2018`) of a DXF authored for this repository with ezdxf
1.4.4: block `LC_ML_BLOCK` with a line, a multi-line attribute definition
`ML_TAG` (`embed_mtext`, lock position 1) and a preset single-line definition
`SL_TAG`, and one reference to the block at (100, 50) carrying both
attributes. It contains no imported drawing data.

| File | SHA-256 |
| --- | --- |
| `lc_multiline_attrib_ac1032.dwg` | `f06218b4975c1ed2a9de0484a57744a4e5b5377ffe4520d6fb9098b8bd0dac11` |

`dwg_entity_encode_round_trip_tests.cpp` additionally holds, as byte arrays,
two object records of a drawing AutoCAD itself saved: the multi-line ATTRIB
(handle 0x79D, 254 bytes) and its ATTDEF (handle 0x796, 212 bytes) of
`samples/sample_AC1032.dwg` from ACadSharp
(https://github.com/DomCR/ACadSharp, MIT License, Copyright (c) 2021 Albert
Domenech). They are the reference the R2018 embedded-MTEXT layout is checked
against bit for bit; the drawing itself is not redistributed, and the test
that imports it skips when it is absent.

The same file holds one object record of the R14 drawing of that collection,
`samples/sample_AC1014.dwg`: its PDF underlay (handle 0xD07, 56 bytes), whose
last handle names the PDFDEFINITION 0xD06. It is the reference for the
definition handle of an underlay in a DWG older than R2000.

`dwg_safety_tests.cpp` holds, as byte arrays, one more object
record of drawings AutoCAD saved: the TEXT "XData Layer" (handle 0x91B) of
ACadSharp's `samples/sample_AC1014.dwg` (AutoCAD R14, 81 bytes) and
`samples/sample_AC1015.dwg` (AutoCAD 2000, 73 bytes), same project and
license as above. Their extended data names a layer, and is the reference
the byte order of such an item is checked against. The drawings are not
redistributed.

## Baseline corpus cleanup

The baseline `origin/master` testdata inventory contained 27 DWGs. This change
removes 24 files that were byte-identical to LibreDWG or ACadSharp samples, or
whose provenance could only be established from developer-local documents.
The tests that exercise those files remain in the suite and skip explicitly
when the optional files are absent. No external baseline DWG is redistributed
by this repository after the cleanup.

The three retained baseline DWGs are generated ODA File Converter outputs from
small, project-authored DXF inputs documented by the original test commits.
They are kept as local regression artifacts rather than external corpus copies:

| File | SHA-256 |
| --- | --- |
| `large_radial.dwg` | `d3f7c22db4b6c3953e2d716da719d7e7e5b285cf73bd4e317f0a171d3859f98a` |
| `mpolygon_solid.dwg` | `66a171c55bc90fdd0764ad3556ec786503666a30a4473a637d168a01979ffc29` |
| `rtext_arctext.dwg` | `db5663cdc5c1759fdc1f68f9122e596307de06317e766cd5e0aab55912e5d511` |

The bundled application data under `librecad/support/patterns`,
`librecad/support/library` and `librecad/res/dxf` is retained; its origin is
recorded here because an earlier revision of this document described it as
unresolved:

| Data | Origin |
| --- | --- |
| 47 hatch patterns, 8 library parts | Present since the initial CADuntu import (`53feefc72`, 2010-08-16); moved to these paths by the directory restructure `839912da8` (R. van Twisk, 2012-02-15). |
| 15 `gost_*` hatch patterns, 1267 library parts | The LLGE library, offered to LibreCAD under the GNU GPL v2 by its author in issue #506 and imported by `3417326b5` (2015-04-04). Upstream: https://savannah.gnu.org/projects/llge/ |
| `millstone.dxf` | Contributed by @Lamy78 in PR #964, merged as `0559d3098` (2018-04-04). |
| `angle45.dxf` | Added by `f55a33e72` (2025-06-17). |
| `res/dxf/dim_sample.dxf` | Authored for the dimension-style preview in PR #2286, commit `64fa97a12` (2025-09-22). |

Packaging code still tolerates the absence of the optional directories, and
test cases remain in place and skip only when an optional external fixture is
unavailable.
