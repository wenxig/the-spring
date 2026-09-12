import { defineConfig } from 'vite-plus'
import type { OxfmtConfig } from 'vite-plus/fmt'
import type { OxlintConfig } from 'vite-plus/lint'

import fmt from './.oxfmtrc.json' with { type: 'json' }
import lint from './.oxlintrc.json' with { type: 'json' }

export default defineConfig({
  staged: {
    '*': 'vp check --fix',
    '*.{ts,tsx,mts,js,jsx,mjs,vue,html,md,json,yaml,toml}': 'vp exec cspell --no-must-find-files',
  },
  fmt: fmt as OxfmtConfig,
  lint: lint as OxlintConfig,
  run: {
    cache: { tasks: true, scripts: false },
    tasks: {
      'font:generate': {
        command: 'sh scripts/generate_hywenhei_font.sh',
        input: [
          'scripts/generate_cjk_font.swift',
          'scripts/generate_hywenhei_font.sh',
          'assets/HYWenHei-65W-3.ttf',
          'packages/esp32p4/components/ui_core/cjk_font.inc',
        ],
        output: ['packages/esp32p4/components/ui_core/cjk_font.inc'],
      },
      'simulator:build': {
        command:
          'cmake -S packages/epaper-simulator -B packages/epaper-simulator/build -G Ninja && cmake --build packages/epaper-simulator/build',
        cache: false,
        dependsOn: ['font:generate'],
      },
      'simulator:test': {
        command: 'ctest --test-dir packages/epaper-simulator/build --output-on-failure',
        cache: false,
        dependsOn: ['simulator:build'],
      },
      'simulator:render': {
        command:
          'mkdir -p .artifacts/epaper && packages/epaper-simulator/build/epaper_simulator .artifacts/epaper/clock.pbm',
        cache: false,
        dependsOn: ['simulator:build'],
      },
      'simulator:render:fallback': {
        command:
          'mkdir -p .artifacts/epaper && packages/epaper-simulator/build/epaper_simulator .artifacts/epaper/clock-fallback.pbm fallback',
        cache: false,
        dependsOn: ['simulator:build'],
      },
      'simulator:patterns': {
        command:
          'mkdir -p .artifacts/epaper && packages/epaper-simulator/build/pattern_test .artifacts/epaper/pattern_',
        cache: false,
        dependsOn: ['simulator:build'],
      },
      'embedded:verify-buffer-only': {
        command: 'python3 packages/epaper-simulator/verify.py',
        cache: false,
        dependsOn: ['font:generate'],
      },
      'ui:xml-validate': {
        command: 'python3 scripts/validate_ui_xml.py',
        cache: false,
      },
      'embedded:verify': {
        command:
          "grep -Eq 'CONFIG_SPRING_DISPLAY_BUFFER_ONLY(=n| is not set)' packages/esp32p4/sdkconfig",
        cache: false,
        dependsOn: ['simulator:test', 'firmware:build'],
      },
      'firmware:build': {
        command: "zsh -lc 'source .tools/esp-idf/export.sh && idf.py -C packages/esp32p4 build'",
        cache: false,
        dependsOn: ['font:generate'],
      },
      'firmware:flash': {
        command:
          'zsh -lc \'source .tools/esp-idf/export.sh && idf.py -C packages/esp32p4 -p "${ESP32_PORT:-/dev/cu.usbmodem141101}" flash\'',
        cache: false,
        dependsOn: ['firmware:build'],
      },
      'firmware:monitor': {
        command:
          'zsh -lc \'source .tools/esp-idf/export.sh && idf.py -C packages/esp32p4 -p "${ESP32_PORT:-/dev/cu.usbmodem141101}" monitor\'',
        cache: false,
      },
      'board:capture': {
        command:
          'mkdir -p .artifacts/board && python3 packages/epaper-simulator/capture_frame.py .artifacts/board/frame.pbm --port "${ESP32_PORT:-/dev/cu.usbmodem141101}" --baud 115200 --timeout "${CAPTURE_TIMEOUT:-30}"',
        cache: false,
      },
    },
  },
  test: {
    clearMocks: true,
    restoreMocks: true,
    unstubEnvs: true,
    unstubGlobals: true,
    coverage: {
      provider: 'v8',
      reporter: ['text', 'json-summary', 'html', 'lcov'],
      reportsDirectory: './coverage',
      include: ['script/**/*.{ts,mts}', 'packages/**/*.{ts,tsx}'],
      exclude: [
        '**/*.{test,spec}.{ts,tsx,mts}',
        '**/*.d.ts',
        '**/*.types.ts',
        '**/{test,__tests__}/**',
      ],
      thresholds: { lines: 75, functions: 75, branches: 70, statements: 75 },
    },
    exclude: ['**/node_modules/**', '**/.git/**', '.agents/**'],
    projects: [{ test: { name: 'root', environment: 'node', include: ['script/**/*.test.ts'] } }],
  },
})
