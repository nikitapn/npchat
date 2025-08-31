<script lang="ts">
	import WebRTCCall from './lib/components/WebRTCCall.svelte';
	import LoginForm from './lib/components/LoginForm.svelte';
	import RegisterForm from './lib/components/RegisterForm.svelte';
	import ChatRoom from './lib/components/ChatRoom.svelte';
	import { router } from './lib/Router.svelte';
	import { authService } from './lib/rpc/services/auth';
	
	// Subscribe to auth state
	let authState = $state(authService.authState);
	authService.subscribe((state) => {
		authState = state;
	});

	// Try auto-login on app start
	authService.tryAutoLogin();
</script>

<svelte:head>
	<title>NPChat WebRTC App</title>
	<meta name="description" content="A simple WebRTC chat application" />
	<link rel="icon" href="/favicon.svg" />
</svelte:head>

{#if authState.isAuthenticated}
	<!-- Authenticated user - show main app -->
	{#if router.currentPath === '/webrtc'}
		<WebRTCCall />
	{:else if router.currentPath === '/chat'}
		<ChatRoom />
	{:else}
		<!-- Main dashboard -->
		<div class="min-h-screen bg-gray-50 flex flex-col justify-center py-12 sm:px-6 lg:px-8">
			<div class="sm:mx-auto sm:w-full sm:max-w-md">
				<h1 class="mt-6 text-center text-3xl font-extrabold text-gray-900">
					Welcome, {authState.user?.name}!
				</h1>
				<p class="mt-2 text-center text-sm text-gray-600">
					NPChat WebRTC Application
				</p>
			</div>

			<div class="mt-8 sm:mx-auto sm:w-full sm:max-w-md">
				<div class="bg-white py-8 px-4 shadow sm:rounded-lg sm:px-10">
				<div class="space-y-4">
					<button 
						onclick={() => router.navigate('/chat')}
						class="w-full flex justify-center py-2 px-4 border border-transparent rounded-md shadow-sm text-sm font-medium text-white bg-blue-600 hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500 transition-colors duration-200"
					>
						💬 Chat Room
					</button>
					
					<button 
						onclick={() => router.navigate('/webrtc')}
						class="w-full flex justify-center py-2 px-4 border border-transparent rounded-md shadow-sm text-sm font-medium text-white bg-green-600 hover:bg-green-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-green-500 transition-colors duration-200"
					>
						🎥 Start Video Call
					</button>						<button 
							onclick={() => authService.logout()}
							class="w-full flex justify-center py-2 px-4 border border-gray-300 rounded-md shadow-sm text-sm font-medium text-gray-700 bg-white hover:bg-gray-50 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-indigo-500 transition-colors duration-200"
						>
							Sign Out
						</button>
					</div>
				</div>
			</div>
		</div>
	{/if}
{:else}
	<!-- Not authenticated - show login/register -->
	{#if router.currentPath === '/register'}
		<RegisterForm />
	{:else if router.currentPath === '/login' || router.currentPath === '/'}
		<LoginForm />
	{:else}
		<!-- Default landing page -->
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
						<button 
							onclick={() => router.navigate('/login')}
							class="w-full flex justify-center py-2 px-4 border border-transparent rounded-md shadow-sm text-sm font-medium text-white bg-indigo-600 hover:bg-indigo-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-indigo-500 transition-colors duration-200"
						>
							Sign In
						</button>
						
						<button 
							onclick={() => router.navigate('/register')}
							class="w-full flex justify-center py-2 px-4 border border-gray-300 rounded-md shadow-sm text-sm font-medium text-gray-700 bg-white hover:bg-gray-50 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-indigo-500 transition-colors duration-200"
						>
							Create Account
						</button>
					</div>
				</div>
			</div>
		</div>
	{/if}
{/if}