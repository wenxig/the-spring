<script setup lang="ts">
withDefaults(defineProps<{ inverted?: boolean }>(), { inverted: false })
</script>

<template><div class="epaper-frame" :class="{ inverted }" data-testid="epaper"><div class="epaper-screen"><span class="screen-label">THE SPRING</span><strong class="screen-time">08:24</strong><span class="screen-date">2026 / 09 / 11  ·  上海</span><span class="screen-status">NETWORK  OK</span></div></div></template>
