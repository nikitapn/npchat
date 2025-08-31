<script lang="ts">
	import type { RegisteredUser, Contact, ContactList } from '../rpc/npchat';
	import { authService } from '../rpc/services/auth';

	interface ContactListProps {
		registeredUser: RegisteredUser | null;
		onCreateChat: (contact: Contact) => void;
	}

	let { registeredUser, onCreateChat }: ContactListProps = $props();

	let contacts: Contact[] = $state([]);
	let searchQuery = $state('');
	let searchResults: Contact[] = $state([]);
	let isSearching = $state(false);
	let showAddContact = $state(false);

	// Load contacts when component initializes
	async function loadContacts() {
		if (!registeredUser) return;
		
		try {
			const contactList = await registeredUser.GetContacts();
			contacts = contactList;
			console.log('Loaded contacts:', contacts);
		} catch (error) {
			console.error('Failed to load contacts:', error);
		}
	}

	// Search for users to add as contacts
	async function searchUsers() {
		if (!registeredUser || !searchQuery.trim()) {
			searchResults = [];
			return;
		}

		try {
			isSearching = true;
			const results = await registeredUser.SearchUsers(searchQuery, 20);
			searchResults = results;
			console.log('Search results:', results);
		} catch (error) {
			console.error('Failed to search users:', error);
			searchResults = [];
		} finally {
			isSearching = false;
		}
	}

	// Add a user as contact
	async function addContact(contact: Contact) {
		if (!registeredUser) return;

		try {
			await registeredUser.AddContact(contact.id);
			console.log('Added contact:', contact.username);
			
			// Refresh contacts list
			await loadContacts();
			
			// Clear search
			searchQuery = '';
			searchResults = [];
		} catch (error) {
			console.error('Failed to add contact:', error);
		}
	}

	// Remove a contact
	async function removeContact(contact: Contact) {
		if (!registeredUser) return;

		try {
			await registeredUser.RemoveContact(contact.id);
			console.log('Removed contact:', contact.username);
			
			// Refresh contacts list
			await loadContacts();
		} catch (error) {
			console.error('Failed to remove contact:', error);
		}
	}

	// Handle search input with debounce
	let searchTimeout: number;
	function handleSearchInput() {
		clearTimeout(searchTimeout);
		searchTimeout = setTimeout(searchUsers, 300);
	}

	// Load contacts when registeredUser becomes available
	$effect(() => {
		if (registeredUser) {
			loadContacts();
		}
	});
</script>

<div class="bg-white rounded-lg shadow p-4">
	<div class="flex justify-between items-center mb-4">
		<h3 class="text-lg font-semibold text-gray-900">Contacts</h3>
		<button
			onclick={() => showAddContact = !showAddContact}
			class="px-3 py-1 bg-blue-600 text-white rounded-md hover:bg-blue-700 text-sm"
		>
			{showAddContact ? 'Cancel' : 'Add Contact'}
		</button>
	</div>

	{#if showAddContact}
		<!-- Add Contact Section -->
		<div class="mb-4 p-3 bg-gray-50 rounded-lg">
			<div class="mb-3">
				<input
					bind:value={searchQuery}
					oninput={handleSearchInput}
					placeholder="Search users to add..."
					class="w-full border border-gray-300 rounded-lg px-3 py-2 focus:outline-none focus:ring-2 focus:ring-blue-500"
				/>
			</div>

			{#if isSearching}
				<div class="text-center text-gray-500 py-2">Searching...</div>
			{:else if searchResults.length > 0}
				<div class="space-y-2 max-h-40 overflow-y-auto">
					{#each searchResults as user}
						<div class="flex justify-between items-center p-2 bg-white rounded border">
							<div class="flex items-center space-x-2">
								<div class="w-8 h-8 bg-gray-300 rounded-full flex items-center justify-center">
									<span class="text-sm font-medium text-gray-600">
										{user.username.charAt(0).toUpperCase()}
									</span>
								</div>
								<span class="text-sm font-medium">{user.username}</span>
							</div>
							<button
								onclick={() => addContact(user)}
								class="px-2 py-1 bg-green-600 text-white rounded text-xs hover:bg-green-700"
							>
								Add
							</button>
						</div>
					{/each}
				</div>
			{:else if searchQuery.trim()}
				<div class="text-center text-gray-500 py-2">No users found</div>
			{/if}
		</div>
	{/if}

	<!-- Contacts List -->
	<div class="space-y-2">
		{#each contacts as contact}
			<div class="flex justify-between items-center p-3 bg-gray-50 rounded-lg hover:bg-gray-100">
				<div class="flex items-center space-x-3">
					<div class="w-10 h-10 bg-blue-500 rounded-full flex items-center justify-center">
						{#if contact.avatar}
							<img 
								src={`data:image/jpeg;base64,${btoa(String.fromCharCode(...contact.avatar))}`}
								alt={contact.username}
								class="w-full h-full rounded-full object-cover"
							/>
						{:else}
							<span class="text-white font-medium">
								{contact.username.charAt(0).toUpperCase()}
							</span>
						{/if}
					</div>
					<div>
						<div class="font-medium text-gray-900">{contact.username}</div>
						<div class="text-sm text-gray-500">ID: {contact.id}</div>
					</div>
				</div>
				
				<div class="flex space-x-2">
					<button
						onclick={() => onCreateChat(contact)}
						class="px-3 py-1 bg-blue-600 text-white rounded text-sm hover:bg-blue-700"
					>
						Chat
					</button>
					<button
						onclick={() => removeContact(contact)}
						class="px-3 py-1 bg-red-600 text-white rounded text-sm hover:bg-red-700"
					>
						Remove
					</button>
				</div>
			</div>
		{:else}
			<div class="text-center text-gray-500 py-4">
				No contacts yet. Add some contacts to start chatting!
			</div>
		{/each}
	</div>
</div>
