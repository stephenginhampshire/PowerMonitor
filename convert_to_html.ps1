$md = Get-Content 'TECHNICAL_USER_GUIDE.md' -Raw
$lines = $md -split "`n"
$html = @"
<!DOCTYPE html>
<html>
<head><meta charset="UTF-8"><title>Power Monitor Technical User Guide</title>
<style>
body { font-family: Calibri; font-size: 11pt; line-height: 1.15; }
p { margin: 0; padding: 0; }
h1 { margin: 12pt 0 6pt 0; font-size: 16pt; }
h2 { margin: 10pt 0 4pt 0; font-size: 14pt; }
h3 { margin: 8pt 0 4pt 0; font-size: 12pt; }
ul { margin: 4pt 0; padding-left: 20pt; }
li { margin: 0; }
table { margin: 8pt 0; }
</style>
</head>
<body>
"@

$inTable = $false
$tableRows = @()

foreach ($line in $lines) {
    $line = $line.Trim("`r")
    
    # Handle tables
    if ($line -match '^\|(.+)\|$') {
        if (-not $inTable) { $inTable = $true; $tableRows = @() }
        $tableRows += $line
        continue
    } elseif ($inTable) {
        $html += "<table border='1' style='border-collapse:collapse'>`n"
        $isFirst = $true
        foreach ($row in $tableRows) {
            if ($row -match '^[\|\s\-:]+$') { continue }
            $cells = $row -split '\|' | Where-Object { $_.Trim() }
            if ($isFirst) {
                $html += "<tr>"
                foreach ($cell in $cells) { $html += "<th style='background:#DDD;padding:4px'>$($cell.Trim())</th>" }
                $html += "</tr>`n"
                $isFirst = $false
            } else {
                $html += "<tr>"
                foreach ($cell in $cells) { $html += "<td style='padding:4px'>$($cell.Trim())</td>" }
                $html += "</tr>`n"
            }
        }
        $html += "</table>`n"
        $inTable = $false
        $tableRows = @()
    }
    
    # Handle headings and content
    if ($line -match '^# (.+)$') { $html += "<h1 style='mso-style-name:""Heading 1""'>$($matches[1])</h1>`n" }
    elseif ($line -match '^## (.+)$') { $html += "<h2 style='mso-style-name:""Heading 2""'>$($matches[1])</h2>`n" }
    elseif ($line -match '^### (.+)$') { $html += "<h3 style='mso-style-name:""Heading 3""'>$($matches[1])</h3>`n" }
    elseif ($line -match '^#### (.+)$') { $html += "<h4 style='mso-style-name:""Heading 4""'>$($matches[1])</h4>`n" }
    elseif ($line -match '^[\*\-] (.+)$') { $html += "<li>$($matches[1])</li>`n" }
    elseif ($line.Trim()) {
        $processed = $line -replace '\*\*(.+?)\*\*', '<b>$1</b>' -replace '`(.+?)`', '<code>$1</code>'
        $html += "<p>$processed</p>`n"
    }
}

$html += "</body></html>"
$html | Out-File 'TECHNICAL_USER_GUIDE.html' -Encoding UTF8
Write-Host "HTML created successfully"
