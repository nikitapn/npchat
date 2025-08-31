<script lang="ts">
	import type { RegisteredUser, ChatId, Contact } from '../rpc/npchat';
	import { authService } from '../rpc/services/auth';
	import { chatService } from '../rpc/services/Chat.svelte';
	import ContactList from './ContactList.svelte';
	import ChatList from './ChatList.svelte';
	import ChatRoom from './ChatRoom.svelte';

	let registeredUser: RegisteredUser | null = $state(null);
	let currentChatId: ChatId | null = $state(null);
	let selectedView = $state<'chats' | 'contacts'>('chats');

	// Initialize RegisteredUser
	$effect(() => {
		const user = authService.authState.user;
		if (user) {
			registeredUser = user;
		}
	});

	// Handle chat selection
	function selectChat(chatId: ChatId) {
		currentChatId = chatId;
		selectedView = 'chats';
		// Notify global chat service about active chat
		chatService.setActiveChatId(chatId);
	}

	// Handle creating new chat
	function createNewChat() {
		// ChatList component will handle the actual creation
		// and call selectChat when done
	}

	// Handle creating chat with a specific contact
	async function createChatWithContact(contact: Contact) {
		if (!registeredUser) return;

		try {
			// Create or find existing chat with this contact
			const chatId = await registeredUser.CreateChatWith(contact.id);
			
			console.log(`Created/found chat ${chatId} with ${contact.username}`);
			
			// Switch to the chat
			selectChat(chatId);
		} catch (error) {
			console.error('Failed to create chat with contact:', error);
		}
	}
</script>

<div class="min-h-screen bg-gray-50">
	<!-- Header -->
	<div class="bg-white shadow-sm border-b border-gray-200 p-4">
		<div class="max-w-7xl mx-auto flex justify-between items-center">
			<h1 class="text-2xl font-bold text-gray-900">NPChat</h1>
			<div class="flex items-center space-x-4">
				<span class="text-sm text-gray-600">
					Welcome, {authService.authState.userData?.name}
				</span>
				<button
					onclick={() => authService.logout()}
					class="px-4 py-2 bg-red-600 text-white rounded-lg hover:bg-red-700"
				>
					Sign Out
				</button>
			</div>
		</div>
	</div>

	<div class="max-w-7xl mx-auto p-4">
		<div class="grid grid-cols-1 lg:grid-cols-4 gap-6 h-[calc(100vh-120px)]">
			<!-- Sidebar -->
			<div class="lg:col-span-1 space-y-4">
				<!-- Navigation Tabs -->
				<div class="bg-white rounded-lg shadow p-2">
					<div class="flex space-x-1">
						<button
							onclick={() => selectedView = 'chats'}
							class="flex-1 px-3 py-2 text-sm font-medium rounded-md transition-colors {
								selectedView === 'chats' 
									? 'bg-blue-600 text-white' 
									: 'text-gray-700 hover:bg-gray-100'
							}"
						>
							<div class="flex items-center justify-center space-x-1">
								<span>💬 Chats</span>
								{#if chatService.getTotalUnreadCount() > 0}
									<span class="bg-red-500 text-white text-xs rounded-full px-2 py-0.5 min-w-[20px] text-center">
										{chatService.getTotalUnreadCount() > 99 ? '99+' : chatService.getTotalUnreadCount()}
									</span>
								{/if}
							</div>
						</button>
						<button
							onclick={() => selectedView = 'contacts'}
							class="flex-1 px-3 py-2 text-sm font-medium rounded-md transition-colors {
								selectedView === 'contacts' 
									? 'bg-blue-600 text-white' 
									: 'text-gray-700 hover:bg-gray-100'
							}"
						>
							👥 Contacts
						</button>
					</div>
				</div>

				<!-- Content based on selected view -->
				{#if selectedView === 'chats'}
					<ChatList 
						{registeredUser} 
						{currentChatId} 
						onSelectChat={selectChat}
						onCreateNewChat={createNewChat}
					/>
				{:else}
					<ContactList 
						{registeredUser} 
						onCreateChat={createChatWithContact}
					/>
				{/if}
			</div>

			<!-- Main Chat Area -->
			<div class="lg:col-span-3">
				{#if currentChatId}
					<ChatRoom currentChatId={currentChatId} />
				{:else}
					<!-- Welcome screen -->
					<div class="bg-white rounded-lg shadow h-full flex items-center justify-center">
						<div class="text-center">
							<div class="text-6xl mb-4">💬</div>
							<h2 class="text-2xl font-semibold text-gray-900 mb-2">Welcome to NPChat!</h2>
							<p class="text-gray-600 mb-6">Select a chat from the sidebar or create a new one to get started.</p>
							<div class="space-y-2">
								<button
									onclick={() => {selectedView = 'chats'; createNewChat();}}
									class="block w-full px-4 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700"
								>
									Create New Chat
								</button>
								<button
									onclick={() => selectedView = 'contacts'}
									class="block w-full px-4 py-2 bg-green-600 text-white rounded-lg hover:bg-green-700"
								>
									Browse Contacts
								</button>
							</div>
						</div>
					</div>
				{/if}
			</div>
		</div>
	</div>
</div>
