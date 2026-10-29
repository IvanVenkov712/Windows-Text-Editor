# Using the editor

The top read-only field displays the current file path (or `new`). The main area contains the document. The status bar displays the caret's line/character position, zoom, line-ending mode, and encoding.

## Commands and shortcuts

| Action | Shortcut / location |
| --- | --- |
| New document | Ctrl+N |
| New editor process | Ctrl+Shift+N |
| Open | Ctrl+O |
| Save existing file | Ctrl+S |
| Save As / first save | Ctrl+Shift+S |
| Undo | Ctrl+Z |
| Cut / Copy / Paste | Ctrl+X / Ctrl+C / Ctrl+V |
| Delete selection | Delete |
| Select all | Edit > Select all |
| Find / Replace | Ctrl+F / Ctrl+H |
| Go to line | Ctrl+G |
| Insert local time and date | F5 |
| Zoom in / out | Ctrl+Plus / Ctrl+Minus; keypad variants are registered too |
| Restore zoom | Ctrl+0; keypad 0 also works |
| Word wrap / Font | Format menu |
| Show or hide status bar | View > Status bar |
| Close | File > Exit or the window close button |

Ctrl+S is disabled until a file has been opened or saved with Save As. New, Open, drop, and Close use an unsaved-changes prompt. Its Yes/No/Cancel choices mean save/discard/cancel, but failed writes are not reliably detected. See the [known issues](KNOWN_ISSUES.md) before testing these workflows.

Dragging files into the window opens only the first dropped file. New Window starts another process; there are no tabs or shared document sessions. Find and Replace use modeless Windows dialogs with a match-case option. Whole-word and direction controls are hidden. Go to expects a one-based line number; zero/empty input is not validated correctly.

## Encodings and line endings

Open offers **Auto find**, **UTF-8**, **UTF-8 BOM**, **UTF-16 LE**, and **UTF-16 BE**. Auto detection checks BOMs, then `IsTextUnicode` heuristics, and falls back to UTF-8. It is not a general encoding detector and does not support legacy code pages explicitly.

Save As offers the four explicit encodings and defaults to UTF-8, even when the current document uses another encoding. Ordinary Save uses the global encoding selected or detected earlier. UTF-16 saves always include a BOM. Loading UTF-8 applies Unicode NFC normalization, so saving may change the original byte representation even without intentional edits. Embedded NUL characters truncate text in the string-based loading path.

The edit control is configured to accept CRLF, CR, and LF. The status label reports the control's line-ending mode; there is no line-ending conversion command or guarantee of byte-for-byte preservation of mixed endings.

## Preferences and limitations

The default font is Consolas with a `LOGFONTW` height of 30 logical units. Font fields are read from `HKEY_CURRENT_USER\TextEditor` at startup and written on first key creation or after a font change when closing. Registry serialization currently has defects described in [known issues](KNOWN_ISSUES.md).

Word wrap recreates the edit control, copying its text, selection, font, and zoom. This loses undo history and can mark the document dirty. Help > About is present in the menu but has no command handler.
