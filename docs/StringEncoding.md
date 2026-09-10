# OIViewer string encoding

OIViewer string conversion is locale-independent. Narrow application text (`char` and `char8_t`) is UTF-8.
Native text uses UTF-8 on Linux and UTF-16 through `wchar_t` and wide Windows APIs on Windows.

## Why encoding is fixed

UTF-8 Linux application text and UTF-16 Windows Unicode interfaces already identify their encodings. Converting
between them does not need the system locale. Consulting locale settings adds another interpretation of the same
bytes, allowing filenames and GPU names to change or fail after regional settings change. Explicit Unicode
encodings preserve multilingual text and make conversion predictable across machines.

For example, bytes `C3 A9` always mean U+00E9. A single byte `E9` is invalid UTF-8 and is rejected when converting
to wide text, even under a Windows-1252 locale. `char` itself does not specify UTF-8; that meaning comes from this
API contract. Windows `wchar_t` holds UTF-16 code units: supplementary characters require surrogate pairs.

Locale remains useful for language, date/number formatting, and linguistic sorting. It is redundant for these
encoding conversions because the representations are explicit. Linux filesystems allow arbitrary filename bytes;
UTF-8 is the application's text contract, not an operating-system guarantee. Native byte paths retain their bytes,
but malformed UTF-8 cannot be transcoded to wide text through this strict API.

## Shared API and performance choices

Use `LLUtils::StringUtility::ConvertString<Destination>(source)`. The
[LLUtils contract](../External/LLUtils/docs/StringEncoding.md) defines inputs, ownership, validation, exceptions,
ASCII casing, and bounded copying. C++23, native `char8_t`, and string `resize_and_overwrite` support are required.

```cpp
using LLUtils::StringUtility;
auto native = StringUtility::ConvertString<LLUtils::native_string_type>(u8"GPU \u00e9 \U0001f4f7");
auto bytes = StringUtility::ConvertString<std::string>(native);
auto typed = StringUtility::ConvertString<std::u8string>(bytes);
```

Byte/wide transcoding validates and emits output in one pass, with at most one destination allocation. A checked
upper bound avoids a sizing pass; `resize_and_overwrite` avoids preliminary zero-filling. The result intentionally
retains spare capacity instead of paying for another allocation or traversal. Same-type moves reuse storage;
identity and `char`/`char8_t` copies preserve code units without validating them. Length-aware conversion inputs
preserve embedded NULs, while pointer inputs stop at the first NUL.

`StrCpy` requires valid Unicode and a NUL-free source view. It trusts the supplied length, checks only the truncation
boundary, copies directly, and appends NUL. It does not decode, scan for a terminator, allocate, or pad. This trusted
copy contract is deliberately different from strict transcoding. Tray tooltips use their known string length and
accept truncation to the shell's capacity without splitting a code point. Grapheme clusters can still be split.

ASCII casing changes only A-Z/a-z and preserves non-ASCII code units. It does not perform linguistic case folding.

## Integration boundaries

Renderer adapter names are UTF-8. D3D11 descriptions enter through UTF-16 and are explicitly encoded. System
information converts backend text to the native representation. LLUtils replaces OIVShared's duplicate UTF-8
helpers. The build selects one authoritative LLUtils implementation despite older nested dependency copies.

Windows command-line text remains wide. Console output uses UTF-16 and redirected diagnostics use UTF-8. Error
messages come from `FormatMessageW`, preserving Windows language selection, then convert to the requested encoding.

Clipboard text is published and requested as `CF_UNICODETEXT`; Windows synthesizes legacy formats when required.
Incoming text requires whole UTF-16 code units and an in-buffer terminator. DIB image formats retain priority.
Narrow clipboard setters accept UTF-8 and report malformed input through the existing failure status.

The bundled Windows FreeType loader still uses `CreateFileA`. Its wrapper explicitly converts native wide paths
to the selected Windows file API code page (ANSI or OEM), rejects substitution/failure, and handles a UTF-8 code
page directly. This legacy boundary is outside StringUtility. Unrepresentable font paths cannot be opened through
that loader; replacing this limitation requires a Unicode-capable or custom FreeType file loader. Linux font paths
retain their original bytes. Standalone narrow Windows builds decode their UTF-8 filenames at this same boundary;
OIViewer borrows its existing wide filename without copying it.

## Migration and validation

This intentionally replaces locale-dependent narrow/wide conversion without a deprecated compatibility layer,
encoding tag, or alternate conversion API. Legacy data must be decoded explicitly at the boundary that owns its
format. Bounded-copy callers must supply valid, NUL-free views and handle or explicitly accept reported truncation.

See [validation results](StringEncodingValidation.md) for correctness, allocation, performance, and runtime checks.
