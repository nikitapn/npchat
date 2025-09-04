import { defineConfig } from 'vite'
import { svelte } from '@sveltejs/vite-plugin-svelte'
import tailwindcss from '@tailwindcss/vite'
import path from 'path'
import fs from 'node:fs'
import { fileURLToPath } from 'node:url'
import type { IncomingMessage, ServerResponse } from 'node:http'

const __dirname = path.dirname(fileURLToPath(import.meta.url))

// https://vite.dev/config/
export default defineConfig({
  plugins: [
    svelte(),
    tailwindcss(),
    // Custom plugin to serve host.json
    {
      name: 'serve-host-json',
      configureServer(server) {
        server.middlewares.use('/host.json', (req: IncomingMessage, res: ServerResponse, next: () => void) => {
          if (req.method === 'GET') {
            const hostJsonPath = path.resolve(__dirname, 'dist/host.json')
            try {
              if (fs.existsSync(hostJsonPath)) {
                const content = fs.readFileSync(hostJsonPath, 'utf-8')
                res.setHeader('Content-Type', 'application/json')
                res.setHeader('Access-Control-Allow-Origin', '*')
                res.end(content)
              } else {
                res.statusCode = 404
                res.end('host.json not found')
              }
            } catch (error) {
              res.statusCode = 500
              res.end('Error reading host.json')
            }
          } else {
            next()
          }
        })
      }
    }
  ],
  build: {
    sourcemap: true, // Enable source maps for production builds
    minify: false,   // Disable minification for easier debugging (optional)
  },
  resolve: {
    alias: {
      '@lib': path.resolve(__dirname, 'src/lib'),
      'nprpc': path.resolve(__dirname, '../external/npsystem/nprpc/nprpc_js/dist/index.esm.js'),
    },
  },
  server: {
    host: '0.0.0.0',
    allowedHosts: ['localhost', 'archvm.lan'],
    https: {
      key: '/home/nikita/projects/social/certs/archvm.lan.key',
      cert: '/home/nikita/projects/social/certs/archvm.lan.crt',
      dhparam: '/home/nikita/projects/social/certs/dhparam.pem'
    }
  }
})
