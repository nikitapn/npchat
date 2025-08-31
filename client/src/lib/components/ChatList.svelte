<script lang="ts">
	import type { RegisteredUser, Chat, ChatList, ChatId } from '../rpc/npchat';
	import { chatService } from '../rpc/services/Chat.svelte';

	interface ChatListProps {
		registeredUser: RegisteredUser | null;
		currentChatId: ChatId | null;
		onSelectChat: (chatId: ChatId) => void;
		onCreateNewChat: () => void;
	}

	let { registeredUser, currentChatId, onSelectChat, onCreateNewChat }: ChatListProps = $props();

	let chats: Chat[] = $state([]);

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
</script>

<div class="bg-white rounded-lg shadow p-4">
	<div class="flex justify-between items-center mb-4">
		<h3 class="text-lg font-semibold text-gray-900">Chats</h3>
		<button
			onclick={handleCreateNewChat}
			class="px-3 py-1 bg-green-600 text-white rounded-md hover:bg-green-700 text-sm"
		>
			New Chat
		</button>
	</div>

	<div class="space-y-2">
		{#each chats as chat}
			{@const unreadCount = chatService.getUnreadCount(chat.id)}
			<button 
				class="w-full flex justify-between items-center p-3 rounded-lg transition-colors {
					currentChatId === chat.id 
						? 'bg-blue-100 border border-blue-300' 
						: 'bg-gray-50 hover:bg-gray-100'
				}"
				onclick={() => onSelectChat(chat.id)}
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
							Chat Room #{chat.id}
						</div>
						<div class="text-sm text-gray-500">
							{chat.participantCount} participant{chat.participantCount !== 1 ? 's' : ''}
						</div>
						{#if chat.lastMessageTime}
							<div class="text-xs text-gray-400">
								Last message: {new Date(chat.lastMessageTime * 1000).toLocaleString()}
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
				No chats yet. Create a new chat to get started!
			</div>
		{/each}
	</div>
</div>
