export function setBit(data: Uint8Array, index: number, value: boolean): void {
  const byteIndex = Math.floor(index / 8);
  const bitIndex = index % 8;
  if (value) {
    data[byteIndex] |= (1 << (7 - bitIndex));
  } else {
    data[byteIndex] &= ~(1 << (7 - bitIndex));
  }
}

export function getBit(data: Uint8Array, index: number): boolean {
  const byteIndex = Math.floor(index / 8);
  const bitIndex = index % 8;
  return (data[byteIndex] & (1 << (7 - bitIndex))) !== 0;
}
