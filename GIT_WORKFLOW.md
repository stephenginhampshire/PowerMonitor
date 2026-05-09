# Power Monitor - Version Control Workflow

## Quick Release Process

After each successful build (`pio run`), create a versioned commit:

### Option 1: Using the Helper Script (Recommended)

```powershell
.\git-release.ps1 "1.3.3" "Brief description of changes"
```

### Option 2: Manual Commands

```powershell
# 1. Stage source files
git add src/ platformio.ini CHANGELOG.md VERSIONING.md

# 2. Create commit with changelog entry
git commit -m "V1.3.3 - Brief description of changes"

# 3. Tag the version
git tag -a v1.3.3 -m "V1.3.3 - Brief description of changes"

# 4. Push to remote (optional)
git push origin main --tags
```

## Version Numbering

Follow semantic versioning:
- **MAJOR.MINOR.PATCH** format: `1.3.2`, `1.3.3`, etc.
- Increment MAJOR for breaking changes
- Increment MINOR for backwards-compatible features
- Increment PATCH for backwards-compatible fixes
- Keep version in sync with:
  - `src/Power_Monitor.ino` `VERSION_MAJOR`, `VERSION_MINOR`, `VERSION_PATCH`, `VERSION_STRING`
  - `CHANGELOG.md` latest entry
  - `VERSIONING.md` current version section

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
git show v1.3.2

# Compare versions
git diff v1.3.1 v1.3.2
```

## Rollback if Needed

```powershell
# Revert to previous version
git checkout v1.3.1

# Create branch from old version
git checkout -b bugfix-from-1.3.1 v1.3.1

# Return to latest
git checkout main
```

## Current Version: V1.3.2

Last successful build: December 30, 2025
- RAM: 24.5% (80,184 bytes)
- Flash: 53.3% (1,047,033 bytes)
