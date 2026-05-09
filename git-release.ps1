# Git Release Helper Script
# Usage: .\git-release.ps1 "1.3.3" "Description of changes"

param(
    [Parameter(Mandatory=$true)]
    [string]$Version,
    
    [Parameter(Mandatory=$true)]
    [string]$Description
)

Write-Host "Creating release for V$Version..." -ForegroundColor Cyan

# Stage source files only (exclude build artifacts)
Write-Host "`nStaging files..." -ForegroundColor Yellow
git add src/ platformio.ini Code_Review_Recommendations.txt Configuration.h KWS_Protocol.h

# Show what will be committed
Write-Host "`nFiles to be committed:" -ForegroundColor Yellow
git status --short

# Create commit with descriptive message
Write-Host "`nCreating commit..." -ForegroundColor Yellow
$commitMessage = "V$Version - $Description"
git commit -m $commitMessage

if ($LASTEXITCODE -eq 0) {
    # Create annotated tag
    Write-Host "`nCreating tag v$Version..." -ForegroundColor Yellow
    $tagMessage = "V$Version - $Description"
    git tag -a "v$Version" -m $tagMessage
    
    Write-Host "`n[OK] Successfully created release V$Version" -ForegroundColor Green
    Write-Host "`nTo push to remote (if configured):" -ForegroundColor Cyan
    Write-Host "  git push origin main --tags" -ForegroundColor White
    
    # Show recent tags
    Write-Host "`nRecent versions:" -ForegroundColor Cyan
    git tag --sort=-version:refname | Select-Object -First 5
} else {
    Write-Host "`n[ERROR] Commit failed - check for errors above" -ForegroundColor Red
}
