<script lang="ts">
  import type { ChatId, MessageId, ChatAttachment, ChatAttachmentType } from '../rpc/npchat';
  import { authService } from '../rpc/services/Auth';
  import { chatService } from '../rpc/services/Chat.svelte';
  import VideoPlayer from './VideoPlayer.svelte';
  import { isVideoFile } from '../utils/shakaLoader';

  interface ChatRoomProps {
    currentChatId: ChatId;
  }

  let { currentChatId }: ChatRoomProps = $props();

  interface Message {
    id: MessageId;
    text: string;
    timestamp: Date;
    sender: string;
    attachment?: ChatAttachment;
  }

  interface MessageGroup {
    sender: string;
    timestamp: Date;
    messages: Message[];
    isOwnMessage: boolean;
  }

  let messages: Message[] = $state([]);
  let messageGroups: MessageGroup[] = $state([]);
  let newMessage = $state('');
  let selectedFile: File | null = $state(null);
  let fileInputRef: HTMLInputElement;
  let messagesContainer: HTMLDivElement;

  // WebRTC call state
  let showCallModal = $state(false);
  let isCallActive = $state(false);
  let isCallConnecting = $state(false);
  let callStatus = $state('Not connected');
  let currentCallId = $state<string | null>(null);
  let localVideo: HTMLVideoElement | undefined = $state();
  let remoteVideo: HTMLVideoElement | undefined = $state();
  let localStream: MediaStream | null = $state(null);
  let remoteStream: MediaStream | null = $state(null);
  let peerConnection: RTCPeerConnection | null = $state(null);

  // WebRTC configuration
  const rtcConfig = {
    iceServers: [
      {
        urls: 'stun:stun.l.google.com:19302'
      }
    ]
  };

  // Request media permissions for camera and microphone
  async function requestMediaPermissions() {
    try {
      localStream = await navigator.mediaDevices.getUserMedia({
        video: { facingMode: 'user' },
        audio: true
      });
      // Attach after element is bound
      queueMicrotask(() => {
        if (localVideo && localStream) {
          localVideo.srcObject = localStream;
          // Some browsers require an explicit play()
          localVideo.play().catch(() => {/* ignore autoplay restrictions */});
        }
      });
      callStatus = 'Media ready - Click "Start Call"';
    } catch (error) {
      console.error('Error accessing media devices:', error);
      callStatus = 'Error: Could not access camera/microphone';
    }
  }

  // Create and configure the RTCPeerConnection
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
      queueMicrotask(() => {
        if (remoteVideo && remoteStream) {
          remoteVideo.srcObject = remoteStream;
          remoteVideo.play().catch(() => {/* may be blocked until user gesture */});
        }
      });
    };

    // Handle ICE candidates (trickling ICE)
    peerConnection.onicecandidate = async (event) => {
      if (event.candidate && currentCallId) {
        try {
          await chatService.sendIceCandidate(currentCallId, JSON.stringify(event.candidate));
        } catch (error) {
          console.error('Error sending ICE candidate:', error);
        }
      }
    };

    // Handle ICE gathering state changes
    peerConnection.onicegatheringstatechange = () => {
      console.log('ICE gathering state:', peerConnection?.iceGatheringState);
      if (peerConnection?.iceGatheringState === 'complete' && currentCallId) {
        console.log('All ICE candidates gathered');
        // Send a special "end-of-candidates" signal
        chatService.sendIceCandidate(currentCallId, JSON.stringify({ candidate: null }))
          .catch(error => console.error('Error sending end-of-candidates:', error));
      }
    };

    // Connection state changes
    peerConnection.onconnectionstatechange = () => {
      callStatus = `Connection: ${peerConnection?.connectionState}`;
      if (peerConnection?.connectionState === 'connected') {
        // Connected; update flags without blocking UI with alert dialogs
        isCallActive = true;
        isCallConnecting = false;
      }
    };

    return peerConnection;
  }

  async function startCall() {
    try {
      isCallConnecting = true;
      callStatus = 'Requesting media permissions...';

      await requestMediaPermissions();
      await createPeerConnection();

      callStatus = 'Creating offer...';
      const offer = await peerConnection!.createOffer();
      await peerConnection!.setLocalDescription(offer);

      callStatus = 'Sending call invitation...';
      currentCallId = await chatService.initiateCall(currentChatId, JSON.stringify(offer));

      showCallModal = true;
      callStatus = 'Call initiated! Waiting for answer...';
    } catch (error: any) {
      console.error('Error starting call:', error);
      callStatus = 'Error starting call';
      isCallConnecting = false;

      if (error.name === 'ChatOperationFailed') {
        alert('Cannot start call: ' + error.message);
      } else {
        alert('Failed to start call: ' + error.message);
      }
    }
  }

  async function startVideoCall() {
    try {
      isCallConnecting = true;
      callStatus = 'Requesting media permissions...';

      await requestMediaPermissions();
      await createPeerConnection();

      callStatus = 'Creating offer...';
      const offer = await peerConnection!.createOffer();
      await peerConnection!.setLocalDescription(offer);

      callStatus = 'Sending call invitation...';
      currentCallId = await chatService.initiateCall(currentChatId, JSON.stringify(offer));

      showCallModal = true;
      callStatus = 'Call initiated! Waiting for answer...';
    } catch (error: any) {
      console.error('Error starting call:', error);
      callStatus = 'Error starting call';
      isCallConnecting = false;

      if (error.name === 'ChatOperationFailed') {
        alert('Cannot start call: ' + error.message);
      } else {
        alert('Failed to start call: ' + error.message);
      }
    }
  }

  async function answerCall(callId: string, offer: string) {
    try {
      isCallConnecting = true;
      callStatus = 'Requesting media permissions...';

      await requestMediaPermissions();
      await createPeerConnection();

      currentCallId = callId;
      callStatus = 'Processing offer...';

      const offerObj = JSON.parse(offer);
      await peerConnection!.setRemoteDescription(offerObj);

      const answer = await peerConnection!.createAnswer();
      await peerConnection!.setLocalDescription(answer);

      callStatus = 'Sending answer...';
      await chatService.answerCall(callId, JSON.stringify(answer));

      showCallModal = true;
      callStatus = 'Answer sent! Establishing connection...';
    } catch (error: any) {
      console.error('Error answering call:', error);
      callStatus = 'Error answering call';
      isCallConnecting = false;
      alert('Failed to answer call: ' + error.message);
    }
  }

  function endCall() {
    if (peerConnection && currentCallId) {
      chatService.endCall(currentCallId).catch(error => {
        console.error('Error ending call:', error);
      });
    }

    cleanupCall();
  }

  function cleanupCall() {
    if (peerConnection) {
      peerConnection.close();
      peerConnection = null;
    }
    if (remoteVideo) {
      remoteVideo.srcObject = null;
    }
    if (localStream) {
      localStream.getTracks().forEach(track => track.stop());
      localStream = null;
    }
    remoteStream = null;
    isCallActive = false;
    isCallConnecting = false;
    currentCallId = null;
    callStatus = 'Call ended';
    showCallModal = false;
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

  // Group messages by sender and time proximity
  function groupMessages(msgs: Message[]): MessageGroup[] {
    if (msgs.length === 0) return [];

    const groups: MessageGroup[] = [];
    const TIME_THRESHOLD = 5 * 60 * 1000; // 5 minutes in milliseconds

    let currentGroup: MessageGroup | null = null;

    for (const message of msgs) {
      const isOwnMessage = message.sender === authService.authState.userData?.name || message.sender === 'You';

      // Check if we should start a new group
      const shouldStartNewGroup = !currentGroup ||
        currentGroup.sender !== message.sender ||
        (message.timestamp.getTime() - currentGroup.timestamp.getTime()) > TIME_THRESHOLD;

      if (shouldStartNewGroup) {
        // Start a new group
        currentGroup = {
          sender: message.sender,
          timestamp: message.timestamp,
          messages: [message],
          isOwnMessage
        };
        groups.push(currentGroup);
      } else {
        // Add to existing group
        currentGroup!.messages.push(message);
      }
    }

    console.log('Grouped messages:', groups);

    return groups;
  }

  // Update message groups when messages change
  $effect(() => {
    messageGroups = groupMessages(messages);
  });

  // Auto-scroll to bottom when new messages arrive
  function scrollToBottom() {
    if (messagesContainer) {
      messagesContainer.scrollTop = messagesContainer.scrollHeight;
    }
  }

  // Effect to scroll to bottom when messages change
  $effect(() => {
    // Watch messageGroups for changes
    if (messageGroups.length > 0) {
      // Use setTimeout to ensure DOM has updated
      setTimeout(scrollToBottom, 50);
    }
  });

  // Utility functions for attachment handling
  function detectAttachmentType(file: File): ChatAttachmentType {
    const imageTypes = ['image/jpeg', 'image/png', 'image/gif', 'image/webp', 'image/svg+xml'];
    const videoTypes = ['video/mp4', 'video/webm', 'video/ogg', 'video/avi', 'video/mov', 'video/wmv', 'video/flv', 'video/mkv'];

    if (imageTypes.includes(file.type)) {
      return 0; // Picture
    } else if (videoTypes.includes(file.type)) {
      return 2; // Video
    } else {
      return 1; // File
    }
  }

  function isImageAttachment(attachment: ChatAttachment): boolean {
    return attachment.type === 0; // Picture
  }

  function isVideoAttachment(attachment: ChatAttachment): boolean {
    return attachment.type === 2; // Video
  }

  function createImageUrl(data: Uint8Array): string {
    // Cast ArrayBufferLike -> ArrayBuffer to satisfy TS lib definitions
    const blob = new Blob([data.buffer as ArrayBuffer], { type: 'image/jpeg' }); // Assume JPEG for now
    return URL.createObjectURL(blob);
  }

  function formatFileSize(bytes: number): string {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
  }

  // Helpers for safe attachment actions
  function openImage(att?: ChatAttachment) {
    if (!att) return;
    const url = createImageUrl(att.data);
    window.open(url, '_blank');
  }

  function downloadAttachment(att?: ChatAttachment) {
    if (!att) return;
    const blob = new Blob([att.data.buffer as ArrayBuffer], { type: 'application/octet-stream' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = att.name;
    a.click();
    URL.revokeObjectURL(url);
  }

  // File handling functions
  function handleFileSelect(event: Event) {
    const input = event.target as HTMLInputElement;
    if (input.files && input.files[0]) {
      selectedFile = input.files[0];
    }
  }

  function clearSelectedFile() {
    selectedFile = null;
    if (fileInputRef) {
      fileInputRef.value = '';
    }
  }

  async function fileToUint8Array(file: File): Promise<Uint8Array> {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = () => {
        if (reader.result instanceof ArrayBuffer) {
          resolve(new Uint8Array(reader.result));
        } else {
          reject(new Error('Failed to read file'));
        }
      };
      reader.onerror = () => reject(reader.error);
      reader.readAsArrayBuffer(file);
    });
  }

  // Convert chat messages to UI messages
  async function updateMessages() {
    const chatMessages = chatService.getMessagesForChat(currentChatId);
    const currentUserId = authService.authState.userData?.userId;

    messages = await Promise.all(chatMessages.map(async (message) => {
      let senderName = 'Unknown User';

      if (message.senderId === currentUserId) {
        senderName = authService.authState.userData?.name || 'You';
      } else {
        // Try to get sender name from cache or fetch from server
        const contact = await chatService.getContactById(message.senderId);
        if (contact) {
          senderName = contact.username;
        }
      }

      return {
        id: message.messageId || message.timestamp, // Use messageId if available, fallback to timestamp
        text: message.content.text,
        timestamp: new Date(message.timestamp * 1000), // Convert Unix timestamp to Date
        sender: senderName,
        attachment: message.content.attachment
      };
    }));
  }

  // Set this chat as active and load messages
  async function activateChat() {
    await chatService.setActiveChatId(currentChatId);
    await updateMessages();
  }

  // Load more history for pagination
  async function loadMoreHistory() {
    const canLoadMore = await chatService.loadMoreHistory(currentChatId);
    if (canLoadMore) {
      await updateMessages();
    }
    return canLoadMore;
  }

  // Check if we're near the top and should load more history
  function handleScroll(event: Event) {
    const container = event.target as HTMLElement;
    const scrollTop = container.scrollTop;
    const scrollThreshold = 100; // pixels from top

    if (scrollTop <= scrollThreshold &&
      chatService.hasMoreHistory(currentChatId) &&
      !chatService.isChatHistoryLoading(currentChatId)) {

      // Store current scroll position to maintain scroll after loading
      const prevScrollHeight = container.scrollHeight;

      loadMoreHistory().then((loaded) => {
        if (loaded) {
          // Maintain scroll position after new messages are added
          setTimeout(() => {
            const newScrollHeight = container.scrollHeight;
            container.scrollTop = newScrollHeight - prevScrollHeight + scrollTop;
          }, 0);
        }
      });
    }
  }

  // Send a message
  async function sendMessage() {
    if (!newMessage.trim() && !selectedFile) return;

    try {
      let attachment: ChatAttachment | undefined = undefined;

      // Handle file attachment
      if (selectedFile) {
        const fileData = await fileToUint8Array(selectedFile);
        attachment = {
          type: detectAttachmentType(selectedFile),
          name: selectedFile.name,
          data: fileData
        };
      }

      const messageId = await chatService.sendMessage(currentChatId, newMessage.trim(), attachment);

      // Add message to local list immediately for immediate feedback
      const localMessage: Message = {
        id: messageId,
        text: newMessage.trim(),
        timestamp: new Date(),
        sender: authService.authState.userData?.name || 'You',
        attachment: attachment
      };
      messages.push(localMessage);
      messages = [...messages]; // Trigger reactivity

      newMessage = '';
      clearSelectedFile();
      console.log('Message sent with ID:', messageId);
    } catch (error: any) {
      console.error('Failed to send message:', error);

      // Handle specific chat errors
      if (error.name === 'ChatOperationFailed') {
        switch (error.reason) {
          case 1: // UserNotParticipant
            alert('You are not a participant in this chat. Please create a new chat.');
            break;
          case 0: // ChatNotFound
            alert('Chat not found. Please create a new chat.');
            break;
          default:
            alert('Failed to send message: ' + error.message);
        }
      } else {
        alert('Failed to send message: ' + error.message);
      }
    }
  }

  // Handle Enter key in input
  function handleKeydown(event: KeyboardEvent) {
    if (event.key === 'Enter' && !event.shiftKey) {
      event.preventDefault();
      sendMessage();
    }
  }

  // Initialize chat when component mounts
  activateChat();

  // Effect to activate chat when currentChatId changes
  $effect(() => {
    if (currentChatId) {
      // Clear messages when switching chats to avoid stale data
      messages = [];
      activateChat();
    }
  });

  // Subscribe to new messages for this chat
  const unsubscribeNewMessage = chatService.onNewMessage(async (notification) => {
    if (notification.chatId === currentChatId) {
      // Update messages from the service (which now includes history)
      await updateMessages();
    }
  });

  // Subscribe to history loaded events
  const unsubscribeHistoryLoaded = chatService.onHistoryLoaded(async (chatId, history) => {
    if (chatId === currentChatId) {
      await updateMessages();
    }
  });

  // Subscribe to WebRTC call events
  const unsubscribeCallInitiated = chatService.onCallInitiated(async (callId, chatId, callerId, offer) => {
    if (chatId === currentChatId) {
      // Show incoming call notification
      const shouldAnswer = confirm('Incoming video call! Do you want to answer?');
      if (shouldAnswer) {
        await answerCall(callId, offer);
      } else {
        // Optionally decline the call
        try {
          await chatService.endCall(callId);
        } catch (error) {
          console.error('Error declining call:', error);
        }
      }
    }
  });

  const unsubscribeCallAnswered = chatService.onCallAnswered(async (callId, answer) => {
    if (callId === currentCallId && peerConnection) {
      try {
        const answerObj = JSON.parse(answer);
        await peerConnection.setRemoteDescription(answerObj);
        callStatus = 'Answer received! Establishing connection...';
      } catch (error) {
        console.error('Error processing answer:', error);
        callStatus = 'Error processing answer';
      }
    }
  });

  const unsubscribeIceCandidate = chatService.onIceCandidate(async (callId, candidate) => {
    if (callId === currentCallId && peerConnection) {
      try {
        const candidateObj = JSON.parse(candidate);
        if (candidateObj.candidate === null) {
          console.log('Received end-of-candidates signal');
          // End-of-candidates received, no need to add null candidate
          return;
        }
        await peerConnection.addIceCandidate(candidateObj);
      } catch (error) {
        console.error('Error adding ICE candidate:', error);
      }
    }
  });

  const unsubscribeCallEnded = chatService.onCallEnded(async (callId, reason) => {
    if (callId === currentCallId) {
      callStatus = `Call ended: ${reason}`;
      cleanupCall();
    }
  });

  // Cleanup when component is destroyed
  $effect(() => {
    return () => {
      unsubscribeNewMessage();
      unsubscribeHistoryLoaded();
      unsubscribeCallInitiated();
      unsubscribeCallAnswered();
      unsubscribeIceCandidate();
      unsubscribeCallEnded();
      // Don't set activeChatId to null here - let the parent component handle it
    };
  });

  // Ensure video elements get streams once modal is visible
  $effect(() => {
    if (showCallModal) {
      if (localVideo && localStream) {
        localVideo.srcObject = localStream;
        // Best-effort play; may be blocked without gesture, but Start/Answer are gestures
        // @ts-ignore - play exists on HTMLMediaElement
        localVideo.play?.().catch(() => {});
      }
      if (remoteVideo && remoteStream) {
        remoteVideo.srcObject = remoteStream;
        // @ts-ignore
        remoteVideo.play?.().catch(() => {});
      }
    }
  });
</script>

<div class="h-full bg-white rounded-lg shadow flex flex-col">
  <!-- Header -->
  <div class="bg-white shadow-sm border-b border-gray-200 p-4 rounded-t-lg">
    <div class="flex justify-between items-center">
      <h1 class="text-xl font-semibold text-gray-900">Chat Room #{currentChatId}</h1>
      <div class="flex items-center space-x-4">
        <div class="text-sm text-gray-500">
          {authService.authState.userData?.name}
        </div>
        <button
          onclick={startVideoCall}
          class="px-4 py-2 bg-green-600 text-white rounded-lg hover:bg-green-700 focus:outline-none focus:ring-2 focus:ring-green-500 transition-colors flex items-center space-x-2"
          title="Start video call"
        >
          <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15 10l4.553-2.276A1 1 0 0121 8.618v6.764a1 1 0 01-1.447.894L15 14M5 18h8a2 2 0 002-2V8a2 2 0 00-2-2H5a2 2 0 00-2 2v8a2 2 0 002 2z"></path>
          </svg>
          <span>Call</span>
        </button>
      </div>
    </div>
  </div>

  <!-- Messages Area -->
  <div class="flex-1 overflow-hidden">
    <div class="h-full flex flex-col">
      <div class="flex-1 overflow-y-auto p-4 space-y-4" bind:this={messagesContainer} onscroll={handleScroll}>
        <!-- Loading indicator for history -->
        {#if chatService.isChatHistoryLoading(currentChatId)}
          <div class="flex justify-center py-2">
            <div class="flex items-center space-x-2 text-gray-500">
              <svg class="animate-spin h-4 w-4" fill="none" viewBox="0 0 24 24">
                <circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4"></circle>
                <path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
              </svg>
              <span class="text-sm">Loading history...</span>
            </div>
          </div>
        {/if}

        {#each messageGroups as group (group.timestamp.getTime())}
          <div class="flex flex-col space-y-1 {group.isOwnMessage ? 'items-end' : 'items-start'}">
            <!-- Group header with sender name and timestamp -->
            <div class="flex items-center space-x-2 {group.isOwnMessage ? 'flex-row-reverse space-x-reverse' : ''} mb-1">
              <span class="text-sm font-medium text-gray-900">{group.sender}</span>
              <span class="text-xs text-gray-500">
                {group.timestamp.toLocaleTimeString()}
              </span>
            </div>

            <!-- Messages in the group -->
            <div class="space-y-1 {group.isOwnMessage ? 'items-end' : 'items-start'} flex flex-col">
              {#each group.messages as message (message.id)}
                <div class="rounded-lg p-3 shadow-sm border max-w-md {
                  group.isOwnMessage
                    ? 'bg-blue-500 text-white border-blue-600'
                    : 'bg-white text-gray-800 border-gray-200'
                }">
                  <!-- Text content -->
                  {#if message.text}
                    <p class="mb-2">{message.text}</p>
                  {/if}

                  <!-- Attachment content -->
                  {#if message.attachment}
                    {#if isImageAttachment(message.attachment)}
                      <!-- Image attachment (accessible) -->
                        <div class="rounded-lg overflow-hidden">
                          <button
                            type="button"
                            class="p-0 border-0 bg-transparent rounded-lg cursor-pointer hover:opacity-90 transition-opacity"
                            onclick={() => openImage(message.attachment)}
                            aria-label={"Open image " + (message.attachment?.name || '')}
                          >
                            <img
                              src={createImageUrl(message.attachment.data)}
                              alt={message.attachment.name}
                              class="max-w-full h-auto rounded-lg block"
                            />
                          </button>
                          <div class="text-xs mt-1 opacity-75">
                            {message.attachment.name}
                          </div>
                        </div>
                    {:else if isVideoAttachment(message.attachment)}
                      <!-- Video attachment -->
                      <div class="video-attachment">
                        <VideoPlayer
                          videoData={message.attachment.data}
                          fileName={message.attachment.name}
                          mimeType="video/mp4"
                        />
                      </div>
                    {:else}
                      <!-- File attachment -->
                      <div class="flex items-center space-x-2 p-2 rounded {
                        group.isOwnMessage ? 'bg-blue-600' : 'bg-gray-100'
                      }">
                        <div class="text-2xl">📎</div>
                        <div class="flex-1 min-w-0">
                          <div class="text-sm font-medium truncate {
                            group.isOwnMessage ? 'text-white' : 'text-gray-900'
                          }">
                            {message.attachment.name}
                          </div>
                          <div class="text-xs {
                            group.isOwnMessage ? 'text-blue-100' : 'text-gray-500'
                          }">
                            {formatFileSize(message.attachment.data.length)}
                          </div>
                        </div>
                        <button
                          class="text-sm px-2 py-1 rounded {
                            group.isOwnMessage
                              ? 'bg-blue-700 text-white hover:bg-blue-800'
                              : 'bg-blue-600 text-white hover:bg-blue-700'
                          }"
                          onclick={() => downloadAttachment(message.attachment)}
                        >
                          Download
                        </button>
                      </div>
                    {/if}
                  {/if}
                </div>
              {/each}
            </div>
          </div>
        {/each}

        {#if messageGroups.length === 0}
          <div class="text-center py-8">
            <p class="text-gray-500">No messages yet. Start the conversation!</p>
          </div>
        {/if}
      </div>
    </div>
  </div>

  <!-- Message Input -->
  <div class="bg-white border-t border-gray-200 p-4">
    <div class="max-w-4xl mx-auto">
      <!-- File preview area -->
      {#if selectedFile}
        <div class="mb-3 p-3 bg-gray-50 rounded-lg flex items-center justify-between">
          <div class="flex items-center space-x-3">
            <div class="text-2xl">
              {#if detectAttachmentType(selectedFile) === 0}
                🖼️
              {:else if detectAttachmentType(selectedFile) === 2}
                🎥
              {:else}
                📎
              {/if}
            </div>
            <div>
              <div class="text-sm font-medium text-gray-900">{selectedFile.name}</div>
              <div class="text-xs text-gray-500">{formatFileSize(selectedFile.size)}</div>
            </div>
          </div>
          <button
            onclick={clearSelectedFile}
            class="text-red-600 hover:text-red-800 text-sm font-medium"
          >
            Remove
          </button>
        </div>
      {/if}

      <form onsubmit={(e) => { e.preventDefault(); sendMessage(); }} class="flex space-x-2">
        <!-- File input button -->
        <button
          type="button"
          onclick={() => fileInputRef.click()}
          class="px-3 py-2 bg-gray-100 text-gray-700 rounded-lg hover:bg-gray-200 focus:outline-none focus:ring-2 focus:ring-blue-500 transition-colors"
          title="Attach file"
        >
          📎
        </button>

        <!-- Hidden file input -->
        <input
          bind:this={fileInputRef}
          type="file"
          onchange={handleFileSelect}
          class="hidden"
          accept="image/*,video/*,*"
        />

        <!-- Message input -->
        <input
          bind:value={newMessage}
          onkeydown={handleKeydown}
          placeholder="Type your message..."
          class="flex-1 border border-gray-300 rounded-lg px-4 py-2 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
        />

        <!-- Send button -->
        <button
          type="submit"
          disabled={!newMessage.trim() && !selectedFile}
          class="px-6 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2 disabled:opacity-50 disabled:cursor-not-allowed transition-colors"
        >
          Send
        </button>
      </form>
    </div>
  </div>

  <!-- Video Call Modal -->
  {#if showCallModal}
    <div class="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div class="bg-white rounded-lg shadow-xl max-w-4xl w-full mx-4">
        <!-- Modal Header -->
        <div class="bg-gray-100 px-6 py-4 rounded-t-lg flex justify-between items-center">
          <h3 class="text-lg font-semibold text-gray-900">Video Call</h3>
          <button
            onclick={endCall}
            class="text-gray-400 hover:text-gray-600 transition-colors"
            aria-label="Close video call"
          >
            <svg class="w-6 h-6" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12"></path>
            </svg>
          </button>
        </div>

        <!-- Call Status -->
        <div class="px-6 py-2 bg-gray-50">
          <p class="text-sm text-gray-600">{callStatus}</p>
        </div>

        <!-- Video Grid -->
        <div class="p-6">
          <div class="grid grid-cols-1 lg:grid-cols-2 gap-4">
            <!-- Local Video -->
            <div class="relative">
              <video
                bind:this={localVideo}
                autoplay
                muted
                playsinline
                class="w-full h-64 bg-gray-900 rounded-lg object-cover"
              >
                <track kind="captions" label="Local video" />
              </video>
              <div class="absolute bottom-2 left-2 bg-black bg-opacity-50 text-white px-2 py-1 rounded text-sm">
                You
              </div>
            </div>

            <!-- Remote Video -->
            <div class="relative">
              <video
                bind:this={remoteVideo}
                autoplay
                playsinline
                class="w-full h-64 bg-gray-900 rounded-lg object-cover"
              >
                <track kind="captions" label="Remote video" />
              </video>
              <div class="absolute bottom-2 left-2 bg-black bg-opacity-50 text-white px-2 py-1 rounded text-sm">
                Remote
              </div>
            </div>
          </div>

          <!-- Call Controls -->
          <div class="flex justify-center space-x-4 mt-6">
            {#if isCallActive}
              <button
                onclick={toggleMute}
                class="px-4 py-2 bg-yellow-600 text-white rounded-lg hover:bg-yellow-700 transition-colors flex items-center space-x-2"
              >
                <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 11a7 7 0 01-7 7m0 0a7 7 0 01-7-7m7 7v4m0 0H8m4 0h4m-4-8a3 3 0 01-3-3V5a3 3 0 116 0v6a3 3 0 01-3 3z"></path>
                </svg>
                <span>Mute</span>
              </button>

              <button
                onclick={toggleVideo}
                class="px-4 py-2 bg-purple-600 text-white rounded-lg hover:bg-purple-700 transition-colors flex items-center space-x-2"
              >
                <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15 10l4.553-2.276A1 1 0 0121 8.618v6.764a1 1 0 01-1.447.894L15 14M5 18h8a2 2 0 002-2V8a2 2 0 00-2-2H5a2 2 0 00-2 2v8a2 2 0 002 2z"></path>
                </svg>
                <span>Video</span>
              </button>
            {/if}

            <button
              onclick={endCall}
              class="px-6 py-2 bg-red-600 text-white rounded-lg hover:bg-red-700 transition-colors flex items-center space-x-2"
            >
              <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M16 8l2-2m0 0l2-2m-2 2l-2-2m2 2l2 2m-6 6v8a2 2 0 01-2 2H6a2 2 0 01-2-2v-8a2 2 0 012-2h8a2 2 0 012 2z"></path>
              </svg>
              <span>End Call</span>
            </button>
          </div>
        </div>
      </div>
    </div>
  {/if}
</div>
