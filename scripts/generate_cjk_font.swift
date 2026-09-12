import CoreGraphics
import CoreText
import Foundation

guard CommandLine.arguments.count == 4 else {
  fputs("usage: generate_cjk_font.swift codepoints.txt font.ttf output.inc\n", stderr)
  exit(2)
}

let codepointURL = URL(fileURLWithPath: CommandLine.arguments[1])
let fontURL = URL(fileURLWithPath: CommandLine.arguments[2])
let outputURL = URL(fileURLWithPath: CommandLine.arguments[3])
let codepoints = try String(contentsOf: codepointURL, encoding: .utf8)
  .split { $0 == " " || $0 == "\n" }
  .map { UInt32($0)! }

var registrationError: Unmanaged<CFError>?
guard CTFontManagerRegisterFontsForURL(fontURL as CFURL, .process, &registrationError) else {
  let message = registrationError?.takeRetainedValue().localizedDescription ?? "unknown error"
  fatalError("could not register font: \(message)")
}

let canvas = 32
let glyphRowBytes = canvas / 8
let glyphBytes = canvas * glyphRowBytes
let font = CTFontCreateWithName("HYWenHei-65W" as CFString, CGFloat(canvas), nil)
let postScriptName = CTFontCopyPostScriptName(font) as String
guard postScriptName == "HYWenHei-FEW" else {
  fatalError("unexpected font: \(postScriptName)")
}
let colorSpace = CGColorSpaceCreateDeviceGray()
var output = "#pragma once\n#include <cstdint>\nnamespace spring::ui::detail {\n"
output += "inline constexpr char cjk_font_name[] = \"HYWenHei-65W Medium\";\n"
output += "inline constexpr std::uint8_t cjk_glyph_size = \(canvas);\n"
output += "inline constexpr std::uint8_t cjk_glyph_row_bytes = \(glyphRowBytes);\n"
output += "inline constexpr std::uint16_t cjk_glyph_bytes = \(glyphBytes);\n"
output += "inline constexpr std::uint32_t cjk_codepoints[] = {"
output += codepoints.map(String.init).joined(separator: ",")
output += "};\ninline constexpr std::uint8_t cjk_glyphs[] = {"

var missing = 0
for (index, codepoint) in codepoints.enumerated() {
  var character = UniChar(codepoint)
  var glyph = CGGlyph()
  guard CTFontGetGlyphsForCharacters(font, &character, &glyph, 1), glyph != 0 else {
    missing += 1
    output += Array(repeating: "0", count: glyphBytes).joined(separator: ",") + ","
    continue
  }

  var bounds = CGRect.zero
  CTFontGetBoundingRectsForGlyphs(font, .horizontal, &glyph, &bounds, 1)
  let pixels = UnsafeMutablePointer<UInt8>.allocate(capacity: canvas * canvas)
  pixels.initialize(repeating: 255, count: canvas * canvas)
  defer {
    pixels.deinitialize(count: canvas * canvas)
    pixels.deallocate()
  }

  guard let context = CGContext(data: pixels, width: canvas, height: canvas,
                                bitsPerComponent: 8, bytesPerRow: canvas,
                                space: colorSpace, bitmapInfo: CGImageAlphaInfo.none.rawValue) else {
    fatalError("could not create glyph bitmap context")
  }
  context.setShouldAntialias(true)
  context.setAllowsAntialiasing(true)
  context.setFillColor(gray: 0, alpha: 1)
  let position = CGPoint(x: (CGFloat(canvas) - bounds.width) / 2 - bounds.minX,
                         y: (CGFloat(canvas) - bounds.height) / 2 - bounds.minY)
  var drawGlyph = glyph
  CTFontDrawGlyphs(font, &drawGlyph, [position], 1, context)

  for row in 0..<canvas {
    for byteIndex in 0..<glyphRowBytes {
      var packed: UInt8 = 0
      for bit in 0..<8 {
        let column = byteIndex * 8 + bit
        if pixels[(canvas - 1 - row) * canvas + column] < 160 {
          packed |= 0x80 >> bit
        }
      }
      output += "\(packed),"
    }
  }
  if index % 500 == 0 { fputs("glyph \(index)/\(codepoints.count)\n", stderr) }
}

output += "};\n}\n"
try output.write(to: outputURL, atomically: true, encoding: .utf8)
fputs("generated \(codepoints.count) glyphs; missing \(missing)\n", stderr)
