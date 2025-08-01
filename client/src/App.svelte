<script lang="ts">
	import WebRTCCall from './lib/components/WebRTCCall.svelte';
</script>

<svelte:head>
	<title>NPChat WebRTC App</title>
	<meta name="description" content="A simple WebRTC chat application" />
	<link rel="icon" href="/favicon.svg" />
</svelte:head>

<WebRTCCall />
<!--
<div class="min-h-screen bg-gray-50 flex flex-col justify-center py-12 sm:px-6 lg:px-8">
	<div class="sm:mx-auto sm:w-full sm:max-w-md">
		<h1 class="mt-6 text-center text-3xl font-extrabold text-gray-900">
			NPChat WebRTC App
		</h1>
		<p class="mt-2 text-center text-sm text-gray-600">
			Welcome to your WebRTC chat application
		</p>
	</div>

	<div class="mt-8 sm:mx-auto sm:w-full sm:max-w-md">
		<div class="bg-white py-8 px-4 shadow sm:rounded-lg sm:px-10">
			<div class="space-y-4">
				<a 
					href="/webrtc"
					class="w-full flex justify-center py-2 px-4 border border-transparent rounded-md shadow-sm text-sm font-medium text-white bg-green-600 hover:bg-green-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-green-500 transition-colors duration-200"
				>
					🎥 Test WebRTC Video Call
				</a>
				
				<a 
					href="/login"
					class="w-full flex justify-center py-2 px-4 border border-transparent rounded-md shadow-sm text-sm font-medium text-white bg-indigo-600 hover:bg-indigo-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-indigo-500 transition-colors duration-200"
				>
					Sign In
				</a>
				
				<a 
					href="/register"
					class="w-full flex justify-center py-2 px-4 border border-gray-300 rounded-md shadow-sm text-sm font-medium text-gray-700 bg-white hover:bg-gray-50 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-indigo-500 transition-colors duration-200"
				>
					Create Account
				</a>
			</div>
			
			<div class="mt-6">
				<div class="relative">
					<div class="absolute inset-0 flex items-center">
						<div class="w-full border-t border-gray-300" />
					</div>
					<div class="relative flex justify-center text-sm">
						<span class="px-2 bg-white text-gray-500">Features</span>
					</div>
				</div>
				
				<div class="mt-6 grid grid-cols-2 gap-3">
					<div class="text-center">
						<div class="mx-auto h-12 w-12 bg-indigo-100 rounded-lg flex items-center justify-center">
							<svg class="h-6 w-6 text-indigo-600" fill="none" stroke="currentColor" viewBox="0 0 24 24">
								<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15 10l4.553-2.276A1 1 0 0121 8.618v6.764a1 1 0 01-1.447.894L15 14M5 18h8a2 2 0 002-2V8a2 2 0 00-2-2H5a2 2 0 00-2 2v8a2 2 0 002 2z" />
							</svg>
						</div>
						<p class="mt-2 text-sm text-gray-600">Video Calls</p>
					</div>
					
					<div class="text-center">
						<div class="mx-auto h-12 w-12 bg-indigo-100 rounded-lg flex items-center justify-center">
							<svg class="h-6 w-6 text-indigo-600" fill="none" stroke="currentColor" viewBox="0 0 24 24">
								<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M8 12h.01M12 12h.01M16 12h.01M21 12c0 4.418-4.03 8-9 8a9.863 9.863 0 01-4.255-.949L3 20l1.395-3.72C3.512 15.042 3 13.574 3 12c0-4.418 4.03-8 9-8s9 3.582 9 8z" />
							</svg>
						</div>
						<p class="mt-2 text-sm text-gray-600">Real-time Chat</p>
					</div>
				</div>
			</div>
		</div>
	</div>
</div>
-->