// Renders SVGs with AppKit (CoreSVG) into PNGs, and composes contact sheets, for Frigate's artwork
// (port/assets/branding): render.py builds it with swiftc and renders the app icons' 1024 px masters and the logos'
// PNGs with it, and the contact sheet that contact.py lays out. macOS only.
//
// usage: sheet SPEC OUT.png
// SPEC lines (top-left origin, pixels):
//   canvas W H [RRGGBB|none]
//   rect X Y W H RRGGBB
//   img PATH X Y W H [MAG]   the SVG fitted into W x H; with MAG, rendered at W/MAG x H/MAG and enlarged unsmoothed
//   text X Y SIZE RRGGBB WORDS...
import AppKit

func hex(_ s: String) -> NSColor {
  var v: UInt64 = 0
  Scanner(string: s).scanHexInt64(&v)
  return NSColor(srgbRed: CGFloat((v >> 16) & 255) / 255, green: CGFloat((v >> 8) & 255) / 255,
                 blue: CGFloat(v & 255) / 255, alpha: 1)
}

func bitmap(_ w: Int, _ h: Int) -> NSBitmapImageRep {
  let rep = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: w, pixelsHigh: h, bitsPerSample: 8,
                             samplesPerPixel: 4, hasAlpha: true, isPlanar: false, colorSpaceName: .deviceRGB,
                             bytesPerRow: 0, bitsPerPixel: 0)!.retagging(with: .sRGB)!
  rep.size = NSSize(width: w, height: h)
  return rep
}

// The SVG fitted (aspect kept, centred) into a transparent w x h bitmap.
func render(_ path: String, _ w: Int, _ h: Int) -> NSBitmapImageRep {
  guard let img = NSImage(contentsOfFile: path) else { fatalError("cannot load \(path)") }
  let s = min(CGFloat(w) / img.size.width, CGFloat(h) / img.size.height)
  let dw = img.size.width * s, dh = img.size.height * s
  let rep = bitmap(w, h)
  NSGraphicsContext.saveGraphicsState()
  NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: rep)
  img.draw(in: NSRect(x: (CGFloat(w) - dw) / 2, y: (CGFloat(h) - dh) / 2, width: dw, height: dh))
  NSGraphicsContext.restoreGraphicsState()
  return rep
}

let args = CommandLine.arguments
let spec = try! String(contentsOfFile: args[1], encoding: .utf8)
var canvas: NSBitmapImageRep! = nil
var H = 0
for raw in spec.split(separator: "\n") {
  let f = raw.split(separator: " ").map(String.init)
  if f.isEmpty || f[0].hasPrefix("#") { continue }
  switch f[0] {
  case "canvas":
    canvas = bitmap(Int(f[1])!, Int(f[2])!)
    H = Int(f[2])!
    NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: canvas)
    if f.count > 3 && f[3] != "none" {
      hex(f[3]).setFill()
      NSRect(x: 0, y: 0, width: Int(f[1])!, height: H).fill()
    }
  case "rect":
    let x = Int(f[1])!, y = Int(f[2])!, w = Int(f[3])!, h = Int(f[4])!
    hex(f[5]).setFill()
    NSRect(x: x, y: H - y - h, width: w, height: h).fill()
  case "img":
    let x = Int(f[2])!, y = Int(f[3])!, w = Int(f[4])!, h = Int(f[5])!
    let mag = f.count > 6 ? Int(f[6])! : 1
    let r = render(f[1], w / mag, h / mag)
    NSGraphicsContext.current!.imageInterpolation = mag > 1 ? .none : .high
    r.draw(in: NSRect(x: x, y: H - y - h, width: w, height: h), from: .zero, operation: .sourceOver, fraction: 1,
           respectFlipped: false, hints: [.interpolation: NSNumber(value: (mag > 1 ? NSImageInterpolation.none : .high).rawValue)])
  case "text":
    let x = Int(f[1])!, y = Int(f[2])!, size = CGFloat(Int(f[3])!)
    let str = f[5...].joined(separator: " ")
    let attrs: [NSAttributedString.Key: Any] = [.font: NSFont.systemFont(ofSize: size), .foregroundColor: hex(f[4])]
    (str as NSString).draw(at: NSPoint(x: CGFloat(x), y: CGFloat(H - y) - size * 1.2), withAttributes: attrs)
  default:
    fatalError("unknown spec line: \(raw)")
  }
}
try! canvas.representation(using: .png, properties: [:])!.write(to: URL(fileURLWithPath: args[2]))
