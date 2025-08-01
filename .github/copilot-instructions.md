# NPChat WebRTC Project - AI Coding Assistant Instructions

## Project Architecture

This is a **hybrid C++/Svelte WebRTC chat application** with a unique RPC-based architecture:

- **Frontend**: Svelte 5 + TypeScript + Tailwind CSS (client/src/)
- **Backend**: C++23 server with custom NPRPC framework (server/src/)
- **IDL**: Custom `.npidl` files define RPC interfaces (idl/)
- **Build System**: Hybrid CMake + Docker for C++, Vite for frontend

## Key Architectural Patterns

### NPRPC Framework Integration
- **Generated Code**: `client/src/rpc/npchat.ts` is auto-generated from `idl/npchat.npidl`
- **RPC Pattern**: Frontend makes typed RPC calls to C++ backend via `NPRPC.ObjectProxy`
- **Never edit**: Files in `client/src/rpc/` - regenerate via IDL compiler instead

### WebRTC Implementation
- **Manual Signaling**: Current implementation uses copy-paste JSON exchange for testing
- **State Management**: Uses Svelte 5's `$state()` runes throughout `WebRTCCall.svelte`

### Svelte 5 Conventions
- **Runes**: Use `$state()`, `$derived()`, `$effect()` - avoid legacy reactive statements
- **Components**: Single-file components in `client/src/lib/components/`
- **Styling**: Tailwind with custom forms plugin, responsive-first approach

## Critical Developer Workflows

### Building the Application
```bash
# Full C++ backend build (requires Docker)
./01_build_app.sh

# Package runtime dependencies
./02_pack_runtime.sh

# Frontend development server
cd client && npm run dev
```

### Project-Specific Development Patterns

#### RPC Service Integration
```typescript
// Frontend RPC usage pattern
import { Server } from '../rpc/npchat';
const server = new Server(endpoint);
await server.LogIn(login, password, userRef);
```

#### Svelte 5 State Management
```svelte
<!-- Use $state() for reactive variables -->
let isCallActive = $state(false);
let connectionStatus = $state('Not connected');

<!-- Event handlers with arrow functions -->
<button onclick={() => startCall()}>Start Call</button>
```

## Integration Points

### Frontend-Backend Communication
- **RPC Endpoint**: Frontend connects to C++ server via WebSocket/HTTP
- **Object Proxies**: Backend services exposed as `NPRPC.ObjectProxy` instances
- **Exception Handling**: Custom exceptions like `AuthorizationFailed` bubble up from C++

### WebRTC Signaling Flow
1. `createPeerConnection()` sets up RTCPeerConnection with TURN server
2. Manual JSON exchange for offers/answers (no WebSocket signaling yet)
3. ICE candidates logged to console, included in localDescription updates

### Build Dependencies
- **Docker**: `cpp-dev-env:latest` container for C++ compilation
- **Boost**: Required for C++ backend (`/usr/local/lib` in container)
- **NPSystem**: Custom framework in `external/npsystem/`

## File Relationships

- `idl/npchat.npidl` → generates → `client/src/rpc/npchat.ts`
- `client/src/nprpc.ts` → exports → `external/npsystem/nprpc/nprpc_js/dist`
- `server/src/services/` → implements → IDL-defined interfaces
- `client/src/App.svelte` → imports → `WebRTCCall.svelte` for main UI

## Testing Patterns

### WebRTC Testing
- Open `/webrtc` route in two browser tabs
- Use "Start Call" → copy offer → "Join Call" → copy answer flow
- Check browser console for ICE candidate logs and connection state

### Development Debugging
- Frontend: Browser DevTools + Svelte DevTools extension
- Backend: spdlog output, compile with `-DCMAKE_BUILD_TYPE=Debug`
- RPC: Network tab shows NPRPC binary protocol exchanges
