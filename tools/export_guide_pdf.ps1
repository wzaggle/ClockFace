param([string]$GuidePath = "", [string]$OutputDirectory = "")
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
if (-not $GuidePath) { $GuidePath = Join-Path $root "docs/OBS-Clock-User-Guide-v1.0.4.docx" }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $root "docs" }
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
# Requires LibreOffice on PATH. Word's Export to PDF is an alternative.
& soffice --headless --convert-to pdf --outdir $OutputDirectory $GuidePath
if ($LASTEXITCODE -ne 0) { throw "PDF conversion failed" }
