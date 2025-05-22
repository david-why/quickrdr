import type { Glyph } from "./font"

export class QuickRDRFile {
  constructor(
    public version: number,
    public name: string,
    public min_extension_byte: number,
    public line_height: number,
    public glyphs: QuickRDRGlyph[],
    public pages: Uint8Array[]
  ) { }

  asBuffer(): Uint8Array {
    const magic = 0x51524452 // 'QRDR'
    const font_glyph_count = this.glyphs.length
    const font_glyph_size = Math.max(...this.glyphs.map((glyph) => glyph.data.length)) + 4
    const page_count = this.pages.length
    const total_size = 44 + 3 * page_count + font_glyph_count * font_glyph_size + this.pages.reduce((acc, page) => acc + page.length, 0)
    const buffer = new Uint8Array(total_size)
    const dataView = new DataView(buffer.buffer)
    dataView.setUint32(0, magic)
    buffer[4] = this.version
    for (let i = 0; i < 15; i++) {
      if (i < this.name.length) {
        buffer[5 + i] = this.name.charCodeAt(i)
      } else {
        buffer[5 + i] = 0
      }
    }
    buffer[20] = 0
    dataView.setUint32(21, total_size, true)
    buffer[24] = this.min_extension_byte
    buffer[25] = this.line_height
    dataView.setUint32(26, font_glyph_count, true)
    buffer[29] = font_glyph_size
    dataView.setUint32(30, page_count, true)
    let offset = 33
    for (let i = 0; i < page_count; i++) {
      dataView.setUint32(offset, this.pages[i].length, true)
      offset += 3
    }
    for (let i = 0; i < font_glyph_count; i++) {
      const glyph = this.glyphs[i]
      dataView.setUint16(offset, glyph.id, true)
      offset += 2
      buffer[offset] = glyph.width
      offset += 1
      buffer[offset] = glyph.height
      offset += 1
      for (let j = 0; j < glyph.data.length; j++) {
        buffer[offset + j] = glyph.data[j]
      }
      offset += glyph.data.length
    }
    for (let i = 0; i < page_count; i++) {
      const page = this.pages[i]
      for (let j = 0; j < page.length; j++) {
        buffer[offset + j] = page[j]
      }
      offset += page.length
    }
    return buffer
  }
}

export class QuickRDRGlyph {
  constructor(
    public id: number,
    public width: number,
    public height: number,
    public data: Uint8Array
  ) { }

  static from(id: number, glyph: Glyph): QuickRDRGlyph {
    return new QuickRDRGlyph(
      id,
      glyph.width,
      glyph.height,
      glyph.data
    )
  }
}
