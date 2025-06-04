export const MAX_APPVAR_SIZE = 65460

export function convertDataToAppVars(data: Uint8Array, name: string): Uint8Array[] {
  if (name.length > 6) {
    throw new Error('Name length exceeds 6 characters')
  }
  const sections = Math.ceil(data.byteLength / MAX_APPVAR_SIZE)
  const files = []
  for (let i = 0; i < sections; i++) {
    const start = i * MAX_APPVAR_SIZE
    const end = Math.min(start + MAX_APPVAR_SIZE, data.byteLength)
    const sectionData = data.slice(start, end)
    const fileName = name + i.toString().padStart(2, '0')
    const entry = convertDataToAppVarEntry(sectionData, fileName)

    const buffer = new Uint8Array(55 + entry.byteLength + 2)
    const dataView = new DataView(buffer.buffer)
    buffer.set([42, 42, 84, 73, 56, 51, 70, 42, 26, 10, 10])
    for (let i = 0; i < 42; i++) {
      buffer[11 + i] = 0
    }
    dataView.setUint16(53, entry.byteLength, true)
    buffer.set(entry, 55)
    let checksum = 0
    for (let i = 0; i < entry.byteLength; i++) {
      checksum += entry[i]
    }
    checksum = checksum & 0xffff
    dataView.setUint16(55 + entry.byteLength, checksum, true)
    files.push(buffer)
  }
  return files
}

export function convertDataToAppVarEntry(data: Uint8Array, name: string): Uint8Array {
  if (data.byteLength > MAX_APPVAR_SIZE) {
    throw new Error('Data size exceeds maximum appvar size of ' + MAX_APPVAR_SIZE)
  }
  if (name.length > 8) {
    throw new Error('Name length exceeds 8 characters')
  }
  const buffer = new Uint8Array(19 + data.byteLength)
  const dataView = new DataView(buffer.buffer)
  buffer.set([13, 0], 0)
  dataView.setUint16(2, data.byteLength + 2, true)
  buffer[4] = 21
  for (let i = 0; i < 8; i++) {
    buffer[5 + i] = name.charCodeAt(i) || 0
  }
  buffer.set([0, 128], 13)
  dataView.setUint16(15, data.byteLength + 2, true)
  dataView.setUint16(17, data.byteLength, true)
  buffer.set(data, 19)
  return buffer
}
