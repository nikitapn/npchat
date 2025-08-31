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

<div class="video-player-container rounded-lg overflow-hidden bg-gray-100">
	<!-- Video Element - Always present for binding but hidden when not needed -->
	<div class="relative bg-black rounded-t-lg overflow-hidden">
		<video
			bind:this={videoElement}
			controls={showControls}
			class="w-full max-h-96 bg-black"
			class:hidden={isLoading || hasError}
			preload="metadata"
			controlslist="download"
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
		{/if}
	</div>

	<!-- Video Info Below Video (not overlay) -->
	{#if !isLoading && !hasError}
		<div class="bg-gray-50 px-4 py-3 rounded-b-lg border-t border-gray-200">
			<div class="flex items-center justify-between">
				<div class="text-gray-800 text-sm flex-1 min-w-0">
					<div class="font-medium truncate">{fileName}</div>
					<div class="text-xs text-gray-500">{formatFileSize(videoData.length)}</div>
				</div>
				<button
					onclick={downloadVideo}
					class="ml-3 px-3 py-1.5 bg-blue-600 text-white rounded text-sm hover:bg-blue-700 transition-colors flex items-center gap-1"
				>
					<svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
						<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 10v6m0 0l-3-3m3 3l3-3m2 8H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z" />
					</svg>
					Download
				</button>
			</div>
		</div>
	{/if}
</div>

<style>
	.video-player-container {
		max-width: 500px;
		box-shadow: 0 1px 3px 0 rgb(0 0 0 / 0.1), 0 1px 2px -1px rgb(0 0 0 / 0.1);
	}
	
	video {
		outline: none;
	}
	
	video::-webkit-media-controls-panel {
		background-color: rgba(0, 0, 0, 0.8);
	}
	
	video::-webkit-media-controls-play-button,
	video::-webkit-media-controls-pause-button {
		background-color: rgba(255, 255, 255, 0.9);
		border-radius: 50%;
	}
	
	video::-webkit-media-controls-volume-slider,
	video::-webkit-media-controls-timeline {
		background-color: rgba(255, 255, 255, 0.3);
	}
	
	video::-webkit-media-controls-current-time-display,
	video::-webkit-media-controls-time-remaining-display {
		color: white;
		text-shadow: 1px 1px 1px rgba(0, 0, 0, 0.8);
	}
</style>
