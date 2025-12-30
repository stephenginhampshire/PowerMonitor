# Power Monitor - Version Control Workflow

## Quick Release Process

After each successful build (`pio run`), create a versioned commit:

### Option 1: Using the Helper Script (Recommended)

```powershell
.\git-release.ps1 "12.28" "Weather API timeout handling improved"
```

### Option 2: Manual Commands

```powershell
# 1. Stage source files
git add src/ platformio.ini Code_Review_Recommendations.txt

# 2. Create commit with changelog entry
git commit -m "V12.28 - Weather API timeout handling improved"

# 3. Tag the version
git tag -a v12.28 -m "V12.28 - Weather API timeout handling improved"

# 4. Push to remote (optional)
git push origin main --tags
```

## Version Numbering

Follow the existing pattern:
- **Major.Minor format**: `12.27`, `12.28`, etc.
- Increment minor version for each change
- Keep version in sync with:
  - `src/Power_Monitor.ino` line ~76: `String version = "V12.27";`
  - Changelog in `src/Power_Monitor.ino` lines 30-56
  - `Code_Review_Recommendations.txt` header

## What Gets Committed

**Include:**
- `src/Power_Monitor.ino` - Main source code
- `src/Configuration.h` - WiFi/API config
- `src/KWS_Protocol.h` - RS485 protocol
- `platformio.ini` - Build configuration
- `Code_Review_Recommendations.txt` - Issue tracking

**Exclude (via .gitignore):**
- `.pio/` - Build artifacts and dependencies
- `Debug/`, `Release/` - Visual Micro build folders
- `__vm/.Power_Monitor.vsarduino.h` - Generated files
- `.vscode/launch.json` - Local debug config

## Viewing History

```powershell
# Show recent commits
git log --oneline -10

# Show all version tags
git tag --sort=-version:refname

# View specific version
git show v12.27

# Compare versions
git diff v12.26 v12.27
```

## Rollback if Needed

```powershell
# Revert to previous version
git checkout v12.26

# Create branch from old version
git checkout -b bugfix-from-12.26 v12.26

# Return to latest
git checkout main
```

## Current Version: V12.27

Last successful build: December 30, 2025
- RAM: 24.5% (80,184 bytes)
- Flash: 53.3% (1,047,033 bytes)
