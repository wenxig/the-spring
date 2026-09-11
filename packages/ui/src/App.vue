<script setup lang="ts">
import { RouterLink, RouterView } from 'vue-router'
</script>

<template>
  <main class="app-shell">
    <header class="topbar"><strong>the-spring</strong><nav><RouterLink to="/clock">时钟</RouterLink><RouterLink to="/simulator">模拟渲染</RouterLink></nav></header>
    <RouterView />
  </main>
</template>
