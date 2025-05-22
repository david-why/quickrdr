import type { QuickRDRGlyph } from "./structs";

export function partitionText(
  options: {
    text: number[]
    glyphs: QuickRDRGlyph[]
    linesPerPage: number
    maxWidth: number
  },
): Uint8Array[] {
  const { text, glyphs, linesPerPage, maxWidth } = options
  const lines: number[][] = []
  let currentLine: number[] = []
  let currentLineWidth = 0
  const insertLineBreak = () => {
    lines.push(currentLine)
    currentLine = []
    currentLineWidth = 0
  }
  for (const glyphId of text) {
    if (glyphId === 0) {
      insertLineBreak()
      continue
    }
    const glyph = glyphs.find((g) => g.id === glyphId)
    if (!glyph) {
      console.warn(`Glyph with ID ${glyphId} not found`)
      continue
    }
    if (currentLineWidth + glyph.width > maxWidth) {
      insertLineBreak()
    }
    const charWidth = glyph.width
    currentLine.push(glyphId)
    currentLineWidth += charWidth
  }
  if (currentLine.length > 0) {
    lines.push([...currentLine])
  }
  const pages: number[][][] = []
  let currentPage: number[][] = []
  for (const line of lines) {
    if (currentPage.length >= linesPerPage) {
      pages.push(currentPage)
      currentPage = []
    }
    currentPage.push(line)
  }
  if (currentPage.length > 0) {
    pages.push(currentPage)
  }
  const result: Uint8Array[] = []
  for (const page of pages) {
    const flatPage = []
    for (const line of page) {
      for (const glyphId of line) {
        if (glyphId > 0xFF) {
          const extension = (glyphId >> 8) & 0xFF
          const offset = glyphId & 0xFF
          flatPage.push(extension)
          flatPage.push(offset)
        } else {
          flatPage.push(glyphId)
        }
      }
      flatPage.push(0)
    }
    result.push(new Uint8Array(flatPage.slice(0, flatPage.length - 1)))
  }
  return result
}
