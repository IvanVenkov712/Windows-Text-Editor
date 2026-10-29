# Development and verification

## Build configuration

The solution has Debug/Release configurations for x64 and x86. The solution's x86 platform maps to the project's Win32 platform. Use x64 initially: only those project configurations explicitly select C++20, the Windows subsystem, and `propsys.lib`, `shlwapi.lib`, and `comctl32.lib` in addition to inherited libraries.

Open `Text Editor.sln` in Visual Studio 2022 with the Desktop development with C++ workload, MSVC v143, and a Windows SDK installed. The project requests SDK version `10.0` without pinning a specific SDK build. It includes Windows SDK headers and the C++ standard library directly; there is no package-manager step.

From Developer PowerShell, in the repository root:

```powershell
msbuild '.\Text Editor.sln' /t:Build /p:Configuration=Debug /p:Platform=x64
msbuild '.\Text Editor.sln' /t:Build /p:Configuration=Release /p:Platform=x64
```

With default MSBuild paths the executable is `x64\Debug\Text Editor.exe` or `x64\Release\Text Editor.exe`. `msbuild` may need to be invoked by its full Visual Studio installation path outside Developer PowerShell. The project does not configure precompiled headers despite the `pch` filenames. `Resource.rc` is compiled as a Windows resource and linked into the executable.

Win32 configurations and the unrelated external `MvcMovie.exe` item need cleanup as described in [current problems](KNOWN_ISSUES.md). There is no installer, automated test target, or CI workflow in the repository.

## Documentation conventions

`ProgramHeader.h` documents public function contracts with Doxygen-style comments, including side effects, ownership, return behavior, and current limitations. File-level comments identify each module; local UI helpers are documented beside their definitions in `MainSource.cpp`. Include `pch.h` before `ProgramHeader.h`, which relies on Windows types.

When changing a document transition, encoding policy, callback contract, or resource lifetime, update the related source comment and guide. Keep known defects separate from intended behavior, and record whether a finding came from source inspection, a build, or GUI reproduction.

## Verification of this documentation update

The x64 Debug solution built successfully using the installed Visual Studio 2022 toolchain, both before and after adding source comments. Build outputs were redirected to a temporary directory; MSBuild emitted MSB8029 because those paths were under the system temporary directory. This warning concerns incremental-build reliability for that output location. Local Markdown links and code-fence balance were checked, `git diff --check` passed, and a comparison of C++ tokens confirmed that the source edits contain only comments/whitespace.

The documentation and comments do not change application logic. GUI behavior and the listed defect scenarios have not been exercised as part of this review. A successful build does not establish correct saving, encoding preservation, or memory ownership.

## Manual checks for future code changes

Use disposable files in a separate test directory. The following are intended checks for repairs; several currently fail based on source inspection.

| Area | Checks and expected result after repair |
| --- | --- |
| Document transitions | New, Open, drop, Close with Save/Discard/Cancel; cancellation keeps text/path intact. New detaches the previous path and resets encoding. |
| Saving | First Save As, ordinary Save, failed write to a protected/read-only target; failed persistence preserves dirty state and blocks the pending destructive action. |
| Encodings | UTF-8 with/without BOM, UTF-16 LE/BE with/without BOM, ASCII, combining characters, supplementary characters, malformed input, odd byte counts, empty files, and embedded NULs; compare resulting bytes with the documented policy. |
| Line endings | Separate CRLF/LF/CR and mixed-ending samples; check status and actual saved bytes. |
| Large inputs | Establish a size limit; verify BE input at/above 131,072 bytes completes after fixing the loop. Do not run that case against the current implementation on work you need to retain. |
| Search | Match case on/off, wraparound, absent/empty terms, long terms, replacement longer/shorter/empty, Replace All, and switching documents between searches. |
| UI state | Selection/caret status, Go to boundary/invalid lines, wrap with undo/selection, zoom labels, status visibility, and resize at different display scales. |
| Preferences | First launch, font change/restart, missing registry values, and malformed registry types after repairing font serialization. |
| Launch/drop | No arguments, one quoted path with spaces, invalid/multiple arguments, multiple dropped files, and switching from a startup file to a dialog-opened file. |
| Builds | x64 Debug and Release; x86 only after aligning its compiler/linker settings. |

For future automated coverage, start with encoding byte round trips, save-failure/document-state transitions, and replacement-size boundaries. These operations will benefit from extraction into functions that accept owned data and return explicit results, reducing dependence on global window handles.
