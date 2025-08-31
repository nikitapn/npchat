<script lang="ts">
	import type { RegisteredUser, ChatMessage, ChatId, MessageId, ContactList } from '../rpc/npchat';
	import { _IChatListener_Servant } from '../rpc/npchat';
	import { authService } from '../rpc/services/auth';
	import { poa } from '../rpc/index';
	import * as NPRPC from 'nprpc';

	interface Message {
		id: MessageId;
		text: string;
		timestamp: Date;
		sender: string;
	}

	let messages: Message[] = $state([]);
	let newMessage = $state('');
	let chatId: ChatId = 1; // Default chat room
	let registeredUser: RegisteredUser | null = null;
	let chatListener: ChatListenerImpl | null = null;

	// Implement ChatListener to receive real-time notifications
	class ChatListenerImpl extends _IChatListener_Servant {
		OnMessageReceived(messageId: MessageId, message: ChatMessage): void {
			console.log('Real-time message received:', messageId, message);
			
			// Only add message if it's for our current chat
			if (message.chatId === chatId) {
				const newMsg: Message = {
					id: messageId,
					text: message.str,
					timestamp: new Date(message.timestamp),
					sender: 'Other User' // We'll improve this later with actual user names
				};
				
				// Check if message already exists (avoid duplicates)
				if (!messages.find(m => m.id === messageId)) {
					messages.push(newMsg);
					messages = [...messages]; // Trigger reactivity
				}
			}
		}

		OnMessageDelivered(chatId: ChatId, messageId: MessageId): void {
			console.log('Message delivered confirmation:', chatId, messageId);
			// Could update message status to "delivered" here
		}

		OnContactListUpdated(contacts: ContactList): void {
			console.log('Contact list updated:', contacts);
			// Could update contact list UI here
		}
	}

	// Initialize RegisteredUser connection and ChatListener
	async function initializeChat() {
		try {
			const user = authService.authState.user;
			if (!user) {
				console.error('No user database object ID available');
				return;
			}

			// Get RegisteredUser proxy
			registeredUser = user;
			
			// Create and register ChatListener for real-time notifications
			chatListener = new ChatListenerImpl();
			const listenerObjectId = poa.activate_object(chatListener);
			
			// Subscribe to events to receive real-time chat notifications
			await registeredUser.SubscribeToEvents(listenerObjectId);
			
			console.log('Chat initialized with RegisteredUser and ChatListener subscribed');
		} catch (error) {
			console.error('Failed to initialize chat:', error);
		}
	}

	// Send a message
	async function sendMessage() {
		if (!newMessage.trim() || !registeredUser) return;

		try {
			const chatMessage: ChatMessage = {
				chatId: chatId,
				timestamp: Date.now(),
				str: newMessage.trim(),
				attachment: undefined
			};

			const messageId = await registeredUser.SendMessage(chatId, chatMessage);
			
			// Add message to local list immediately for immediate feedback
			const localMessage: Message = {
				id: messageId,
				text: newMessage.trim(),
				timestamp: new Date(),
				sender: authService.authState.userData?.name || 'You'
			};
			messages.push(localMessage);
			messages = [...messages]; // Trigger reactivity

			newMessage = '';
			console.log('Message sent with ID:', messageId);
		} catch (error) {
			console.error('Failed to send message:', error);
		}
	}

	// Handle Enter key in input
	function handleKeydown(event: KeyboardEvent) {
		if (event.key === 'Enter' && !event.shiftKey) {
			event.preventDefault();
			sendMessage();
		}
	}

	// Cleanup function to remove listener
	function cleanup() {
		if (chatListener) {
			try {
        poa.deactivate_object(chatListener.oid);
				console.log('ChatListener unregistered');
			} catch (error) {
				console.error('Error removing ChatListener:', error);
			}
		}
	}

	// Initialize chat when component mounts
	initializeChat();

	// Cleanup when component is destroyed
	$effect(() => {
		return cleanup;
	});
</script>

<div class="min-h-screen bg-gray-50 flex flex-col">
	<!-- Header -->
	<div class="bg-white shadow-sm border-b border-gray-200 p-4">
		<div class="max-w-4xl mx-auto flex justify-between items-center">
			<h1 class="text-xl font-semibold text-gray-900">Chat Room #{chatId}</h1>
			<div class="text-sm text-gray-500">
				{authService.authState.userData?.name}
			</div>
		</div>
	</div>

	<!-- Messages Area -->
	<div class="flex-1 overflow-hidden">
		<div class="max-w-4xl mx-auto h-full flex flex-col">
			<div class="flex-1 overflow-y-auto p-4 space-y-3">
				{#each messages as message (message.id)}
					<div class="flex flex-col space-y-1">
						<div class="flex items-center space-x-2">
							<span class="text-sm font-medium text-gray-900">{message.sender}</span>
							<span class="text-xs text-gray-500">
								{message.timestamp.toLocaleTimeString()}
							</span>
						</div>
						<div class="bg-white rounded-lg p-3 shadow-sm border border-gray-200 max-w-md">
							<p class="text-gray-800">{message.text}</p>
						</div>
					</div>
				{/each}
				
				{#if messages.length === 0}
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
			<form onsubmit={(e) => { e.preventDefault(); sendMessage(); }} class="flex space-x-4">
				<input
					bind:value={newMessage}
					onkeydown={handleKeydown}
					placeholder="Type your message..."
					class="flex-1 border border-gray-300 rounded-lg px-4 py-2 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent"
				/>
				<button
					type="submit"
					disabled={!newMessage.trim()}
					class="px-6 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2 disabled:opacity-50 disabled:cursor-not-allowed"
				>
					Send
				</button>
			</form>
		</div>
	</div>
</div>
