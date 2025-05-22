<script setup lang="ts">
const title = defineModel<string>('title', { required: true })
</script>

<template>
  <div class="metadata-form">
    <div class="form-row">
      <label><span>Title</span> <input type="text" v-model="title" /></label>
    </div>
  </div>
</template>

<style scoped>
.metadata-form {
  display: flex;
  flex-direction: column;
  gap: 1rem;
}
.form-row label {
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}
.form-row label span {
  font-weight: bold;
}
.form-row label input {
  padding: 0.5rem;
  border: 1px solid #ccc;
  border-radius: 4px;
}
</style>
