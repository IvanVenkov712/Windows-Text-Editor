# Windows Text Editor

A native Windows desktop text editor written in C++20 using the Win32 API. It uses the standard Windows multiline edit control, common dialogs, and a status bar, with one document per process.

## Features

- Create, open, save, and drag a file into the window; launch another editor process with New Window.
- Read and write UTF-8, UTF-8 with BOM, UTF-16 LE, and UTF-16 BE, with automatic encoding detection on open.
- Undo, clipboard editing, find and replace, Go to line, and local time/date insertion.
- Word wrap, zoom, a selectable font, and a status bar showing caret position, zoom, line endings, and encoding.
- Store the editor font in `HKEY_CURRENT_USER\TextEditor`.

The implementation has known correctness issues. In particular, failed writes can go unreported, New can retain the previous file path, and large UTF-16 BE files can hang during loading. Use disposable copies while evaluating the editor; see [current problems](docs/KNOWN_ISSUES.md).

## Build and run

The checked-in project targets Visual Studio 2022, the MSVC **v143** toolset, and an installed Windows SDK (`WindowsTargetPlatformVersion` is `10.0`). Install the **Desktop development with C++** workload. There are no third-party package dependencies.

1. Open [`Text Editor.sln`](Text%20Editor.sln) in Visual Studio.
2. Select **Debug | x64** or **Release | x64**. The x86 configurations need repairs described in the [development guide](docs/DEVELOPMENT.md).
3. Build the solution, then run with **F5** or **Ctrl+F5**.

From a Visual Studio Developer PowerShell:

```powershell
msbuild '.\Text Editor.sln' /t:Build /p:Configuration=Debug /p:Platform=x64
& '.\x64\Debug\Text Editor.exe'
```

For Release, replace `Debug` with `Release` in both commands. Command-line file opening exists but has parsing and memory-ownership problems; use **File > Open** for now.

## Documentation

- [Using the editor](docs/USAGE.md): commands, shortcuts, and encoding behavior.
- [Architecture](docs/ARCHITECTURE.md): modules, shared state, event flow, and ownership.
- [Current problems](docs/KNOWN_ISSUES.md): source findings, impact, and repair priorities.
- [Development](docs/DEVELOPMENT.md): build configuration, verification, and manual checks.
- [GitHub metadata](docs/GITHUB_METADATA.md): a repository description and suggested topics.

Public function contracts and local UI helpers have Doxygen-style comments in the C++ source and headers. No Doxygen configuration is included.

## License

[MIT](LICENSE), copyright 2026 IvanVenkov712.
