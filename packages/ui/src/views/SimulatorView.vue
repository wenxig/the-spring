<script setup lang="ts">
import { ref } from 'vue'
import EpaperCanvas from '../components/EpaperCanvas.vue'
const inverted = ref(false)
</script>

<template><section class="page"><div><p class="eyebrow">RENDER CHECK</p><h1>模拟渲染验证</h1><p class="muted">固定画布尺寸与状态输入，可用于截图校准。</p><button @click="inverted = !inverted">切换状态</button></div><EpaperCanvas :inverted="inverted" /></section></template>
