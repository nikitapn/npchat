import type { RegisteredUser, ChatMessage, ChatId, MessageId, ContactList } from '../npchat';
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

class ChatServiceImpl {
	private chatListener: ChatListenerImpl | null = null;
	private registeredUser: RegisteredUser | null = null;
	
	// State for real-time notifications
	public notifications = $state<ChatNotification[]>([]);
	public chatUpdates = $state<Map<ChatId, ChatUpdate>>(new Map());
	public activeChatId = $state<ChatId | null>(null);
	
	// Callbacks for UI updates
	private onNewMessageCallbacks = new Set<(notification: ChatNotification) => void>();
	private onChatUpdateCallbacks = new Set<(chatId: ChatId, update: ChatUpdate) => void>();

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
	}

	// Set the currently active chat (when user opens a specific chat)
	setActiveChatId(chatId: ChatId | null) {
		this.activeChatId = chatId;
		
		// Mark messages as read for active chat
		if (chatId && this.chatUpdates.has(chatId)) {
			const update = this.chatUpdates.get(chatId)!;
			update.unreadCount = 0;
			this.chatUpdates.set(chatId, update);
		}
	}

	// Send a message through the registered user
	async sendMessage(chatId: ChatId, text: string): Promise<MessageId> {
		if (!this.registeredUser) {
			throw new Error('Chat service not initialized');
		}

		const chatMessage: ChatMessage = {
			chatId: chatId,
			timestamp: Date.now(),
			str: text.trim(),
			attachment: undefined
		};

		return await this.registeredUser.SendMessage(chatId, chatMessage);
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

	// Get messages for a specific chat (from notifications)
	getMessagesForChat(chatId: ChatId): ChatNotification[] {
		return this.notifications.filter(n => n.chatId === chatId);
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
