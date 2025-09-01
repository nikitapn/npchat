<script lang="ts">
  import { onMount } from 'svelte';

  // WebRTC configuration with your STUN server
  let rtcConfig = $state({
    iceServers: [
      { urls: 'stun:stun.l.google.com:19302' }, // Fallback Google STUN
      // Add your VPS STUN server here - replace with your server details
    ]
  });

  // Video elements and state
  let localVideo: HTMLVideoElement;
  let remoteVideo: HTMLVideoElement;
  let localStream: MediaStream | null = $state(null);
  let remoteStream: MediaStream | null = $state(null);
  let peerConnection: RTCPeerConnection | null = $state(null);

  // UI state
  let isCallActive = $state(false);
  let isConnecting = $state(false);
  let connectionStatus = $state('Not connected');
  let localOffer = $state('');
  let remoteAnswer = $state('');
  let remoteOfferInput = $state('');
  let localAnswerInput = $state('');
  let showAdvanced = $state(false);

  // Custom STUN server input
  let customStunServer = $state('');

  onMount(() => {
    // Request permissions on component mount
    requestMediaPermissions();
  });

  async function requestMediaPermissions() {
    try {
      localStream = await navigator.mediaDevices.getUserMedia({
        video: true,
        audio: true
      });
      if (localVideo) {
        localVideo.srcObject = localStream;
      }
      connectionStatus = 'Media ready - Click "Start Call" or "Join Call"';
    } catch (error) {
      console.error('Error accessing media devices:', error);
      connectionStatus = 'Error: Could not access camera/microphone';
    }
  }

  function addCustomStunServer() {
    if (customStunServer.trim()) {
      rtcConfig.iceServers.push({ urls: `stun:${customStunServer.trim()}` });
      customStunServer = '';
      connectionStatus = `Added STUN server: ${customStunServer}`;
    }
  }

  async function createPeerConnection() {
    peerConnection = new RTCPeerConnection(rtcConfig);

    // Add local stream to peer connection
    if (localStream) {
      localStream.getTracks().forEach(track => {
        peerConnection?.addTrack(track, localStream!);
      });
    }

    // Handle remote stream
    peerConnection.ontrack = (event) => {
      remoteStream = event.streams[0];
      if (remoteVideo) {
        remoteVideo.srcObject = remoteStream;
      }
    };

    // Handle ICE candidates
    peerConnection.onicecandidate = (event) => {
      if (event.candidate) {
        console.log('New ICE candidate:', event.candidate);
        localOffer = JSON.stringify(peerConnection!.localDescription, null, 2);
      }
    };

    // Connection state changes
    peerConnection.onconnectionstatechange = () => {
      connectionStatus = `Connection: ${peerConnection?.connectionState}`;
      if (peerConnection?.connectionState === 'connected') {
        isCallActive = true;
        isConnecting = false;
      }
    };

    return peerConnection;
  }

  async function startCall() {
    try {
      isConnecting = true;
      connectionStatus = 'Creating offer...';

      await createPeerConnection();
      const offer = await peerConnection!.createOffer();
      await peerConnection!.setLocalDescription(offer);

      localOffer = JSON.stringify(offer, null, 2);
      connectionStatus = 'Offer created! Share the offer with the other person.';
    } catch (error) {
      console.error('Error starting call:', error);
      connectionStatus = 'Error starting call';
      isConnecting = false;
    }
  }

  async function joinCall() {
    try {
      if (!remoteOfferInput.trim()) {
        connectionStatus = 'Please paste the offer from the other person';
        return;
      }

      isConnecting = true;
      connectionStatus = 'Processing offer...';

      await createPeerConnection();
      const offer = JSON.parse(remoteOfferInput);
      await peerConnection!.setRemoteDescription(offer);

      const answer = await peerConnection!.createAnswer();
      await peerConnection!.setLocalDescription(answer);

      localAnswerInput = JSON.stringify(answer, null, 2);
      connectionStatus = 'Answer created! Share the answer back.';
    } catch (error) {
      console.error('Error joining call:', error);
      connectionStatus = 'Error joining call - check the offer format';
      isConnecting = false;
    }
  }

  async function acceptAnswer() {
    try {
      if (!remoteAnswer.trim()) {
        connectionStatus = 'Please paste the answer from the other person';
        return;
      }

      const answer = JSON.parse(remoteAnswer);
      await peerConnection!.setRemoteDescription(answer);
      connectionStatus = 'Answer processed! Connection should establish soon.';
    } catch (error) {
      console.error('Error accepting answer:', error);
      connectionStatus = 'Error accepting answer - check the format';
    }
  }

  function endCall() {
    if (peerConnection) {
      peerConnection.close();
      peerConnection = null;
    }
    if (remoteVideo) {
      remoteVideo.srcObject = null;
    }
    remoteStream = null;
    isCallActive = false;
    isConnecting = false;
    localOffer = '';
    remoteAnswer = '';
    remoteOfferInput = '';
    localAnswerInput = '';
    connectionStatus = 'Call ended';
  }

  function toggleMute() {
    if (localStream) {
      const audioTrack = localStream.getAudioTracks()[0];
      if (audioTrack) {
        audioTrack.enabled = !audioTrack.enabled;
      }
    }
  }

  function toggleVideo() {
    if (localStream) {
      const videoTrack = localStream.getVideoTracks()[0];
      if (videoTrack) {
        videoTrack.enabled = !videoTrack.enabled;
      }
    }
  }

  function copyToClipboard(text: string) {
    navigator.clipboard.writeText(text);
    connectionStatus = 'Copied to clipboard!';
  }
</script>

<div class="min-h-screen bg-gray-50 p-4">
  <div class="max-w-6xl mx-auto">
    <h1 class="text-3xl font-bold text-center mb-8">WebRTC Video Call Test</h1>

    <!-- Connection Status -->
    <div class="mb-6 p-4 bg-white rounded-lg shadow">
      <h2 class="text-lg font-semibold mb-2">Connection Status</h2>
      <p class="text-sm text-gray-600">{connectionStatus}</p>
    </div>

    <!-- Custom STUN Server Configuration -->
    <div class="mb-6 p-4 bg-white rounded-lg shadow">
      <button
        onclick={() => showAdvanced = !showAdvanced}
        class="text-lg font-semibold mb-2 text-indigo-600 hover:text-indigo-800"
      >
        STUN Server Configuration {showAdvanced ? '▼' : '▶'}
      </button>

      {#if showAdvanced}
        <div class="mt-4 space-y-4">
          <div>
            <label class="block text-sm font-medium text-gray-700 mb-2">
              Add Custom STUN Server (your VPS):
            </label>
            <div class="flex gap-2">
              <input
                bind:value={customStunServer}
                placeholder="your-vps-ip:3478"
                class="flex-1 px-3 py-2 border border-gray-300 rounded-md focus:ring-indigo-500 focus:border-indigo-500"
              />
              <button
                onclick={addCustomStunServer}
                class="px-4 py-2 bg-indigo-600 text-white rounded-md hover:bg-indigo-700"
              >
                Add
              </button>
            </div>
          </div>

          <div>
            <label class="block text-sm font-medium text-gray-700 mb-2">Current STUN Servers:</label>
            <div class="text-sm text-gray-600">
              {#each rtcConfig.iceServers as server}
                <div class="bg-gray-100 p-2 rounded mb-1">{server.urls}</div>
              {/each}
            </div>
          </div>
        </div>
      {/if}
    </div>

    <!-- Video Grid -->
    <div class="grid grid-cols-1 lg:grid-cols-2 gap-6 mb-6">
      <!-- Local Video -->
      <div class="bg-white rounded-lg shadow p-4">
        <h3 class="text-lg font-semibold mb-4">Your Video</h3>
        <video
          bind:this={localVideo}
          autoplay
          muted
          playsinline
          class="w-full h-64 bg-gray-900 rounded-lg object-cover"
        ></video>
      </div>

      <!-- Remote Video -->
      <div class="bg-white rounded-lg shadow p-4">
        <h3 class="text-lg font-semibold mb-4">Remote Video</h3>
        <video
          bind:this={remoteVideo}
          autoplay
          playsinline
          class="w-full h-64 bg-gray-900 rounded-lg object-cover"
        ></video>
      </div>
    </div>

    <!-- Call Controls -->
    <div class="bg-white rounded-lg shadow p-6 mb-6">
      <h3 class="text-lg font-semibold mb-4">Call Controls</h3>
      <div class="flex flex-wrap gap-4 justify-center">
        {#if !isCallActive && !isConnecting}
          <button
            onclick={startCall}
            class="px-6 py-2 bg-green-600 text-white rounded-lg hover:bg-green-700 transition-colors"
          >
            Start Call (Create Offer)
          </button>
          <button
            onclick={joinCall}
            class="px-6 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 transition-colors"
          >
            Join Call (Answer Offer)
          </button>
        {:else if isCallActive}
          <button
            onclick={toggleMute}
            class="px-6 py-2 bg-yellow-600 text-white rounded-lg hover:bg-yellow-700 transition-colors"
          >
            Toggle Mute
          </button>
          <button
            onclick={toggleVideo}
            class="px-6 py-2 bg-purple-600 text-white rounded-lg hover:bg-purple-700 transition-colors"
          >
            Toggle Video
          </button>
          <button
            onclick={endCall}
            class="px-6 py-2 bg-red-600 text-white rounded-lg hover:bg-red-700 transition-colors"
          >
            End Call
          </button>
        {:else}
          <div class="text-center">
            <div class="animate-spin w-8 h-8 border-4 border-blue-500 border-t-transparent rounded-full mx-auto mb-2"></div>
            <p>Connecting...</p>
          </div>
        {/if}
      </div>
    </div>

    <!-- Signaling Exchange -->
    <div class="grid grid-cols-1 lg:grid-cols-2 gap-6">
      <!-- Offer/Answer Display -->
      <div class="bg-white rounded-lg shadow p-6">
        <h3 class="text-lg font-semibold mb-4">Your Offer/Answer</h3>
        {#if localOffer}
          <div class="mb-4">
            <label class="block text-sm font-medium text-gray-700 mb-2">
              Share this offer with the other person:
            </label>
            <textarea
              readonly
              value={localOffer}
              class="w-full h-32 p-2 border border-gray-300 rounded text-xs font-mono"
            ></textarea>
            <button
              onclick={() => copyToClipboard(localOffer)}
              class="mt-2 px-4 py-2 bg-indigo-600 text-white rounded hover:bg-indigo-700"
            >
              Copy Offer
            </button>
          </div>
        {/if}

        {#if localAnswerInput}
          <div>
            <label class="block text-sm font-medium text-gray-700 mb-2">
              Share this answer back:
            </label>
            <textarea
              readonly
              value={localAnswerInput}
              class="w-full h-32 p-2 border border-gray-300 rounded text-xs font-mono"
            ></textarea>
            <button
              onclick={() => copyToClipboard(localAnswerInput)}
              class="mt-2 px-4 py-2 bg-indigo-600 text-white rounded hover:bg-indigo-700"
            >
              Copy Answer
            </button>
          </div>
        {/if}
      </div>

      <!-- Remote Input -->
      <div class="bg-white rounded-lg shadow p-6">
        <h3 class="text-lg font-semibold mb-4">Paste from Other Person</h3>

        {#if !localOffer}
          <div class="mb-4">
            <label class="block text-sm font-medium text-gray-700 mb-2">
              Paste the offer here:
            </label>
            <textarea
              bind:value={remoteOfferInput}
              placeholder="Paste the JSON offer here..."
              class="w-full h-32 p-2 border border-gray-300 rounded text-xs font-mono"
            ></textarea>
          </div>
        {:else}
          <div>
            <label class="block text-sm font-medium text-gray-700 mb-2">
              Paste the answer here:
            </label>
            <textarea
              bind:value={remoteAnswer}
              placeholder="Paste the JSON answer here..."
              class="w-full h-32 p-2 border border-gray-300 rounded text-xs font-mono"
            ></textarea>
            <button
              onclick={acceptAnswer}
              class="mt-2 px-4 py-2 bg-green-600 text-white rounded hover:bg-green-700"
            >
              Accept Answer
            </button>
          </div>
        {/if}
      </div>
    </div>

    <!-- Instructions -->
    <div class="mt-6 bg-blue-50 border border-blue-200 rounded-lg p-4">
      <h4 class="font-semibold text-blue-900 mb-2">Quick Test Instructions:</h4>
      <ol class="text-sm text-blue-800 space-y-1">
        <li>1. <strong>Person A:</strong> Click "Start Call" to create an offer</li>
        <li>2. <strong>Person A:</strong> Copy and share the offer with Person B</li>
        <li>3. <strong>Person B:</strong> Paste the offer and click "Join Call"</li>
        <li>4. <strong>Person B:</strong> Copy and share the answer back to Person A</li>
        <li>5. <strong>Person A:</strong> Paste the answer and click "Accept Answer"</li>
        <li>6. Both should see each other's video once connected!</li>
      </ol>
    </div>
  </div>
</div>
