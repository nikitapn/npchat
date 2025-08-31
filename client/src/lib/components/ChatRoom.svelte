<script lang="ts">
	import type { ChatId, MessageId } from '../rpc/npchat';
	import { authService } from '../rpc/services/auth';
	import { chatService } from '../rpc/services/Chat.svelte';

	interface ChatRoomProps {
		currentChatId: ChatId;
	}

	let { currentChatId }: ChatRoomProps = $props();

	interface Message {
		id: MessageId;
		text: string;
		timestamp: Date;
		sender: string;
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
				currentGroup.messages.push(message);
			}
		}

		return groups;
	}

	// Update message groups when messages change
	$effect(() => {
		messageGroups = groupMessages(messages);
	});

	// Convert chat notifications to UI messages
	function updateMessages() {
		const notifications = chatService.getMessagesForChat(currentChatId);
		messages = notifications.map(notification => ({
			id: notification.messageId,
			text: notification.message.str,
			timestamp: notification.timestamp,
			sender: 'Other User' // TODO: Get actual sender name from user ID
		}));
	}

	// Set this chat as active and load messages
	function activateChat() {
		chatService.setActiveChatId(currentChatId);
		updateMessages();
	}

	// Send a message
	async function sendMessage() {
		if (!newMessage.trim()) return;

		try {
			const messageId = await chatService.sendMessage(currentChatId, newMessage.trim());
			
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

	// Subscribe to new messages for this chat
	const unsubscribe = chatService.onNewMessage((notification) => {
		if (notification.chatId === currentChatId) {
			// Add new message to the list
			const newMsg: Message = {
				id: notification.messageId,
				text: notification.message.str,
				timestamp: notification.timestamp,
				sender: 'Other User' // TODO: Get actual sender name
			};
			
			// Check if message already exists (avoid duplicates)
			if (!messages.find(m => m.id === notification.messageId)) {
				messages.push(newMsg);
				messages = [...messages]; // Trigger reactivity
			}
		}
	});

	// Cleanup when component is destroyed
	$effect(() => {
		return () => {
			unsubscribe();
			// Don't set activeChatId to null here - let the parent component handle it
		};
	});
</script>

<div class="min-h-screen bg-gray-50 flex flex-col">
	<!-- Header -->
	<div class="bg-white shadow-sm border-b border-gray-200 p-4">
		<div class="max-w-4xl mx-auto flex justify-between items-center">
			<h1 class="text-xl font-semibold text-gray-900">Chat Room #{currentChatId}</h1>
			<div class="text-sm text-gray-500">
				{authService.authState.userData?.name}
			</div>
		</div>
	</div>

	<!-- Messages Area -->
	<div class="flex-1 overflow-hidden">
		<div class="max-w-4xl mx-auto h-full flex flex-col">
			<div class="flex-1 overflow-y-auto p-4 space-y-4">
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
									<p>{message.text}</p>
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
