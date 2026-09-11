import { createApp } from 'vue'
import { createRouter, createWebHistory } from 'vue-router'
import App from './App.vue'
import './styles.css'
import ClockView from './views/ClockView.vue'
import SimulatorView from './views/SimulatorView.vue'

const router = createRouter({
  history: createWebHistory(),
  routes: [
    { path: '/', redirect: '/clock' },
    { path: '/clock', component: ClockView, meta: { title: '时钟' } },
    { path: '/simulator', component: SimulatorView, meta: { title: '模拟渲染' } },
    { path: '/:pathMatch(.*)*', redirect: '/clock' },
  ],
})

createApp(App).use(router).mount('#app')
