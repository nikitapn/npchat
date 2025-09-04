<script lang="ts">
  import WebRTCCall from './lib/components/WebRTCCall.svelte';
  import LoginForm from './lib/components/LoginForm.svelte';
  import RegisterForm from './lib/components/RegisterForm.svelte';
  import ChatApp from './lib/components/ChatApp.svelte';
  import LanguageSelector from './lib/components/LanguageSelector.svelte';
  import { router } from './lib/Router.svelte';
  import { authService } from './lib/rpc/services/Auth';
  import { _ } from './lib/i18n';

  // Subscribe to auth state
  let authState = $state(authService.authState);
  authService.subscribe((state) => {
    authState = state;
  });

  // Initialize auth service and try auto-login on app start
  $effect(() => {
    authService.initialize();
  });
</script>

<svelte:head>
  <title>{$_('app.title')} WebRTC App</title>
  <meta name="description" content="A simple WebRTC chat application" />
  <link rel="icon" href="/favicon.svg" />
</svelte:head>

{#if authState.isAuthenticated}
  <!-- Authenticated user - show main app -->
  {#if router.currentPath === '/webrtc'}
    <WebRTCCall />
  {:else if router.currentPath === '/chat'}
    <ChatApp />
  {:else}
    <!-- Main dashboard -->
    <div class="bg-gray-50">
      <!-- Header with language selector -->
      <div class="bg-white shadow-sm">
        <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div class="flex justify-between items-center py-4">
            <h1 class="text-xl font-semibold text-gray-900">{$_('app.title')}</h1>
            <div class="flex items-center space-x-4">
              <span class="text-sm text-gray-600">{$_('app.welcome')}, {authState.userData?.name}!</span>
              <LanguageSelector />
              <button
                onclick={() => authService.logout()}
                class="text-sm text-red-600 hover:text-red-800"
              >
                {$_('auth.logout')}
              </button>
            </div>
          </div>
        </div>
      </div>

      <!-- Main content -->
      <div class="flex flex-col justify-center py-12 sm:px-6 lg:px-8">
        <div class="sm:mx-auto sm:w-full sm:max-w-md">
          <h2 class="mt-6 text-center text-3xl font-extrabold text-gray-900">
            {$_('app.welcome')}, {authState.userData?.name}!
          </h2>
          <p class="mt-2 text-center text-sm text-gray-600">
            {$_('app.title')} WebRTC Application
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
              </button>

              <button
                onclick={() => authService.logout()}
                class="w-full flex justify-center py-2 px-4 border border-gray-300 rounded-md shadow-sm text-sm font-medium text-gray-700 bg-white hover:bg-gray-50 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-indigo-500 transition-colors duration-200"
              >
                {$_('auth.logout')}
              </button>
            </div>
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
    <div class="min-h-screen bg-gray-50">
      <!-- Header with language selector -->
      <div class="bg-white shadow-sm">
        <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div class="flex justify-between items-center py-4">
            <h1 class="text-xl font-semibold text-gray-900">{$_('app.title')}</h1>
            <LanguageSelector />
          </div>
        </div>
      </div>

      <!-- Main content -->
      <div class="flex flex-col justify-center py-12 sm:px-6 lg:px-8">
        <div class="sm:mx-auto sm:w-full sm:max-w-md">
          <h2 class="mt-6 text-center text-3xl font-extrabold text-gray-900">
            {$_('app.title')} WebRTC App
          </h2>
          <p class="mt-2 text-center text-sm text-gray-600">
            {$_('app.welcome')} to your WebRTC chat application
          </p>
        </div>

        <div class="mt-8 sm:mx-auto sm:w-full sm:max-w-md">
          <div class="bg-white py-8 px-4 shadow sm:rounded-lg sm:px-10">
            <div class="space-y-4">
              <button
                onclick={() => router.navigate('/login')}
                class="w-full flex justify-center py-2 px-4 border border-transparent rounded-md shadow-sm text-sm font-medium text-white bg-indigo-600 hover:bg-indigo-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-indigo-500 transition-colors duration-200"
              >
                {$_('auth.loginButton')}
              </button>

              <button
                onclick={() => router.navigate('/register')}
                class="w-full flex justify-center py-2 px-4 border border-gray-300 rounded-md shadow-sm text-sm font-medium text-gray-700 bg-white hover:bg-gray-50 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-indigo-500 transition-colors duration-200"
              >
                {$_('auth.registerButton')}
              </button>
            </div>
          </div>
        </div>
      </div>
    </div>
  {/if}
{/if}
