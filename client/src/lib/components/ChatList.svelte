<script lang="ts">
  import type { RegisteredUser, Chat, ChatList, ChatId } from '../rpc/npchat';
  import { chatService } from '../rpc/services/Chat.svelte';
  import { authService } from '../rpc/services/auth';
  import ContextMenu, { type ContextMenuItem } from './ContextMenu.svelte';
  import { _ } from '../i18n';

  interface ChatListProps {
    registeredUser: RegisteredUser | null;
    currentChatId: ChatId | null;
    onSelectChat: (chatId: ChatId | null) => void;
    onCreateNewChat: () => void;
  }

  let { registeredUser, currentChatId, onSelectChat, onCreateNewChat }: ChatListProps = $props();

  let chats: Chat[] = $state([]);
  let contextMenu = $state({
    visible: false,
    x: 0,
    y: 0,
    chatId: null as ChatId | null
  });

  // Get context menu items for a chat
  function getContextMenuItems(chat: Chat): ContextMenuItem[] {
    return [
      {
        label: $_('chat.openChat'),
        icon: 'open',
        action: () => {
          onSelectChat(chat.id);
          hideContextMenu();
        }
      },
      {
        separator: true,
        label: '',
        action: () => {}
      },
      {
        label: $_('chat.leaveChat'),
        icon: 'exit',
        className: 'text-yellow-700 hover:bg-yellow-50',
        action: () => leaveChat(chat.id)
      },
      ...(chat.canDelete ? [{
        label: $_('chat.deleteChat'),
        icon: 'delete',
        className: 'text-red-600 hover:bg-red-50',
        action: () => deleteChat(chat.id)
      }] : []),
      {
        separator: true,
        label: '',
        action: () => {}
      },
      {
        label: $_('chat.cancel'),
        icon: 'cancel',
        className: 'text-gray-500 hover:bg-gray-100',
        action: () => hideContextMenu()
      }
    ];
  }

  // Load user's chats
  async function loadChats() {
    if (!registeredUser) return;

    try {
      const chatList = await registeredUser.GetChats();
      chats = chatList;
      console.log('Loaded chats:', chats);
    } catch (error) {
      console.error('Failed to load chats:', error);
    }
  }

  // Create a new chat
  async function createNewChat() {
    if (!registeredUser) return;

    try {
      const newChatId = await registeredUser.CreateChat();
      console.log('Created new chat:', newChatId);

      // Refresh chat list
      await loadChats();

      // Select the new chat
      onSelectChat(newChatId);
    } catch (error) {
      console.error('Failed to create chat:', error);
    }
  }

  // Subscribe to chat updates for real-time unread counts
  const unsubscribe = chatService.onChatUpdate((chatId, update) => {
    // Refresh chat list to show new chats that may have been created
    if (!chats.find(c => c.id === chatId)) {
      loadChats();
    }
  });

  // Load chats when registeredUser becomes available
  $effect(() => {
    if (registeredUser) {
      loadChats();
    }
  });

  // Cleanup subscription
  $effect(() => {
    return unsubscribe;
  });

  // Handle create new chat click
  function handleCreateNewChat() {
    onCreateNewChat();
    createNewChat();
  }

  // Context menu handlers
  function handleRightClick(event: MouseEvent, chatId: ChatId) {
    event.preventDefault();
    contextMenu.visible = true;
    contextMenu.x = event.clientX;
    contextMenu.y = event.clientY;
    contextMenu.chatId = chatId;
  }

  function hideContextMenu() {
    contextMenu.visible = false;
    contextMenu.chatId = null;
  }

  // Chat actions
  async function deleteChat(chatId: ChatId) {
    if (!registeredUser) {
      console.error('No registered user available');
      return;
    }

    const confirmMessage = $_('chat.confirmations.deleteChat', { values: { id: chatId } });
    if (!confirm(confirmMessage)) {
      return;
    }

    try {
      const currentUserId = authService.getCurrentUserId();

      await registeredUser.DeleteChat(chatId, currentUserId);
      console.log('Deleted chat:', chatId);

      // Refresh chat list
      await loadChats();

      // If this was the current chat, clear selection
      if (currentChatId === chatId) {
        onSelectChat(null);
      }
    } catch (error) {
      console.error('Failed to delete chat:', error);
      alert($_('chat.errors.failedToDeleteChat'));
    } finally {
      hideContextMenu();
    }
  }

  async function leaveChat(chatId: ChatId) {
    if (!registeredUser || !chatId) return;

    const confirmed = confirm($_('chat.confirmations.leaveChat', { values: { id: chatId } }));
    if (!confirmed) {
      hideContextMenu();
      return;
    }

    try {
      const currentUserId = authService.getCurrentUserId();

      await registeredUser.LeaveChatParticipant(chatId, currentUserId);
      console.log('Left chat:', chatId);

      // Refresh chat list
      await loadChats();

      // If this was the current chat, clear selection
      if (currentChatId === chatId) {
        onSelectChat(null);
      }
    } catch (error) {
      console.error('Failed to leave chat:', error);
      alert($_('chat.errors.failedToLeaveChat'));
    } finally {
      hideContextMenu();
    }
  }
</script>

<div class="bg-white rounded-lg shadow h-full flex flex-col">
  <div class="flex justify-between items-center p-4 border-b border-gray-200">
    <h3 class="text-lg font-semibold text-gray-900">{$_('chat.title')}</h3>
    <button
      onclick={handleCreateNewChat}
      class="px-3 py-1 bg-green-600 text-white rounded-md hover:bg-green-700 text-sm"
    >
      {$_('chat.newChat')}
    </button>
  </div>

  <div class="flex-1 overflow-y-auto p-4">
    <div class="space-y-2">
    {#each chats as chat}
      {@const unreadCount = chatService.getUnreadCount(chat.id)}
      <button
        class="w-full flex justify-between items-center p-3 rounded-lg transition-colors select-none {
          currentChatId === chat.id
            ? 'bg-blue-100 border border-blue-300'
            : 'bg-gray-50 hover:bg-gray-100'
        }"
        onclick={() => onSelectChat(chat.id)}
        oncontextmenu={(e) => handleRightClick(e, chat.id)}
        title="Left click to open, right click for options"
      >
        <div class="flex items-center space-x-3">
          <div class="relative">
            <div class="w-10 h-10 bg-green-500 rounded-full flex items-center justify-center">
              <span class="text-white font-medium">
                #{chat.id}
              </span>
            </div>
            {#if unreadCount > 0}
              <div class="absolute -top-1 -right-1 w-5 h-5 bg-red-500 rounded-full flex items-center justify-center">
                <span class="text-white text-xs font-bold">
                  {unreadCount > 9 ? '9+' : unreadCount}
                </span>
              </div>
            {/if}
          </div>
          <div>
            <div class="font-medium text-gray-900 {unreadCount > 0 ? 'font-bold' : ''}">
              {$_('chat.chatRoom', { values: { id: chat.id } })}
            </div>
            <div class="text-sm text-gray-500">
              {$_('chat.participantCount', { values: { count: chat.participantCount } })}
            </div>
            {#if chat.lastMessageTime}
              <div class="text-xs text-gray-400">
                {$_('chat.lastMessage', { values: { time: new Date(chat.lastMessageTime * 1000).toLocaleString() } })}
              </div>
            {/if}
          </div>
        </div>

        {#if currentChatId === chat.id}
          <div class="text-blue-600 font-medium text-sm">Active</div>
        {/if}
      </button>
    {:else}
      <div class="text-center text-gray-500 py-4">
        {$_('chat.noChats')}
      </div>
    {/each}
    </div>
  </div>
</div>

<!-- Context Menu using reusable component -->
{#if contextMenu.visible && contextMenu.chatId !== null}
  {@const selectedChat = chats.find(c => c.id === contextMenu.chatId)}
  {#if selectedChat}
    <ContextMenu
      visible={contextMenu.visible}
      x={contextMenu.x}
      y={contextMenu.y}
      items={getContextMenuItems(selectedChat)}
      title="Chat #{selectedChat.id}"
      onClose={hideContextMenu}
    />
  {/if}
{/if}
