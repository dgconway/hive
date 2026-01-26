import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    proxy: {
      '/api': {
        target: 'http://127.0.0.1:8080',
        changeOrigin: true,
        rewrite: (path) => path.replace(/^\/api/, ''),
      },
      // Also proxy /games directly if needed, or better use /api prefix in frontend calls.
      // But python API routes are at root /games. 
      // So if frontend calls /api/games -> backend /games. Correct.
    },
  },
})
