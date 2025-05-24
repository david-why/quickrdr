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
    const total_size = 34 + 3 * page_count + font_glyph_count * font_glyph_size + this.pages.reduce((acc, page) => acc + page.length, 0)
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
    dataView.setUint16(29, font_glyph_size, true)
    dataView.setUint32(31, page_count, true)
    let offset = 34
    let pageOffset = 34 + 3 * page_count + font_glyph_count * font_glyph_size
    for (let i = 0; i < page_count; i++) {
      dataView.setUint32(offset, pageOffset, true)
      offset += 3
      pageOffset += this.pages[i].length
    }
    for (let i = 0; i < font_glyph_count; i++) {
      console.log('glyph', this.glyphs[i], 'data length', this.glyphs[i].data.length, 'font_glyph_size', font_glyph_size)
      const glyph = this.glyphs[i]
      dataView.setUint16(offset, glyph.id, true)
      buffer[offset + 2] = glyph.width
      buffer[offset + 3] = glyph.height
      for (let j = 0; j < glyph.data.length; j++) {
        buffer[offset + 4 + j] = glyph.data[j]
      }
      for (let j = glyph.data.length; j < font_glyph_size - 4; j++) {
        buffer[offset + 4 + j] = 0
      }
      offset += font_glyph_size
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
