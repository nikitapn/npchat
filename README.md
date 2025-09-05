# NPChat — Hybrid C++/Svelte WebRTC Chat

A real-time chat and video-calling application with a C++23 backend and a Svelte 5 + TypeScript frontend. It uses a custom NPRPC framework for typed RPC between client and server, SQLite for persistence, and WebRTC for peer-to-peer media.

## Features
- Secure login with cookie-based sessions and CSRF protection
- Chats, contacts, messages, unread tracking
- Real-time notifications via observer pattern
- WebRTC video calls with trickle ICE and TURN support
- Custom IDL (.npidl) → generated client/server RPC stubs
- Optimized hybrid build (CMake/Docker for C++, Vite for client)

## Tech Stack
- Backend: C++23, CMake, SQLite, OpenSSL, spdlog
- Frontend: Svelte 5, TypeScript, Vite, Tailwind CSS
- RPC: NPRPC (custom), IDL in `idl/`
- Media: WebRTC (STUN/TURN)

## Repository Layout
- `server/` — C++ server and services
- `client/` — Svelte 5 frontend
- `idl/` — NPRPC IDL files (generate TS/CPP bindings)
- `external/` — NPSystem/NPRPC sources
- `build.sh`, `configure.sh` — Build scripts (hybrid)
- `gen_stubs.sh` — Generate RPC stubs and copy to client

## Quick Start
Prerequisites
- Linux host (tested), C++ toolchain, CMake
- Node.js 18+ and npm
- OpenSSL (for TLS if using HTTPS/WSS)

Setup
- Configure once
  - `./configure.sh`
- Build all (server + client)
  - `./build.sh`
- Run backend server
  - `./build.sh run`
- Frontend dev server (in another terminal)
  - `cd client && npm run dev`

The client dev server runs with HTTPS (see `client/vite.config.ts`) and proxies RPC/WebSocket to the C++ backend.

## Environment Variables
Server (`.env` in repo root)
- `USE_SSL=1` — enable TLS (default in this repo)
- `BUILD_TYPE=Debug|Release`

Client (`client/.env.*` for Vite)
- Preferred (JSON array of ICE servers)
  - `VITE_WEBRTC_ICE_SERVERS='[{"urls":"stun:stun.l.google.com:19302"},{"urls":"turn:example.com:3478","username":"user","credential":"pass"}]'`
- Fallback (individual vars)
  - `VITE_WEBRTC_STUN_URLS=stun:stun.l.google.com:19302,stun:stun1.l.google.com:19302`
  - `VITE_WEBRTC_TURN_URL=turn:example.com:3478`
  - `VITE_WEBRTC_TURN_USERNAME=user`
  - `VITE_WEBRTC_TURN_CREDENTIAL=pass`

Notes
- Vite only exposes variables prefixed with `VITE_` to client code.
- See `client/.env.example` for examples.

## Development Workflow
- Generate RPC stubs after updating `idl/npchat.npidl`:
  - `./gen_stubs.sh`
- Incremental rebuilds:
  - `./build.sh server` (backend only)
  - `./build.sh client` (frontend bundle only)
- Hot reload for UI:
  - `cd client && npm run dev`

## WebRTC Testing
- Open the app in two browser tabs or devices
- Start a call from a chat using the “Call” button
- ICE trickling and end-of-candidates are handled automatically
- Ensure your ICE/TURN config is set via Vite env for NAT traversal

## Deployment
- Docker build and packaging scripts included:
  - `./01_docker_build.sh` — Build production images
  - `./02_pack_runtime.sh` — Create runtime package
  - `./03_deploy.sh` — Deploy to remote host

Adjust server certificates and hostnames in `client/vite.config.ts` and server config as needed.

## Security
- Cookie-based sessions with CSRF protection
- 256-bit cryptographically secure session IDs (OpenSSL)
- Authorization checks for chat operations (creator permissions)

## License
This project is licensed under the MIT License. See [LICENSE](./LICENSE).

## Acknowledgements
- NPSystem / NPRPC framework (in `external/`)
- spdlog, Tailwind CSS, Svelte, Vite