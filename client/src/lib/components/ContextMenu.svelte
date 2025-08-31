<script lang="ts">
	export interface ContextMenuItem {
		label: string;
		icon?: string;
		action: () => void;
		className?: string;
		disabled?: boolean;
		separator?: boolean;
	}

	interface ContextMenuProps {
		visible: boolean;
		x: number;
		y: number;
		items: ContextMenuItem[];
		title?: string;
		onClose?: () => void;
	}

	let { visible, x, y, items, title, onClose }: ContextMenuProps = $props();

	// Close context menu when clicking outside
	function handleDocumentClick(event: MouseEvent) {
		if (visible && onClose) {
			onClose();
		}
	}

	// Handle keyboard events
	function handleKeydown(event: KeyboardEvent) {
		if (visible && event.key === 'Escape' && onClose) {
			onClose();
		}
	}

	// Add document listeners when visible
	$effect(() => {
		if (visible) {
			document.addEventListener('click', handleDocumentClick);
			document.addEventListener('keydown', handleKeydown);
			return () => {
				document.removeEventListener('click', handleDocumentClick);
				document.removeEventListener('keydown', handleKeydown);
			};
		}
	});

	// Handle item click
	function handleItemClick(item: ContextMenuItem) {
		if (!item.disabled) {
			item.action();
		}
	}

	// Default SVG icons
	const defaultIcons: Record<string, string> = {
		open: 'M8 12h.01M12 12h.01M16 12h.01M21 12c0 4.418-4.03 8-9 8a9.863 9.863 0 01-4.255-.949L3 20l1.395-3.72C3.512 15.042 3 13.574 3 12c0-4.418 4.03-8 9-8s9 3.582 9 8z',
		leave: 'M17 16l4-4m0 0l-4-4m4 4H7m6 4v1a3 3 0 01-3 3H6a3 3 0 01-3-3V7a3 3 0 013-3h4a3 3 0 013 3v1',
		delete: 'M19 7l-.867 12.142A2 2 0 0116.138 21H7.862a2 2 0 01-1.995-1.858L5 7m5 4v6m4-6v6m1-10V4a1 1 0 00-1-1h-4a1 1 0 00-1 1v3M4 7h16',
		cancel: 'M6 18L18 6M6 6l12 12',
		edit: 'M11 5H6a2 2 0 00-2 2v11a2 2 0 002 2h11a2 2 0 002-2v-5m-1.414-9.414a2 2 0 112.828 2.828L11.828 15H9v-2.828l8.586-8.586z'
	};
</script>

{#if visible}
	<div 
		class="fixed bg-white border border-gray-200 rounded-lg shadow-lg py-2 z-50 min-w-48"
		style="left: {x}px; top: {y}px;"
		role="menu"
		tabindex="-1"
		onclick={(e) => e.stopPropagation()}
		onkeydown={handleKeydown}
	>
		{#if title}
			<div class="px-3 py-1 text-xs text-gray-500 border-b border-gray-100 mb-1">
				{title}
			</div>
		{/if}
		
		{#each items as item, i (i)}
			{#if item.separator}
				<div class="border-t border-gray-100 my-1"></div>
			{:else}
				<button
					class="w-full px-4 py-2 text-left text-sm flex items-center space-x-2 transition-colors {
						item.disabled 
							? 'text-gray-400 cursor-not-allowed' 
							: item.className || 'text-gray-700 hover:bg-gray-100'
					}"
					onclick={() => handleItemClick(item)}
					disabled={item.disabled}
					role="menuitem"
				>
					{#if item.icon}
						<svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
							<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" 
								d={defaultIcons[item.icon] || item.icon} />
						</svg>
					{/if}
					<span>{item.label}</span>
				</button>
			{/if}
		{/each}
	</div>
{/if}
