<script lang="ts">
	import { onDestroy } from 'svelte';
	import { loadShaka, requiresShaka } from '../utils/shakaLoader';

	interface VideoPlayerProps {
		videoData: Uint8Array;
		fileName: string;
		mimeType?: string;
	}

	let { videoData, fileName, mimeType = 'video/mp4' }: VideoPlayerProps = $props();

	let videoElement: HTMLVideoElement;
	let shakaPlayer: any = null;
	let isLoading = $state(true);
	let hasError = $state(false);
	let errorMessage = $state('');
	let showControls = $state(false);
	let videoUrl = $state('');
  let isPlayerLoaded = $state(false);

	// Create blob URL for video data
	function createVideoUrl(): string {
		const blob = new Blob([videoData], { type: mimeType });
		return URL.createObjectURL(blob);
	}

	// Initialize video player with Shaka if needed
	async function initializePlayer() {
		try {
			isLoading = true;
			hasError = false;
			
			// Create video URL
			videoUrl = createVideoUrl();
			
			// Check if this format requires Shaka Player
			if (requiresShaka(mimeType, fileName)) {
				console.log('Loading Shaka Player for adaptive streaming format:', mimeType);
				const shaka = await loadShaka();
				
				if (shaka && shaka.Player) {
					shakaPlayer = new shaka.Player(videoElement);
					
					// Configure player for streaming
					shakaPlayer.configure({
						streaming: {
							bufferingGoal: 30,
							rebufferingGoal: 5,
						}
					});

					// Load the video
					await shakaPlayer.load(videoUrl);
					showControls = true;
					console.log('Video loaded with Shaka Player');
				} else {
					throw new Error('Failed to load Shaka Player');
				}
			} else {
				// For all other formats, try native HTML5 video first
				console.log('Using native HTML5 video player for:', mimeType);
				videoElement.src = videoUrl;
				
				// Listen for load errors to potentially fallback to Shaka
				videoElement.addEventListener('error', async (e) => {
					console.warn('Native player failed, trying Shaka Player as fallback');
					try {
						const shaka = await loadShaka();
						if (shaka && shaka.Player) {
							shakaPlayer = new shaka.Player(videoElement);
							await shakaPlayer.load(videoUrl);
							showControls = true;
							console.log('Video loaded with Shaka Player fallback');
						}
					} catch (shakaError) {
						console.error('Both native and Shaka players failed:', shakaError);
						hasError = true;
						errorMessage = 'Unsupported video format';
					}
				});
				
				showControls = true;
			}
			
			isLoading = false;
		} catch (error) {
			console.error('Error initializing video player:', error);
			hasError = true;
			errorMessage = error instanceof Error ? error.message : 'Unknown error';
			isLoading = false;
		}
    isPlayerLoaded = true;
	}

	// Download video file
	function downloadVideo() {
		const blob = new Blob([videoData], { type: mimeType });
		const url = URL.createObjectURL(blob);
		const a = document.createElement('a');
		a.href = url;
		a.download = fileName;
		a.click();
		URL.revokeObjectURL(url);
	}

	// Format file size utility
	function formatFileSize(bytes: number): string {
		if (bytes === 0) return '0 Bytes';
		const k = 1024;
		const sizes = ['Bytes', 'KB', 'MB', 'GB'];
		const i = Math.floor(Math.log(bytes) / Math.log(k));
		return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
	}

	// Cleanup
	onDestroy(() => {
		if (shakaPlayer) {
			shakaPlayer.destroy();
		}
		if (videoUrl) {
			URL.revokeObjectURL(videoUrl);
		}
	});

	// Initialize when video element becomes available
	$effect(() => {
		if (videoElement && !isPlayerLoaded) {
			initializePlayer();
		}
	});
</script>

<div class="video-player-container rounded-lg overflow-hidden bg-black relative">
	<!-- Video Element - Always present for binding but hidden when not needed -->
	<video
		bind:this={videoElement}
		controls={showControls}
		class="w-full max-h-96 bg-black"
		class:hidden={isLoading || hasError}
		preload="metadata"
		poster="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'%3E%3Crect fill='%23000' width='100' height='100'/%3E%3Ctext y='50' x='50' text-anchor='middle' dominant-baseline='middle' fill='white' font-size='40'%3E▶%3C/text%3E%3C/svg%3E"
	>
		<track kind="captions" src="" label="No captions available" />
		Your browser does not support the video tag.
	</video>

	{#if isLoading}
		<div class="flex items-center justify-center h-48 bg-gray-100">
			<div class="text-center">
				<div class="animate-spin rounded-full h-8 w-8 border-b-2 border-blue-600 mx-auto mb-2"></div>
				<p class="text-sm text-gray-600">Loading video...</p>
			</div>
		</div>
	{:else if hasError}
		<div class="flex items-center justify-center h-48 bg-red-50">
			<div class="text-center">
				<p class="text-red-600 mb-2">Failed to load video</p>
				<p class="text-xs text-red-500 mb-3">{errorMessage}</p>
				<button
					onclick={downloadVideo}
					class="px-3 py-1 bg-red-600 text-white rounded text-sm hover:bg-red-700"
				>
					Download Video
				</button>
			</div>
		</div>
	{:else}
		<!-- Video Info Overlay -->
		<div class="absolute bottom-0 left-0 right-0 bg-gradient-to-t from-black/70 to-transparent p-3">
			<div class="flex items-center justify-between">
				<div class="text-white text-sm">
					<div class="font-medium">{fileName}</div>
					<div class="text-xs opacity-75">{formatFileSize(videoData.length)}</div>
				</div>
				<button
					onclick={downloadVideo}
					class="px-3 py-1 bg-white/20 text-white rounded text-sm hover:bg-white/30 transition-colors"
				>
					Download
				</button>
			</div>
		</div>
	{/if}
</div>

<style>
	.video-player-container {
		max-width: 500px;
	}
	
	video::-webkit-media-controls-panel {
		background-color: rgba(0, 0, 0, 0.8);
	}
</style>
