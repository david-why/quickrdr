import type { Font, Glyph } from "./font"
import { partitionText } from "./partition"
import { QuickRDRFile, QuickRDRGlyph } from "./structs"

interface ConvertOptions {
  text: string
  title: string
  font: Font
  lineSpacing?: number
}

function calcMinExtensionByte(total: number): number {
  if (total <= 255) {
    return 0;
  }
  // (m-1)+(256-m)*256 >= total
  return Math.floor((65535 - total) / 255);
}

function getGlyphID(index: number, total: number): number {
  if (index < 0) {
    throw new Error('Index must be non-negative');
  }
  const minExtensionByte = calcMinExtensionByte(total)
  if (index + 1 < minExtensionByte) {
    return index + 1
  }
  const extension = (index + 1 - minExtensionByte) >> 8
  const offset = (index + 1 - minExtensionByte) & 0xFF
  const extensionByte = minExtensionByte + extension
  return (extensionByte << 8) | offset
}

export async function convertTextToQuickRDR(options: ConvertOptions): Promise<Uint8Array> {
  const { text, title, font } = options
  const lineSpacing = options.lineSpacing ?? 2
  const counter = new Map<string, number>()
  const glyphs = new Map<string, Glyph>()
  for (const char of text) {
    if (char == '\n') {
      continue
    }
    counter.set(char, (counter.get(char) || 0) + 1)
    if (!glyphs.has(char)) {
      const glyph = await font.getGlyph(char)
      if (glyph) {
        glyphs.set(char, glyph)
      }
    }
  }
  console.log('glyphs', glyphs)
  const totalGlyphs = glyphs.size
  const maxHeight = Math.max(...Array.from(glyphs.values()).map((glyph) => glyph.height))
  const lineHeight = maxHeight + lineSpacing
  const sortedChars = Array.from(glyphs.keys()).sort((a, b) => {
    const aCount = counter.get(a) || 0
    const bCount = counter.get(b) || 0
    if (aCount === bCount) {
      return a.localeCompare(b)
    }
    return bCount - aCount
  })
  const quickrdrGlyphs = []
  for (let i = 0; i < sortedChars.length; i++) {
    const char = sortedChars[i]
    const glyph = glyphs.get(char)!
    const id = getGlyphID(i, totalGlyphs)
    quickrdrGlyphs.push(QuickRDRGlyph.from(id, glyph))
  }
  console.log('quickrdrGlyphs', quickrdrGlyphs)
  const tokens = text.split('').map((char) => char == '\n' ? 0 : getGlyphID(sortedChars.indexOf(char), totalGlyphs))
  const linesPerPage = Math.floor(180 / lineHeight)
  const pages = partitionText({
    text: tokens,
    glyphs: quickrdrGlyphs,
    linesPerPage,
    maxWidth: 304,
  })
  const file = new QuickRDRFile(
    1,
    title,
    calcMinExtensionByte(quickrdrGlyphs.length),
    lineHeight,
    quickrdrGlyphs,
    pages
  )
  return file.asBuffer()
}
