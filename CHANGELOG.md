# Changelog

All notable changes to CellAnalyzer will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **Negative Markers Support**: Added right-click functionality in ParameterTuningWidget to mark areas to exclude from detection (red crosses)
- **New Cell List Widget**: Introduced `CellListItemWidget` for improved cell display in list view
- **Markup Image Widget**: Added `MarkupImageWidget` for advanced image visualization with annotations
- **Help Menu**: Added "About" dialog accessible via F1 or Help menu, displaying README.md content
- **Portable Settings**: Settings now stored next to executable for easy portability
- **Statistics Thresholds**: Added configurable min/max thresholds for statistical analysis in SettingsManager
- **Preset Management API**: New methods in SettingsManager for managing presets (getPresets, setPresets, getLastSelectedPreset)

### Changed
- **Verification Widget Redesign**: Complete UI overhaul with file tabs, left cell list panel, and right preview panel (25%/75% split)
- **Parameter Tuning Workflow**: Removed auto-apply on parameter change; now requires explicit "Apply Parameters" button click
- **Window Size**: Increased default window size from 800x600 to 1200x800 for better visibility
- **Preview Size**: Removed preview size slider, set fixed optimal size (200px)
- **Preset Loading**: Last-used preset now loads without auto-applying, allowing manual verification before use
- **Selection Display**: Enhanced marker display showing positive (●) and negative (✕) markers separately with counts
- **Settings Storage**: Moved from AppData to application directory for portable deployment

### Improved
- **Coordinate Mapping**: Fixed coordinate mapping in ParameterTuningWidget for accurate cell selection
- **UI/UX**: Better visual feedback with separate positive and negative marker indicators
- **Code Organization**: Refactored VerificationWidget with cleaner separation of concerns
- **Documentation**: Updated CLAUDE.md and README.md with comprehensive feature descriptions
- **Cell Preview**: Enhanced cell preview with improved layout and navigation

### Removed
- **Build Scripts**: Cleaned up redundant build scripts (build-portable.bat, build-release.bat, build-release.ps1, build-release.cmake)
- **Utility Scripts**: Removed check-dependencies.bat, prepare-win7.bat, package-portable.bat, test_run.bat
- **Test Files**: Removed temporary test images and debug logs (TIM1.jpg, debug_cells_highlighted.png, cell_analyzer_debug.log)
- **Preview Size Slider**: Removed from toolbar in favor of fixed optimal size

### Fixed
- **Parameter Application**: Fixed issue where parameters were applied automatically on every change
- **Cell Selection**: Improved accuracy of cell selection with proper coordinate transformation
- **Preset Management**: Fixed preset loading to not auto-apply, preventing unwanted parameter changes

## [Previous Versions]

### [1.0.0] - 2025-01-XX
- Initial release
- Basic cell detection using HoughCircles algorithm
- Parameter tuning interface
- CSV export functionality
- Preset management system
- Scale bar detection
