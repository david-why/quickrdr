<script setup lang="ts">
import QUICKRDRExec from '@/assets/QUICKRDR.8xp?inline'
import { convertTextToQuickRDR } from '@/convert/convert'
import { font } from '@/convert/font'
import { convertDataToAppVars } from '@/convert/ti'
import { delay } from '@/utils'
import JSZip from 'jszip'
import { ref } from 'vue'
import FileUploader from '../FileUploader.vue'
import MetadataForm from '../MetadataForm.vue'

const file = ref<Blob | null>(null)
const title = ref('')

const isConverting = ref(false)
const isFinished = ref(false)

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
  isConverting.value = true
  await delay(100)
  try {
    const appVars = convertDataToAppVars(data, baseName)
    const zip = new JSZip()
    const folder = zip.folder(baseName)!
    for (let i = 0; i < appVars.length; i++) {
      const appVar = appVars[i]
      const fileName = baseName + i.toString().padStart(2, '0') + '.8xv'
      folder.file(fileName, appVar)
    }
    // folder.file('raw.bin', data)
    folder.file('QUICKRDR.8xp', QUICKRDRExec.split(',')[1], { base64: true })
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
    isFinished.value = true
  } finally {
    isConverting.value = false
  }
}
</script>

<template>
  <main :class="{ converting: isConverting }">
    <div class="container">
      <p>
        QuickRDR is an E-book reader for your TI-84+ CE calculator. You can read any book you'd
        like, as long as it fits in the calculator's storage limit (about 2MB).
      </p>
      <p>
        This website lets you convert your E-book to a program on your calculator to read it. Follow
        the steps below to get started!
      </p>
    </div>
    <div class="container">
      <h2>Step 1. Upload a text E-Book file</h2>
      <FileUploader @uploaded="file = $event" />
    </div>
    <div class="container" v-if="file">
      <h2>Step 2. Enter metadata</h2>
      <MetadataForm v-model:title="title" />
    </div>
    <div class="container" v-if="file && title">
      <h2>Step 3. Convert!</h2>
      <p>This may take a while and appear to hang - that's fine. Just hang tight!</p>
      <p><button class="convert-button" @click="convertFile">Convert</button></p>
    </div>
    <div class="container" v-if="isFinished">
      <h2>Step 4. Send to calculator &amp; Enjoy!</h2>
      <p>
        Unzip the downloaded file and send ALL files inside to your calculator, using the
        <a
          href="https://education.ti.com/en/products/computer-software/ti-connect-ce-sw"
          target="_blank"
          >TI Connect™ CE software</a
        >
        or some other method. (If you already have prgmQUICKRDR on your calculator, you can skip
        that one!)
      </p>
    </div>
  </main>
</template>

<style scoped>
.converting {
  cursor: progress;
}
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
