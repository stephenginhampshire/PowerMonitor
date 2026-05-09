$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $repo

$mdPath = Join-Path $repo 'TECHNICAL_USER_GUIDE.md'
$outPath = Join-Path $repo 'TECHNICAL_USER_GUIDE.docx'
$tmp = Join-Path $repo '.docx_tmp'

if (-not (Test-Path $mdPath)) {
    throw "Missing source file: $mdPath"
}

Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path (Join-Path $tmp '_rels') -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $tmp 'word\\_rels') -Force | Out-Null

function Escape-Xml([string]$s) {
    if ($null -eq $s) { return '' }
    return $s.Replace('&','&amp;').Replace('<','&lt;').Replace('>','&gt;')
}

function Is-TableRow([string]$line) {
    return $line.TrimStart() -match '^\|.*\|'
}

function Is-TableSeparator([string]$line) {
    $line = $line.Trim()
    return $line -match '^\|[\s\|\-:]+\|$'
}

function Parse-TableRow([string]$line) {
    $line = $line.Trim()
    if ($line.StartsWith('|')) { $line = $line.Substring(1) }
    if ($line.EndsWith('|')) { $line = $line.Substring(0, $line.Length - 1) }
    return @($line -split '\|' | ForEach-Object { $_.Trim() })
}

function Build-TableXml([string[]]$tableLines) {
    if ($tableLines.Count -lt 2) { return $null }
    
    $headerCells = Parse-TableRow $tableLines[0]
    $colCount = $headerCells.Count
    
    # Build table header row
    $headerRows = @('<w:tr>')
    foreach ($cell in $headerCells) {
        $cellText = Escape-Xml $cell
        $headerRows += @(
            '<w:tc>',
            '<w:tcPr>',
            '<w:tcW w:w="1440" w:type="dxa"/>',
            '<w:shd w:fill="D9E1F2"/>',
            '</w:tcPr>',
            '<w:p>',
            '<w:r>',
            '<w:rPr><w:b/></w:rPr>',
            "<w:t xml:space=""preserve"">$cellText</w:t>",
            '</w:r>',
            '</w:p>',
            '</w:tc>'
        )
    }
    $headerRows += '</w:tr>'
    
    # Build data rows
    $tableRows = $headerRows
    for ($i = 2; $i -lt $tableLines.Count; $i++) {
        $cells = Parse-TableRow $tableLines[$i]
        $tableRows += '<w:tr>'
        foreach ($cell in $cells) {
            $cellText = Escape-Xml $cell
            $tableRows += @(
                '<w:tc>',
                '<w:tcPr><w:tcW w:w="1440" w:type="dxa"/></w:tcPr>',
                '<w:p>',
                "<w:r><w:t xml:space=""preserve"">$cellText</w:t></w:r>",
                '</w:p>',
                '</w:tc>'
            )
        }
        $tableRows += '</w:tr>'
    }
    
    $tableXml = @(
        '<w:tbl>',
        '<w:tblPr>',
        "<w:tblW w:w=""$(5000)""w:type=""dxa""/>",
        '<w:tblBorders>',
        '<w:top w:val="single" w:sz="12" w:space="0" w:color="000000"/>',
        '<w:left w:val="single" w:sz="12" w:space="0" w:color="000000"/>',
        '<w:bottom w:val="single" w:sz="12" w:space="0" w:color="000000"/>',
        '<w:right w:val="single" w:sz="12" w:space="0" w:color="000000"/>',
        '<w:insideH w:val="single" w:sz="12" w:space="0" w:color="000000"/>',
        '<w:insideV w:val="single" w:sz="12" w:space="0" w:color="000000"/>',
        '</w:tblBorders>',
        '</w:tblPr>'
    ) + $tableRows + '</w:tbl>'
    
    return [string]::Join("`n", $tableXml)
}

$mdLines = Get-Content $mdPath
$paras = New-Object System.Collections.Generic.List[string]

$i = 0
while ($i -lt $mdLines.Count) {
    $line = $mdLines[$i]
    $t = $line.TrimEnd()
    
    # Check if this is the start of a table
    if (Is-TableRow $t) {
        $tableLines = @($t)
        $i++
        
        # Collect separator row
        if ($i -lt $mdLines.Count -and (Is-TableSeparator $mdLines[$i])) {
            $i++
        }
        
        # Collect data rows
        while ($i -lt $mdLines.Count -and (Is-TableRow $mdLines[$i])) {
            $tableLines += $mdLines[$i].TrimEnd()
            $i++
        }
        
        $tableXml = Build-TableXml $tableLines
        if ($tableXml) {
            $paras.Add($tableXml)
        }
        continue
    }
    
    if ($t -eq '') {
        $paras.Add('<w:p/>')
        $i++
        continue
    }

    $style = $null
    if ($t -match '^#\s+(.+)$') {
        $style = 'Heading1'
        $t = $Matches[1]
    } elseif ($t -match '^##\s+(.+)$') {
        $style = 'Heading2'
        $t = $Matches[1]
    } elseif ($t -match '^###\s+(.+)$') {
        $style = 'Heading3'
        $t = $Matches[1]
    } elseif ($t -match '^####\s+(.+)$') {
        $style = 'Heading4'
        $t = $Matches[1]
    } elseif ($t -match '^[\*-]\s+(.+)$') {
        $t = "* " + $Matches[1]
    }

    $txt = Escape-Xml $t
    if ($style) {
        $paras.Add(('<w:p><w:pPr><w:pStyle w:val="{0}"/></w:pPr><w:r><w:t xml:space="preserve">{1}</w:t></w:r></w:p>' -f $style, $txt))
    } else {
        $paras.Add(('<w:p><w:r><w:t xml:space="preserve">{0}</w:t></w:r></w:p>' -f $txt))
    }
    $i++
}

$body = [string]::Join("`n", $paras)

$contentTypes = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
'@

$rels = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
</Relationships>
'@

$docRels = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"/>
'@

$documentXml = @"
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
$body
    <w:sectPr>
      <w:pgSz w:w="12240" w:h="15840"/>
      <w:pgMar w:top="1440" w:right="1440" w:bottom="1440" w:left="1440" w:header="708" w:footer="708" w:gutter="0"/>
    </w:sectPr>
  </w:body>
</w:document>
"@

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText((Join-Path $tmp '[Content_Types].xml'), $contentTypes, $utf8NoBom)
[System.IO.File]::WriteAllText((Join-Path $tmp '_rels\\.rels'), $rels, $utf8NoBom)
[System.IO.File]::WriteAllText((Join-Path $tmp 'word\\_rels\\document.xml.rels'), $docRels, $utf8NoBom)
[System.IO.File]::WriteAllText((Join-Path $tmp 'word\\document.xml'), $documentXml, $utf8NoBom)

if (Test-Path $outPath) {
    Remove-Item $outPath -Force
}

$zipPath = Join-Path $repo 'TECHNICAL_USER_GUIDE.zip'
if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}

Compress-Archive -Path (Join-Path $tmp '*') -DestinationPath $zipPath -Force
Move-Item -Path $zipPath -Destination $outPath -Force
Remove-Item $tmp -Recurse -Force

Write-Output "Created $outPath"
