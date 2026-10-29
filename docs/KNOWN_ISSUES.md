# Current problems

These findings come from reviewing the checked-in source. They describe existing behavior and risks; this documentation change does not repair them. Unless explicitly stated in the [development guide](DEVELOPMENT.md), the scenarios below have not been reproduced in the GUI. The priorities are suggested repair order.

## Data loss and document state (high priority)

### Writes can fail silently while destructive actions continue

**Where:** [`SaveWorkingFile` and `ExecuteHandleUnsaved`](../Text%20Editor/FileHandling.cpp), [`ShowSaveFileDialog`](../Text%20Editor/FileDialog.cpp).

The writer truncates the target directly and checks neither open/write/close status nor conversion results. It returns `void`. If saving to a read-only path fails, the unsaved-changes gate still invokes New, Open, or Close for an existing path. Save As can also clear `haveUnsaved` after a failed write. An interruption during a write can leave a partial file.

**Repair direction:** Return a persistence result, preserve dirty state on failure, and stop the pending action. Write to a temporary sibling and replace the target only after a successful close, with errors shown to the user.

### New retains the previous file identity

**Where:** [`WindowProc`, `ID_FILE_NEW`](../Text%20Editor/MainSource.cpp).

New clears visible text and resets labels but does not free/reset `fileName`, reset `encoding`, or disable Save. After opening file A, choosing New, and typing, Ctrl+S can overwrite A even though the path display says `new`. The labels can also claim UTF-8 while the writer uses the prior encoding.

**Repair direction:** Reset the complete document state together, including path ownership, encoding, Save availability, dirty state, and status labels.

### Dirty state is inconsistent

**Where:** [`WindowProc` / `SwapMainEditControls`](../Text%20Editor/MainSource.cpp), [`OpenWorkingFile`, `SaveWorkingFile`, `ExecuteHandleUnsaved`](../Text%20Editor/FileHandling.cpp).

`EN_CHANGE` marks every text update dirty, including programmatic loads and wrap recreation. Ordinary Save never clears the flag. The unsaved-change gate clears it after a callback only when the gate was entered dirty, so an Open can leave different dirty state depending on the previous document. Cancellation of a subsequent Open can also occur after state was already changed by the prompt path.

**Repair direction:** Establish explicit success/cancel/failure results for document actions, suppress or classify programmatic changes, and update dirty state only at well-defined transitions.

## Memory safety and hangs (high priority)

### Find/replace advertises oversized buffers and retains unsafe pointers

**Where:** [`InitProgram` / `FindReplaceProc`](../Text%20Editor/MainSource.cpp).

`findWhat` and `replaceWith` contain 128 `WCHAR`s each, but the `FINDREPLACEW` lengths use `sizeof`, advertising 256 characters on Windows. The common dialog can therefore write beyond those arrays for sufficiently long input.

Replace locks the edit control buffer, sends `EM_REPLACESEL`, and searches through the previous pointer afterward. A control edit can reallocate that buffer, leaving a stale pointer and stale length/position assumptions. The static search position also survives document changes.

**Repair direction:** Supply character counts, copy text into owned storage before mutations, validate positions against the new text, and reset search state when a document changes.

### UTF-16 BE loading can loop forever

**Where:** [`LoadBufferUTF_16_BE`](../Text%20Editor/FileHandling.cpp).

The byte-swap loop uses `uint16_t i`. At 65,536 code units (131,072 file bytes, including any BOM), increment wraps to zero while the loop condition remains true. Because loading occurs on the UI thread, this hangs the window.

**Repair direction:** Use a size-appropriate index and validate that the byte count contains complete UTF-16 code units.

### Startup path ownership is incompatible with dialog cleanup

**Where:** [`wWinMain` / `OpenDropFile`](../Text%20Editor/MainSource.cpp), [`file dialog result handling`](../Text%20Editor/FileDialog.cpp).

`CommandLineToArgvW` receives the raw argument tail rather than a full executable command line. Empty or unquoted inputs can be interpreted unexpectedly; an empty tail can produce the executable's own path and trigger an attempt to load it as text. The returned argument allocation is never released with `LocalFree`.

When a single argument is accepted, `fileName` borrows `argv[0]`. Later Open, Save As, and drop paths pass the old value to `CoTaskMemFree`, which is incompatible with that borrowed pointer. This can corrupt the heap or crash.

**Repair direction:** Parse the full command line with explicit argument rules, release the argument block correctly, and give the active path one consistent owning representation.

### Replace All has no empty-search guard

**Where:** [`FindReplaceProc`, `FR_REPLACEALL`](../Text%20Editor/MainSource.cpp).

With an empty search string, the counting loop advances by zero and can loop forever. This is a handler-level finding; an empty-search notification has not been reproduced through the dialog. Output size arithmetic also uses signed `int` and is unchecked for overflow with large inputs/replacements.

**Repair direction:** Reject empty terms and check replacement count/size arithmetic before allocation. Add focused tests when extracting the search logic.

## Encoding, file I/O, and preferences

### Input validation and error handling are incomplete

**Where:** [`OpenWorkingFile` and encoding helpers](../Text%20Editor/FileHandling.cpp).

`file_size` and allocations can throw; stream reads and conversion/normalization calls are unchecked. The implementation does not validate malformed UTF-8, odd-length UTF-16, or short reads. Whole-file allocation plus conversion copies increases memory use, and all processing blocks the UI. Several APIs accept narrower counts than the `UINT64` file size. `TextLimit = MAXINT` is not a practical large-file guarantee.

**Repair direction:** Validate sizes and conversion results, handle exceptions at action boundaries, expose errors, and define a supported file-size limit before introducing asynchronous I/O.

### Round trips are not byte preserving

**Where:** [`LoadBufferUTF_8`, UTF-16 loaders, and `SaveWorkingFile`](../Text%20Editor/FileHandling.cpp), [`ShowSaveFileDialog`](../Text%20Editor/FileDialog.cpp).

UTF-8 input is normalized to NFC. String-based APIs stop at embedded NULs. UTF-16 saves always add a BOM, and Save As defaults to UTF-8 rather than the current encoding. BOM-less auto detection is heuristic and offers no explicit legacy-code-page support. Mixed-line-ending preservation is not tested, and the status text reflects the control's mode rather than a full-file analysis.

**Repair direction:** Decide and document preservation policy, default Save As to the active encoding, use length-aware conversions, and test exact output bytes for every supported encoding and line-ending case.

### Font registry serialization is incorrect

**Where:** [`ReadFontFromRegistry` / `WriteFontToRegistry`](../Text%20Editor/FontHandling.cpp).

Several DWORD registry values are read into one-byte `LOGFONTW` fields using a four-byte buffer size, overwriting neighboring fields. Those same fields are then assigned from stale `data` left over from earlier reads. `lfClipPrecision` is never saved or restored, and reads accept any registry type. Successful API calls therefore do not imply a faithfully restored font.

**Repair direction:** Read each DWORD into a DWORD temporary, validate its type/range, and assign the intended field. Persist all required fields consistently and validate partial/malformed keys.

## UI, build configuration, and maintainability

- **Word wrap loses editing history:** `SwapMainEditControls` destroys the old control, losing its undo and scroll state. It can trigger dirty notifications and briefly routes messages while `hMainEdit` still references the old control. Preserve document/UI state deliberately when replacing the control.
- **Go to accepts invalid input:** `GotoProc` converts empty/zero input into `-1`, which has special current-line meaning for `EM_FILELINEINDEX`; explicitly validate positive line numbers before sending the message.
- **Zoom labels use an invalid format:** zoom menu handlers use `L"%llu%"` with an unescaped trailing `%` and ignore the formatting result. Use `%%` or string concatenation and check the result.
- **Layout uses fixed pixels:** font heights, top field height, status partition widths, and editor offsets are hard-coded. Tiny windows can produce negative widths/heights; no explicit DPI-aware layout is implemented.
- **About is a placeholder:** `ID_HELP_ABOUT` exists in the resource menu but has no handler in `WindowProc`.
- **x86 configuration is incomplete:** Win32 settings omit the x64 configurations' C++20 standard, Windows subsystem, and additional libraries. The source uses C++20 designated initializers and `wWinMain`; x86 needs configuration alignment before it is a supported build path.
- **Unrelated project item:** `.vcxproj` and `.filters` reference an external `Cs/MvcMovie/.../MvcMovie.exe` as a `None` item. This is stale IDE metadata, not an application dependency.
- **Ownership and diagnostics are manual:** cleanup of buffers, paths, interfaces, and GDI objects is scattered. Startup errors often exit with code zero and no explanation; `GetMessageW` errors are not distinguished from normal quit. RAII wrappers and explicit action results would make cleanup and errors easier to reason about.
- **No automated tests or CI are checked in:** conversion, document transitions, and search logic are coupled to controls and globals. Extracting those operations would permit focused tests without launching the GUI.

Suggested implementation order: protect saving and New first; repair dialog buffers, path ownership, and the BE loop next; then isolate encoding/search logic and align build configurations. These are proposals, not changes included in this documentation update.
