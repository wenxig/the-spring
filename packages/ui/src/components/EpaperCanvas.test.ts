import { shallowMount } from '@vue/test-utils'
import { describe, expect, it } from 'vitest'
import EpaperCanvas from './EpaperCanvas.vue'

describe('EpaperCanvas', () => {
  it('keeps the physical 4:3 frame and deterministic content', () => {
    const wrapper = shallowMount(EpaperCanvas)
    expect(wrapper.find('[data-testid="epaper"]').classes()).toEqual(['epaper-frame'])
    expect(wrapper.text()).toContain('08:24')
  })
  it('renders the alternate state', () => {
    const wrapper = shallowMount(EpaperCanvas, { props: { inverted: true } })
    expect(wrapper.find('[data-testid="epaper"]').classes()).toContain('inverted')
  })
})
