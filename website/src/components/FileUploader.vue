<script setup lang="ts">
import { ref } from 'vue'

const fileInput = ref<HTMLInputElement>()
const isDragOver = ref(false)
const uploadedFile = ref<File | null>(null)

const emit = defineEmits<{
  uploaded: [File]
}>()

function handleDrop(event: DragEvent) {
  isDragOver.value = false
  const files = event.dataTransfer?.files
  if (files && files.length > 0) {
    const file = files[0]
    if (file.type === 'text/plain') {
      uploadedFile.value = file
      emit('uploaded', file)
    } else {
      alert('Please upload a .TXT file.')
    }
  }
}

function handleFileChange() {
  const files = fileInput.value?.files
  if (files && files.length > 0) {
    const file = files[0]
    if (file.type === 'text/plain') {
      uploadedFile.value = file
      emit('uploaded', file)
    } else {
      alert('Please upload a .TXT file.')
    }
  }
}
</script>

<template>
  <div class="file-uploader">
    <div
      class="drop-zone"
      :class="{ 'drag-over': isDragOver }"
      @dragover.prevent="isDragOver = true"
      @dragenter.prevent="isDragOver = true"
      @dragleave.prevent="isDragOver = false"
      @drop.prevent="handleDrop"
    >
      <label class="file-label">
        <input
          type="file"
          @change="handleFileChange"
          accept=".txt"
          class="file-input"
          ref="fileInput"
        />
        <p>Upload your .TXT ebook file:</p>
        <p>📄 Drag and drop or click to browse</p>
      </label>
    </div>
    <p v-if="uploadedFile">File uploaded: {{ uploadedFile.name }}</p>
  </div>
</template>

<style scoped>
.file-uploader {
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  padding: 1.5rem 2rem;
  background: var(--color-background);
  color: var(--color-text);
}
.file-input {
  display: none;
}
.drop-zone {
  width: 100%;
  height: 200px;
  border: 2px dashed var(--color-bar-background);
  border-radius: 8px;
  display: flex;
  justify-content: center;
  align-items: center;
  transition: background-color 0.3s ease;
}
.drop-zone.drag-over {
  background-color: var(--color-bar-background);
}
.file-label {
  text-align: center;
}
</style>
