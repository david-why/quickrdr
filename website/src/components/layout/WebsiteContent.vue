<script setup lang="ts">
import { ref } from 'vue'
import FileUploader from '../FileUploader.vue'
import MetadataForm from '../MetadataForm.vue'
import { convertTextToQuickRDR } from '@/convert/convert'
import { font } from '@/convert/font'
import { convertDataToAppVars } from '@/convert/ti'
import JSZip from 'jszip'

const file = ref<File | null>(null)

const title = ref('')

async function convertFile() {
  if (!file.value) {
    console.error('No file selected')
    return
  }
  const text = await file.value.text()
  const data = await convertTextToQuickRDR({
    text,
    title: title.value,
    font: font,
    lineSpacing: 2,
  })
  let baseName = Math.random().toString(36).substring(2, 8).toUpperCase()
  if (baseName.charCodeAt(0) < 65) {
    baseName = 'A' + baseName.substring(1)
  }
  const appVars = convertDataToAppVars(data, baseName)
  const zip = new JSZip()
  for (let i = 0; i < appVars.length; i++) {
    const appVar = appVars[i]
    const fileName = baseName + i.toString().padStart(2, '0') + '.8xv'
    zip.file(fileName, appVar)
  }
  const zipFileName = baseName + '.zip'
  const zipFile = await zip.generateAsync({ type: 'blob' })
  const blob = new Blob([zipFile], { type: 'application/zip' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = zipFileName
  a.style.display = 'none'
  document.body.appendChild(a)
  a.click()
  document.body.removeChild(a)
  URL.revokeObjectURL(url)
}

async function makeTestFile() {
  const testFile = convertDataToAppVars(new Uint8Array([0x62]), 'TEST')
  const blob = new Blob([testFile[0]], { type: 'application/octet-stream' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = 'test.8xv'
  a.style.display = 'none'
  document.body.appendChild(a)
  a.click()
  document.body.removeChild(a)
  URL.revokeObjectURL(url)
}
</script>

<template>
  <main>
    <div class="container">
      <!-- <p></p> -->
    <button class="convert-button" @click="makeTestFile">Test</button>
      <h2>Step 1. Upload a file</h2>
      <FileUploader @uploaded="file = $event" />
    </div>
    <div class="container" v-if="file">
      <h2>Step 2. Enter metadata</h2>
      <MetadataForm v-model:title="title" />
    </div>
    <div class="container" v-if="file && title">
      <h2>Step 3. Convert!</h2>
      <div><button class="convert-button" @click="convertFile">Convert</button></div>
    </div>
  </main>
</template>

<style scoped>
.convert-button {
  background-color: var(--color-bar-background);
  border: none;
  border-radius: 8px;
  color: white;
  padding: 15px 32px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
  margin: 4px 2px;
  cursor: pointer;
}
.container {
  display: flex;
  flex-direction: column;
  margin: 2rem auto;
  padding: 1rem 2rem;
  max-width: 800px;
  background-color: #fff; /* White content background */
  border-radius: 8px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
}
</style>
