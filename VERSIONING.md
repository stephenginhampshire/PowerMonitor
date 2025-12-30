# Power Monitor - Semantic Versioning Guide

## Current Version: V1.0.0

Starting from December 30, 2025, this project uses **Semantic Versioning (SemVer)** to clearly communicate the nature and impact of changes in each release.

## Version Format: MAJOR.MINOR.PATCH

### Version Numbers Explained

**Format:** `MAJOR.MINOR.PATCH` (e.g., V1.0.0, V1.2.3, V2.0.0)

### MAJOR Version (X.0.0)
Increment for **breaking changes** that require action from users:
- Hardware schematic changes (pin assignments, component changes)
- Configuration file format changes (incompatible with previous config.json)
- Data file format changes (incompatible with existing data files)
- API changes that break existing functionality
- Removal of features
- Changes requiring re-upload to all deployed units

**Example:** V2.0.0 if config.json format changes to require new fields

### MINOR Version (1.X.0)
Increment for **new features** that are backwards-compatible:
- New sensor support added
- New web pages or API endpoints
- New configuration options (with sensible defaults)
- Additional data logging capabilities
- New communication protocols
- Performance improvements
- Enhanced functionality

**Example:** V1.1.0 if adding temperature sensor support

### PATCH Version (1.0.X)
Increment for **bug fixes and minor improvements**:
- Bug fixes that don't change functionality
- Performance optimizations
- Code refactoring (no functional changes)
- Documentation improvements
- Minor UI tweaks
- Security patches

**Example:** V1.0.1 if fixing WiFi reconnection bug

## Version History

### V1.0.0 (December 30, 2025) - First Stable Release
Major milestone after comprehensive code improvements:
- Semantic versioning implementation
- Configuration file system (config.json)
- Circular log buffer (500 entries)
- System health monitoring
- Enhanced WiFi recovery with exponential backoff
- SD card retry logic (3 attempts)
- File write verification
- Date/time validation
- Non-blocking patterns throughout
- RS485 communication improvements
- WhatsApp message deduplication
- Gas volume persistence in EEPROM
- Debug level system
- Watchdog timer implementation
- All V12.17-V12.37 improvements consolidated

### Pre-1.0.0 Versions (V12.17 and earlier)
Legacy versioning using V12.X format. See changelog in Power_Monitor.ino for details.

## Implementation Details

### Code Constants
```cpp
#define VERSION_MAJOR 1   // Breaking changes
#define VERSION_MINOR 0   // New features
#define VERSION_PATCH 0   // Bug fixes
#define VERSION_STRING "V1.0.0"
```

### Helper Function
```cpp
String get_version_string() {
    String version = "Version " VERSION_STRING;
    version += " (Built: " + String(__DATE__) + " " + String(__TIME__) + ")";
    return version;
}
```

## Decision Matrix for Version Increments

| Change Type | Version Increment | Example |
|-------------|-------------------|---------|
| Config format incompatible | MAJOR | V2.0.0 |
| Hardware pin change | MAJOR | V2.0.0 |
| Data file format change | MAJOR | V2.0.0 |
| Add new sensor | MINOR | V1.1.0 |
| Add new web page | MINOR | V1.2.0 |
| Add config option (compatible) | MINOR | V1.1.0 |
| Fix WiFi bug | PATCH | V1.0.1 |
| Refactor code (no changes) | PATCH | V1.0.1 |
| Update documentation | PATCH | V1.0.1 |

## Git Workflow

1. Make changes to code
2. Update version constants in Power_Monitor.ino:
   ```cpp
   #define VERSION_MAJOR X
   #define VERSION_MINOR Y
   #define VERSION_PATCH Z
   #define VERSION_STRING "VX.Y.Z"
   ```
3. Update changelog in Power_Monitor.ino header
4. Build and verify: `pio run`
5. Commit: `git commit -m "VX.Y.Z - Description"`
6. Tag: `git tag -a vX.Y.Z -m "Description"`

## Benefits of Semantic Versioning

1. **Clear Communication**: Users instantly understand the impact of an update
2. **Confidence**: PATCH updates are safe; MAJOR updates need attention
3. **Planning**: Teams can plan upgrade schedules based on version numbers
4. **Compatibility**: Easy to identify compatible versions
5. **Professional**: Industry-standard versioning system

## Field Deployment Considerations

- **PATCH updates**: Can be deployed immediately, low risk
- **MINOR updates**: Test on one unit before mass deployment
- **MAJOR updates**: Requires planning, testing, and may need config file updates

## Questions?

For version-related questions, check:
- This document (VERSIONING.md)
- Changelog in Power_Monitor.ino header
- Git tags: `git tag -l -n9` to see all tagged versions with notes
- Code_Review_Recommendations.txt for implementation details
