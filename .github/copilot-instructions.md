# NPChat WebRTC Project - AI Coding Assistant Instructions

## Project Architecture

This is a **hybrid C++/Svelte WebRTC chat application** with a comprehensive real-time chat system:

- **Frontend**: Svelte 5 + TypeScript + Tailwind CSS (client/src/)
- **Backend**: C++23 server with custom NPRPC framework (server/src/)
- **Database**: SQLite with comprehensive chat, user, and session management
- **IDL**: Custom `.npidl` files define RPC interfaces (idl/)
- **Build System**: Optimized hybrid CMake + Docker for C++, Vite for frontend

## Key Architectural Patterns

### NPRPC Framework Integration
- **Generated Code**: `client/src/rpc/npchat.ts` is auto-generated from `idl/npchat.npidl`
- **RPC Pattern**: Frontend makes typed RPC calls to C++ backend via `NPRPC.ObjectProxy`

### Authentication & Security System
- **Secure Sessions**: Cookie-based authentication with CSRF protection
- **Session Management**: `SecureSessionManager` class handles secure storage
- **Password Security**: SHA256 hashing with secure session ID generation (256-bit OpenSSL random)
- **RPC Initialization**: Proper timing controls with `isRPCAvailable()` checks

### Real-Time Chat Architecture
- **Global Chat Service**: `ChatService` manages all chat operations and real-time notifications
- **Observer Pattern**: `ChatObserver` system for multi-chat notifications
- **Database Services**: Separate services for contacts, messages, and chat management
- **Participant Management**: Full CRUD operations with authorization controls

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
# Configure the build system (run once)
./configure.sh

# Build everything (Frontend + Backend) -- default
./build.sh

# Build only the server target (skip client/Vite step)
./build.sh server

# Build only the generated client JS target (Vite/TS output)
./build.sh client

# Build and start the backend server (runs the server after build)
./build.sh run

# Build helper target to generate RPC stubs and copy them to the client source directory
./gen_stubs.sh

# Frontend development server (optimized build system)
# This is useful for hot-reloading during UI development
# and it seamlessly integrates with the C++ backend (make sure backend is running)
cd client && npm run dev

# Docker build (for production deployment)
./01_docker_build.sh
# Creating runtime package
./02_pack_runtime.sh
# Deployment to server (example)
./03_deploy.sh
Options for deploy.sh:
  -h <hostname>          - Hostname of the server
  -s -c -d               - Copy server, client, database
  -w /home/debian/npchat - Working directory on remote server
  <user>@<hostname>:/home/debian/temp/npchat - Temporary upload location
```

### Optimized Build System
- **Source Tracking**: CMake now properly tracks TypeScript, Svelte, CSS, and HTML files
- **Incremental Builds**: Only rebuilds when source files actually change
- **NPRPC Dependencies**: Automatic NPRPC JavaScript library building
- **VSCode Integration**: Maintains TypeScript configuration compatibility

### Project-Specific Development Patterns

#### Authentication Flow
```typescript
// Enhanced auth service with secure sessions
import { authService } from '../rpc/services/auth';
await authService.initialize(); // Proper RPC timing
const userData = await authService.logIn(username, password);
// Cookies automatically managed with CSRF protection
```

#### Chat Management
```typescript
// Global chat service for real-time features
import { chatService } from '../rpc/services/chat';

// Get user chats with full details
const chats = await registeredUser.GetChats(); // Returns ChatList with metadata

// Create private chat
const chatId = await registeredUser.CreateChatWith(userId);

// Remove participants (authorization enforced)
await registeredUser.LeaveChatParticipant(chatId, participantId);
```

#### Database Service Patterns
```cpp
// ChatService with comprehensive features
class ChatService {
  // Full chat details with participant count and last message time
  npchat::ChatList getUserChatsWithDetails(std::uint32_t user_id);
  
  // Participant management with authorization
  bool removeParticipant(std::uint32_t requesting_user_id, 
                        npchat::ChatId chat_id, 
                        std::uint32_t participant_id);
  
  // Automatic chat deletion when last participant leaves
  bool deleteChat(npchat::ChatId chat_id);
};
```

#### Secure Session Management
```cpp
// AuthService with 256-bit secure session IDs
static std::string generateSessionId() {
  unsigned char random_bytes[32]; // 256-bit entropy
  RAND_bytes(random_bytes, 32);   // OpenSSL cryptographic randomness
  // Convert to hex string (64 characters)
}
```

#### RPC Service Integration
```typescript
// Frontend RPC usage pattern with proper error handling
import { Server } from '../rpc/npchat';
const server = new Server(endpoint);

try {
  await server.LogIn(login, password, userRef);
} catch (e) {
  if (e instanceof npchat.AuthorizationFailed) {
    // Handle specific auth errors
  }
}
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
- **Exception Handling**: Custom exceptions like `AuthorizationFailed`, `ChatOperationFailed` bubble up from C++
- **Session Management**: Secure cookie-based authentication with automatic renewal

### Real-Time Chat System
- **Global Notifications**: `ChatObserver` system broadcasts to all participants
- **Multi-Chat Support**: Users can participate in multiple chats simultaneously
- **Unread Tracking**: Automatic message read/unread state management
- **Participant Management**: Creator permissions and authorization controls

### Security Architecture
- **Cookie-Based Sessions**: CSRF protection with secure session storage
- **Input Sanitization**: Server-side validation and client-side escaping
- **Authorization**: Method-level permissions (e.g., only chat creator can remove participants)
- **Secure Random**: 256-bit cryptographically secure session IDs

### WebRTC Signaling Flow
1. `createPeerConnection()` sets up RTCPeerConnection with TURN server
2. Manual JSON exchange for offers/answers (no WebSocket signaling yet)
3. ICE candidates logged to console, included in localDescription updates

### Build Dependencies
- **Docker**: `cpp-dev-env:latest` container for C++ compilation
- **Boost**: Required for C++ backend (`/usr/local/lib` in container)
- **NPSystem**: Custom framework in `external/npsystem/`
- **NPRPC JavaScript**: Auto-built from `external/npsystem/nprpc/nprpc_js/`

## File Relationships

- `idl/npchat.npidl` → generates → `client/src/rpc/npchat.ts`
- `client/src/nprpc.ts` → exports → `external/npsystem/nprpc/nprpc_js/dist`
- `server/src/services/` → implements → IDL-defined interfaces
- `client/src/App.svelte` → imports → `WebRTCCall.svelte` for main UI
- `server/src/services/db/ChatService.hpp` → provides → participant management and chat operations
- `client/src/lib/rpc/services/auth.ts` → manages → secure authentication and session handling

## Key Components Status

### Backend Services (C++)
- **AuthService**: ✅ Secure session management with 256-bit random IDs
- **ChatService**: ✅ Full participant management, chat deletion, authorization
- **ContactService**: ✅ User search and contact management
- **MessageService**: ✅ Message operations and unread tracking
- **ChatObserver**: ✅ Real-time notifications across multiple chats

### Frontend Services (TypeScript)
- **authService**: ✅ Cookie-based authentication with CSRF protection
- **chatService**: ✅ Global chat state management and real-time updates
- **session.ts**: ✅ Security utilities and session validation

### UI Components (Svelte 5)
- **ContactList**: ✅ Search and chat creation functionality
- **ChatList**: ✅ Real-time chat display with unread counts
- **ChatApp**: ✅ Integrated chat interface with participant management
- **WebRTCCall**: ✅ Video calling with manual signaling

## Testing Patterns

### Chat System Testing
- Test participant removal: Creator can remove others, users can leave
- Test chat deletion: Automatic when last participant leaves
- Test real-time updates: Messages appear across all participants
- Test authorization: Only creators can remove participants

### Authentication Testing
- Test secure session creation and validation
- Test auto-login with cookie persistence
- Test CSRF protection and input sanitization
- Test RPC timing with proper initialization

### WebRTC Testing
- Open `/webrtc` route in two browser tabs
- Use "Start Call" → copy offer → "Join Call" → copy answer flow
- Check browser console for ICE candidate logs and connection state

### Development Debugging
- Frontend: Browser DevTools + Svelte DevTools extension
- Backend: spdlog output, compile with `-DCMAKE_BUILD_TYPE=Debug`
- RPC: Network tab shows NPRPC binary protocol exchanges
- Database: SQLite browser for direct database inspection

### Build System Optimization
- **Incremental Builds**: Only rebuilds when source files change
- **Dependency Tracking**: GLOB patterns track `.ts`, `.svelte`, `.js`, `.css`, `.html` files
- **NPRPC Integration**: Automatic JavaScript library building
- **Development Mode**: `npm run dev` for hot reloading with Vite
