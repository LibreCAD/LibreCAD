# SBOM Skill

## Purpose

Use this guidance when changing Software Bill of Materials (SBOM) generation,
validation, enrichment, or publication in this repository.

## Tooling standard

- Use Microsoft SBOM Tool to generate and validate SPDX 2.2 documents.
- Use CycloneDX CLI only to convert the validated SPDX document to CycloneDX 1.6.
- Generate both formats from the staged `windows/` build, not from source alone.
- Treat validation failure or a missing output document as a release-blocking error.
- Do not commit generated content under `SBOM/reports/`.

## Repository workflow

1. Build LibreCAD so `windows/` contains the staged application and runtime files.
2. Run `SBOM/sbom-generation-ms-tool.ps1` with PowerShell 7 or later.
3. The script installs `Microsoft.SBOMTool` and `CycloneDX.CLI` with winget when
   necessary, generates SPDX 2.2, merges additional aspects, validates the aggregate,
   and converts it to CycloneDX 1.6.
4. Jenkins archives the two JSON documents and copies them beside the installer.

The Jenkins `VERSION_FULL` value supplies the SBOM version. For local runs the script
uses `git describe` with the qmake `LC_VERSION` value as a final fallback.

## Additional aspects and CPE enrichment

Repository-maintained dependency manifests belong at:

`SBOM/AdditionalAspects/<dependency>/_manifest/spdx_2.2/manifest.spdx.json`

Each aspect must be a valid SPDX 2.2 document. Use stable SPDX identifiers and exact
versions. CPE entries use SPDX package `externalRefs` with:

- `referenceCategory`: `SECURITY`
- `referenceType`: `cpe23Type`
- `referenceLocator`: a valid CPE 2.3 value

The generator remaps aspect SPDX identifiers, links each aspect root to the LibreCAD
root package with `DEPENDS_ON`, and copies CPE values to matching CycloneDX components.
A CPE-bearing aspect is expected to match exactly one converted component by package
name and version; otherwise generation fails rather than publishing ambiguous data.

## Typical commands

```powershell
pwsh -File .\SBOM\sbom-generation-ms-tool.ps1
pwsh -File .\SBOM\sbom-generation-ms-tool.ps1 -BuildPath .\windows -ProductVersion 2.2.1.1234
```

## References

- [Microsoft SBOM Tool](https://github.com/microsoft/sbom-tool)
- [Microsoft SBOM Tool arguments](https://github.com/microsoft/sbom-tool/blob/main/docs/sbom-tool-arguments.md)
- [CycloneDX 1.6 JSON](https://cyclonedx.org/docs/1.6/json/)
- [SPDX 2.2 specification](https://spdx.github.io/spdx-spec/v2.2/)
- [NIST Common Platform Enumeration](https://www.nist.gov/itl/products-and-services/common-platform-enumeration-cpe)
