# ESP32 Fish Tank Automation System - Version Information

**Current Version**: 3.0.1  
**Release Date**: October 28, 2025  
**Author**: mhd-riaz  
**Status**: Production Ready ✅

---

## Version Number Format: X.Y.Z

```
X = Major Version (Breaking changes, major features, significant redesigns)
Y = Minor Version (New features, non-breaking enhancements, new capabilities)
Z = Patch Version (Bug fixes, small improvements, configuration tweaks)
```

### Current: 3.0.1
- **3** = Major version (commercial product design)
- **0** = Minor version (no new features yet in this major)
- **1** = Patch version (display rotation support added)

---

## Quick Version Reference

| Version | Date | Notes |
|---------|------|-------|
| 3.0.1 | Oct 28, 2025 | Display rotation support |
| 3.0.0 | Oct 21, 2025 | Initial commercial release |
| 2.x | Previous | 5-device system |
| 1.x | Initial | Prototype |

---

## When to Update Version Numbers

### Major Version (X) ↑
- Breaking API changes
- Significant architecture redesign
- Major feature additions (e.g., 6-device → 12-device support)
- Incompatible with previous versions

### Minor Version (Y) ↑
- New REST API endpoints
- New appliance types or features
- New scheduling capabilities
- New configuration options
- Backward compatible

### Patch Version (Z) ↑
- Bug fixes
- Small improvements
- Configuration tweaks
- Display rotation support
- Memory optimizations
- Documentation updates
- No feature additions

---

## Update Checklist

When making changes, update version in ALL these files:

- [ ] **esp32-scheduler.ino** - Line 3 header comment
  ```cpp
  // Version: X.Y.Z - Description
  // Date: Month DD, YYYY
  ```

- [ ] **README.md** - Top section
  ```markdown
  ### **Current Version**: X.Y.Z
  ### **Release Date**: Month DD, YYYY
  ```

- [ ] **CHANGELOG.md** - New section
  ```markdown
  ## [X.Y.Z] - Month DD, YYYY
  ```

- [ ] **VERSION file** - This file
  ```
  **Current Version**: X.Y.Z
  **Release Date**: Month DD, YYYY
  ```

---

## Recent Changes

### 3.0.1 Updates (Oct 28, 2025)
- ✅ Added OLED display rotation to 90° clockwise
- ✅ Created comprehensive CHANGELOG.md
- ✅ Updated version tracking system
- ✅ Enhanced README with system analysis

### 3.0.0 Release (Oct 21, 2025)
- ✅ 6-device support (Filter, CO2, Light, Heater, HangOnFilter, WaveMaker)
- ✅ REST API with 9 endpoints
- ✅ Temperature-based heater control
- ✅ Emergency safety system
- ✅ OLED display monitoring
- ✅ WiFi + NTP synchronization

---

## Development Guidelines

1. **Make your changes** to the code
2. **Test thoroughly** - Compile and functional testing
3. **Decide version bump** - Use decision guide above
4. **Update all files** with new version and date
5. **Document changes** in CHANGELOG.md
6. **Commit with message**: `v3.0.1: Add feature/fix bug/improve performance`

---

**For complete version details, see [CHANGELOG.md](CHANGELOG.md)**
