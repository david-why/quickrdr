export declare interface Font {
  getGlyph(text: string): Promise<Glyph | null>
}

export declare interface Glyph {
  width: number
  height: number
  data: Uint8Array // 1 bit per pixel
}
