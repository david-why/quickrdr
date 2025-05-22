<script setup lang="ts">
import WebsiteContent from './components/layout/WebsiteContent.vue';
import WebsiteHeader from './components/layout/WebsiteHeader.vue'
</script>

<template>
  <div class="app-wrapper">
    <WebsiteHeader />
    <WebsiteContent />
  </div>
</template>

<style scoped>
.app-wrapper {
  display: flex;
  flex-direction: column;
  min-height: 100vh;
}
</style>
