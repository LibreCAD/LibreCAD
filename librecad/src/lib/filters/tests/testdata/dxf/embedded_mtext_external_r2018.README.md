# Independent embedded ATTRIB/ATTDEF DXF fixture

`embedded_mtext_external_r2018.dxf` was generated with **ezdxf 1.4.3**,
not LibreCAD/libdxfrw, using the adjacent generator's public ezdxf API.
Normal C++ tests consume these checked-in bytes without Python, ezdxf or network.

SHA256 identities of the original independently generated inputs:

- DXF: `97540b467577294533ee4ef971c153999b2b4dad54a682c5d0e6e22746566e38`
- Generator: `fcde99046e9a15e22b5d721f7632b82178bb84446ffb7408007ab3151f2d2570`

The generator uses `ezdxf.new("R2018")`, fixed metadata options, two explicit
STYLE records and registered APPIDs. `set_mtext(..., graphic_properties=False)`
sets a native body on one block ATTDEF and one INSERT ATTRIB; the original
standalone MTEXT is then deleted. Both outer entities have independent
`OUTER-FALLBACK` text and `OUTER_STYLE`, while the embedded body is
`embedded \\H1.5x;body\\Psecond` with `EMBED_STYLE`. `OuterScope` and the opaque
`LibreCADText` application groups belong to the **outer entity** after the
group-101 `Embedded Object` native body. Embedded MTEXT has no inline XDATA.

Reproduce/check in a separately provisioned environment pinned to ezdxf 1.4.3:

```sh
python generate_embedded_mtext_external_r2018.py generate candidate.dxf
python generate_embedded_mtext_external_r2018.py check candidate.dxf
```

Generation runs the same semantic checks and ezdxf audit. Byte identity depends
on the pinned generator/environment; do not refresh the fixture merely to make
a failing parser test pass. The C++ paired-writer case can also retain its output
through `LC_EMBEDDED_DXF_ORACLE_OUTPUT` for the independent `check` command.

References: [Autodesk embedded-object group 101 contract](https://help.autodesk.com/cloudhelp/2017/ENU/OARXMAC-RefGuide/files/OREFMAC-__MEMBERTYPE_Methods_AcDbDxfFiler.html),
[ezdxf extended-tag ownership](https://github.com/mozman/ezdxf/blob/v1.4.3/src/ezdxf/lldxf/extendedtags.py),
and [embedded attribute implementation](https://github.com/mozman/ezdxf/blob/v1.4.3/src/ezdxf/entities/attrib.py).
This fixture proves native DXF boundary/outer ownership only, not an independent
embedded metadata carrier, pre-2018 downgrade, DWG or schema-freeze acceptance.
