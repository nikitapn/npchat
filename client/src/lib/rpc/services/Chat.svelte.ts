import type { RegisteredUser, ChatMessage, ChatId, MessageId, ContactList, ChatAttachment, ChatMessageContent } from '../npchat';
import { _IChatListener_Servant } from '../npchat';
import { poa } from '../index';
import { authService } from './auth';

interface ChatNotification {
  chatId: ChatId;
  messageId: MessageId;
  message: ChatMessage;
  timestamp: Date;
}

interface ChatUpdate {
  chatId: ChatId;
  lastMessage?: ChatMessage;
  unreadCount: number;
}

interface ChatHistory {
  chatId: ChatId;
  messages: ChatMessage[];
  hasMore: boolean;
  isLoading: boolean;
  offset: number;
}

class ChatServiceImpl {
  private chatListener: ChatListenerImpl | null = null;
  private registeredUser: RegisteredUser | null = null;

  // Constants for pagination
  private readonly MESSAGES_PER_PAGE = 50;
  private readonly MAX_CACHED_MESSAGES = 200;

  // State for real-time notifications
  public notifications = $state<ChatNotification[]>([]);
  public chatUpdates = $state<Map<ChatId, ChatUpdate>>(new Map());
  public activeChatId = $state<ChatId | null>(null);

  // State for chat histories with pagination
  public chatHistories = $state<Map<ChatId, ChatHistory>>(new Map());

  // Callbacks for UI updates
  private onNewMessageCallbacks = new Set<(notification: ChatNotification) => void>();
  private onChatUpdateCallbacks = new Set<(chatId: ChatId, update: ChatUpdate) => void>();
  private onHistoryLoadedCallbacks = new Set<(chatId: ChatId, history: ChatHistory) => void>();

  async initialize() {
    try {
      const user = authService.authState.user;
      if (!user) {
        console.error('No user available for chat service');
        return;
      }

      this.registeredUser = user;

      // Create and register global ChatListener
      this.chatListener = new ChatListenerImpl(this);
      const listenerObjectId = poa.activate_object(this.chatListener);

      // Subscribe to events for all chats
      await this.registeredUser.SubscribeToEvents(listenerObjectId);

      console.log('Global chat service initialized');
    } catch (error) {
      console.error('Failed to initialize chat service:', error);
    }
  }

  cleanup() {
    if (this.chatListener) {
      try {
        poa.deactivate_object(this.chatListener.oid);
        console.log('Global ChatListener unregistered');
      } catch (error) {
        console.error('Error removing global ChatListener:', error);
      }
    }

    // Clear all state
    this.chatHistories.clear();
    this.chatUpdates.clear();
    this.notifications.length = 0;
    this.activeChatId = null;
  }

  // Set the currently active chat (when user opens a specific chat)
  async setActiveChatId(chatId: ChatId | null) {
    this.activeChatId = chatId;

    // Mark messages as read for active chat
    if (chatId && this.chatUpdates.has(chatId)) {
      const update = this.chatUpdates.get(chatId)!;
      update.unreadCount = 0;
      this.chatUpdates.set(chatId, update);
    }

    // Load chat history if not already loaded
    if (chatId && !this.chatHistories.has(chatId)) {
      await this.loadChatHistory(chatId);
    }
  }

  // Send a message through the registered user
  async sendMessage(chatId: ChatId, text: string, attachment?: ChatAttachment): Promise<MessageId> {
    if (!this.registeredUser) {
      throw new Error('Chat service not initialized');
    }

    const chatMessage: ChatMessageContent = {
      text: text.trim(),
      attachment: attachment
    };

    return await this.registeredUser.SendMessage(chatId, chatMessage);
  }

  // Load chat history for a specific chat
  private async loadChatHistory(chatId: ChatId, offset: number = 0): Promise<void> {
    if (!this.registeredUser) {
      throw new Error('Chat service not initialized');
    }

    // Get or create history entry
    let history = this.chatHistories.get(chatId);
    if (!history) {
      history = {
        chatId,
        messages: [],
        hasMore: true,
        isLoading: false,
        offset: 0
      };
      this.chatHistories.set(chatId, history);
    }

    // Prevent concurrent loading
    if (history.isLoading) {
      return;
    }

    history.isLoading = true;

    try {
      console.log(`Loading chat history for chat ${chatId}, offset: ${offset}, limit: ${this.MESSAGES_PER_PAGE}`);

      const messages = await this.registeredUser.GetChatHistory(chatId, this.MESSAGES_PER_PAGE, offset);

      if (offset === 0) {
        // Initial load - replace messages
        history.messages = messages;
      } else {
        // Pagination - prepend older messages
        history.messages = [...messages, ...history.messages];
      }

      // Update pagination state
      history.hasMore = messages.length === this.MESSAGES_PER_PAGE;
      history.offset = offset + messages.length;

      // Limit cache size to prevent memory issues
      if (history.messages.length > this.MAX_CACHED_MESSAGES) {
        const excess = history.messages.length - this.MAX_CACHED_MESSAGES;
        history.messages = history.messages.slice(0, this.MAX_CACHED_MESSAGES);
        // Adjust offset since we removed messages
        history.offset = Math.max(0, history.offset - excess);
      }

      console.log(`Loaded ${messages.length} messages for chat ${chatId}. Total: ${history.messages.length}, hasMore: ${history.hasMore}`);

      // Notify callbacks
      this.onHistoryLoadedCallbacks.forEach(callback => callback(chatId, history!));

    } catch (error) {
      console.error('Failed to load chat history:', error);
    } finally {
      history.isLoading = false;
    }
  }

  // Load more history (older messages) for pagination
  async loadMoreHistory(chatId: ChatId): Promise<boolean> {
    const history = this.chatHistories.get(chatId);
    if (!history || !history.hasMore || history.isLoading) {
      return false;
    }

    await this.loadChatHistory(chatId, history.offset);
    return this.chatHistories.get(chatId)?.hasMore || false;
  }

  // Refresh chat history (reload from beginning)
  async refreshChatHistory(chatId: ChatId): Promise<void> {
    // Clear existing history
    this.chatHistories.delete(chatId);
    // Load fresh history
    await this.loadChatHistory(chatId, 0);
  }

  // Handle incoming message notification
  onMessageReceived(messageId: MessageId, message: ChatMessage) {
    const notification: ChatNotification = {
      chatId: message.chatId,
      messageId,
      message,
      timestamp: new Date(message.timestamp)
    };

    // Add to global notifications
    this.notifications.push(notification);

    // Add to chat history if it's loaded
    const history = this.chatHistories.get(message.chatId);
    if (history) {
      // Check if message already exists (avoid duplicates)
      const exists = history.messages.some(m =>
        m.timestamp === message.timestamp && m.content.text === message.content.text
      );

      if (!exists) {
        history.messages.push(message);

        // Maintain cache size limit
        if (history.messages.length > this.MAX_CACHED_MESSAGES) {
          const excess = history.messages.length - this.MAX_CACHED_MESSAGES;
          history.messages = history.messages.slice(excess);
          history.offset = Math.max(0, history.offset - excess);
        }
      }
    }

    // Update chat-specific info
    const existingUpdate = this.chatUpdates.get(message.chatId);
    const isActiveChat = this.activeChatId === message.chatId;

    const chatUpdate: ChatUpdate = {
      chatId: message.chatId,
      lastMessage: message,
      unreadCount: isActiveChat ? 0 : (existingUpdate?.unreadCount || 0) + 1
    };

    this.chatUpdates.set(message.chatId, chatUpdate);

    // Notify callbacks
    this.onNewMessageCallbacks.forEach(callback => callback(notification));
    this.onChatUpdateCallbacks.forEach(callback => callback(message.chatId, chatUpdate));

    console.log('Message received for chat', message.chatId, '- unread count:', chatUpdate.unreadCount);
  }

  // Handle message delivery confirmation
  onMessageDelivered(chatId: ChatId, messageId: MessageId) {
    console.log('Message delivered confirmation:', chatId, messageId);
    // Could update message status in UI
  }

  // Handle contact list updates
  onContactListUpdated(contacts: ContactList) {
    console.log('Contact list updated:', contacts);
    // Could trigger contact list refresh
  }

  // Subscribe to new message notifications
  onNewMessage(callback: (notification: ChatNotification) => void) {
    this.onNewMessageCallbacks.add(callback);
    return () => this.onNewMessageCallbacks.delete(callback);
  }

  // Subscribe to chat updates
  onChatUpdate(callback: (chatId: ChatId, update: ChatUpdate) => void) {
    this.onChatUpdateCallbacks.add(callback);
    return () => this.onChatUpdateCallbacks.delete(callback);
  }

  // Subscribe to history loaded events
  onHistoryLoaded(callback: (chatId: ChatId, history: ChatHistory) => void) {
    this.onHistoryLoadedCallbacks.add(callback);
    return () => this.onHistoryLoadedCallbacks.delete(callback);
  }

  // Get messages for a specific chat (from history or notifications)
  getMessagesForChat(chatId: ChatId): ChatMessage[] {
    const history = this.chatHistories.get(chatId);
    if (history) {
      return history.messages;
    }

    // Fallback to notifications if history not loaded
    return this.notifications
      .filter(n => n.chatId === chatId)
      .map(n => n.message);
  }

  // Get chat history object for a specific chat
  getChatHistory(chatId: ChatId): ChatHistory | undefined {
    return this.chatHistories.get(chatId);
  }

  // Check if chat history is currently loading
  isChatHistoryLoading(chatId: ChatId): boolean {
    return this.chatHistories.get(chatId)?.isLoading || false;
  }

  // Check if chat has more history to load
  hasMoreHistory(chatId: ChatId): boolean {
    return this.chatHistories.get(chatId)?.hasMore || false;
  }

  // Get current message count for a chat
  getMessageCount(chatId: ChatId): number {
    return this.chatHistories.get(chatId)?.messages.length || 0;
  }

  // Get pagination info for a chat
  getPaginationInfo(chatId: ChatId) {
    const history = this.chatHistories.get(chatId);
    return {
      messageCount: history?.messages.length || 0,
      hasMore: history?.hasMore || false,
      isLoading: history?.isLoading || false,
      offset: history?.offset || 0
    };
  }

  // Get unread count for a chat
  getUnreadCount(chatId: ChatId): number {
    return this.chatUpdates.get(chatId)?.unreadCount || 0;
  }

  // Get total unread count across all chats
  getTotalUnreadCount(): number {
    let total = 0;
    this.chatUpdates.forEach(update => {
      total += update.unreadCount;
    });
    return total;
  }
}

// Implementation of ChatListener that integrates with the service
class ChatListenerImpl extends _IChatListener_Servant {
  constructor(private chatService: ChatServiceImpl) {
    super();
  }

  OnMessageReceived(messageId: MessageId, message: ChatMessage): void {
    this.chatService.onMessageReceived(messageId, message);
  }

  OnMessageDelivered(chatId: ChatId, messageId: MessageId): void {
    this.chatService.onMessageDelivered(chatId, messageId);
  }

  OnContactListUpdated(contacts: ContactList): void {
    this.chatService.onContactListUpdated(contacts);
  }
}

// Export singleton instance
export const chatService = new ChatServiceImpl();

// Initialize when auth state changes
authService.subscribe((authState) => {
  if (authState.isAuthenticated && authState.user) {
    chatService.initialize();
  } else {
    chatService.cleanup();
  }
});
