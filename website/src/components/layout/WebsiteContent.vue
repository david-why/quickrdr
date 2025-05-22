<script setup lang="ts">
import { ref } from 'vue'
import FileUploader from '../FileUploader.vue'
import MetadataForm from '../MetadataForm.vue'

const file = ref<File | null>(null)

const title = ref('')

async function convertFile() {}
</script>

<template>
  <main>
    <div class="container">
      <!-- <p></p> -->
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
