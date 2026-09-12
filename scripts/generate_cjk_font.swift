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

let font = CTFontCreateWithName("HYWenHei-65W" as CFString, 16, nil)
let colorSpace = CGColorSpaceCreateDeviceGray()
var output = "#pragma once\n#include <cstdint>\nnamespace spring::ui::detail {\n"
output += "inline constexpr std::uint32_t cjk_codepoints[] = {"
output += codepoints.map(String.init).joined(separator: ",")
output += "};\ninline constexpr std::uint8_t cjk_glyphs[] = {"

var missing = 0
for (index, codepoint) in codepoints.enumerated() {
  var character = UniChar(codepoint)
  var glyph = CGGlyph()
  guard CTFontGetGlyphsForCharacters(font, &character, &glyph, 1), glyph != 0 else {
    missing += 1
    output += Array(repeating: "0", count: 32).joined(separator: ",") + ","
    continue
  }

  var bounds = CGRect.zero
  CTFontGetBoundingRectsForGlyphs(font, .horizontal, &glyph, &bounds, 1)
  let canvas = 16
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
  context.setShouldAntialias(false)
  context.setAllowsAntialiasing(false)
  context.setFillColor(gray: 0, alpha: 1)
  let position = CGPoint(x: (CGFloat(canvas) - bounds.width) / 2 - bounds.minX,
                         y: (CGFloat(canvas) - bounds.height) / 2 - bounds.minY)
  var drawGlyph = glyph
  CTFontDrawGlyphs(font, &drawGlyph, [position], 1, context)

  for row in 0..<canvas {
    var first: UInt8 = 0
    var second: UInt8 = 0
    for column in 0..<canvas {
      if pixels[(canvas - 1 - row) * canvas + column] < 128 {
        if column < 8 { first |= 0x80 >> column }
        else { second |= 0x80 >> (column - 8) }
      }
    }
    output += "\(first),\(second),"
  }
  if index % 500 == 0 { fputs("glyph \(index)/\(codepoints.count)\n", stderr) }
}

output += "};\n}\n"
try output.write(to: outputURL, atomically: true, encoding: .utf8)
fputs("generated \(codepoints.count) glyphs; missing \(missing)\n", stderr)
