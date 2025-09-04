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
    const blob = new Blob([data], { type: 'image/jpeg' }); // Assume JPEG for now
    return URL.createObjectURL(blob);
  }

  function formatFileSize(bytes: number): string {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
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

  // Cleanup when component is destroyed
  $effect(() => {
    return () => {
      unsubscribeNewMessage();
      unsubscribeHistoryLoaded();
      // Don't set activeChatId to null here - let the parent component handle it
    };
  });
</script>

<div class="h-full bg-white rounded-lg shadow flex flex-col">
  <!-- Header -->
  <div class="bg-white shadow-sm border-b border-gray-200 p-4 rounded-t-lg">
    <div class="flex justify-between items-center">
      <h1 class="text-xl font-semibold text-gray-900">Chat Room #{currentChatId}</h1>
      <div class="text-sm text-gray-500">
        {authService.authState.userData?.name}
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
                            onclick={() => window.open(createImageUrl(message.attachment!.data), '_blank')}
                            aria-label={"Open image " + message.attachment!.name}
                          >
                            <img
                              src={createImageUrl(message.attachment.data)}
                              alt={message.attachment.name}
                              class="max-w-full h-auto rounded-lg block"
                            />
                          </button>
                          <div class="text-xs mt-1 opacity-75">
                            {message.attachment!.name}
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
                          onclick={() => {
                            const blob = new Blob([message.attachment!.data]);
                            const url = URL.createObjectURL(blob);
                            const a = document.createElement('a');
                            a.href = url;
                            a.download = message.attachment!.name;
                            a.click();
                            URL.revokeObjectURL(url);
                          }}
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
</div>
