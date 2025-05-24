import { setBit } from "@/utils"

export declare interface Font {
  getGlyph(text: string): Promise<Glyph | null>
}

export declare interface Glyph {
  width: number
  height: number
  data: Uint8Array // 1 bit per pixel
}

export class TtfFont implements Font {
  public family: string
  private font: FontFace
  private glyphs: Map<string, Glyph | null>
  private canvas: OffscreenCanvas | HTMLCanvasElement
  public fontLoaded: Promise<any>

  constructor(fontUrl: any, private fontSize: number, canvas?: OffscreenCanvas | HTMLCanvasElement) {
    this.family = Math.random().toString(36).substring(2, 15)
    this.font = new FontFace(this.family, `url(${fontUrl})`, {
      style: 'normal',
      weight: 'normal',
      stretch: 'normal',
    })
    this.glyphs = new Map()
    this.canvas = canvas || new OffscreenCanvas(0, 0)
    document.fonts.add(this.font)
    this.fontLoaded = this.font.load()
  }

  async getGlyph(text: string): Promise<Glyph | null> {
    if (this.glyphs.has(text)) {
      return this.glyphs.get(text) || null
    }
    await this.fontLoaded
    const ctx = this.canvas.getContext('2d') as OffscreenCanvasRenderingContext2D | CanvasRenderingContext2D
    if (!ctx) {
      throw new Error('Failed to get 2D context')
    }
    ctx.font = `${this.fontSize}px ${this.family}`
    console.log('font', ctx.font)
    const metrics = ctx.measureText(text)
    const width = Math.ceil(metrics.actualBoundingBoxRight + metrics.actualBoundingBoxLeft)
    const height = Math.ceil(metrics.fontBoundingBoxAscent + metrics.actualBoundingBoxDescent)
    this.canvas.width = width
    this.canvas.height = height
    ctx.clearRect(0, 0, width, height)
    ctx.fillStyle = 'white'
    ctx.fillRect(0, 0, width, height)
    ctx.fillStyle = 'black'
    ctx.textBaseline = 'top'
    ctx.font = `${this.fontSize}px ${this.family}`
    ctx.fillText(text, 0, 0)
    const imageData = ctx.getImageData(0, 0, width, height)
    const data = new Uint8Array(Math.ceil(width * height / 8))
    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const bitIndex = y * width + x
        const isBlack = imageData.data[(y * width + x) * 4] < 128
        setBit(data, bitIndex, isBlack)
      }
    }
    // TODO delete me
    // const debugCanvas = document.createElement('canvas')
    // const debugCtx = debugCanvas.getContext('2d')
    // if (!debugCtx) {
    //   throw new Error('Failed to get 2D context')
    // }
    // debugCanvas.width = width
    // debugCanvas.height = height
    // debugCtx.putImageData(imageData, 0, 0)
    // document.body.appendChild(debugCanvas)
    const glyph: Glyph = {
      width,
      height,
      data,
    }
    this.glyphs.set(text, glyph)
    return glyph
  }
}

export const font = new TtfFont('/font.ttf', 16)
// export const font = new TtfFont('/font.ttf', 16)
