<script lang="ts">
  import { locale } from '../i18n';

  const languages = [
    { code: 'en', name: 'English', flag: '🇺🇸' },
    { code: 'ru', name: 'Русский', flag: '🇷🇺' },
    { code: 'es', name: 'Español', flag: '🇪🇸' },
    { code: 'fr', name: 'Français', flag: '🇫🇷' },
    { code: 'de', name: 'Deutsch', flag: '🇩🇪' }
  ];

  let showDropdown = $state(false);
  let currentLocale = $state('en');

  // Subscribe to locale changes
  $effect(() => {
    const unsubscribe = locale.subscribe((value: string | null | undefined) => {
      currentLocale = value || 'en';
    });
    return unsubscribe;
  });

  function setLanguage(lang: string) {
    locale.set(lang);
    showDropdown = false;
    // Persist language preference
    if (typeof localStorage !== 'undefined') {
      localStorage.setItem('preferred-language', lang);
    }
  }

  function getCurrentLanguage() {
    return languages.find(lang => lang.code === currentLocale) || languages[0];
  }

  // Load saved language preference on mount
  $effect(() => {
    if (typeof localStorage !== 'undefined') {
      const savedLang = localStorage.getItem('preferred-language');
      if (savedLang && languages.some(lang => lang.code === savedLang)) {
        locale.set(savedLang);
      }
    }
  });

  // Close dropdown when clicking outside
  function handleDocumentClick() {
    if (showDropdown) {
      showDropdown = false;
    }
  }

  function handleKeydown(event: KeyboardEvent) {
    if (showDropdown && event.key === 'Escape') {
      showDropdown = false;
    }
  }

  $effect(() => {
    if (showDropdown) {
      document.addEventListener('click', handleDocumentClick);
      document.addEventListener('keydown', handleKeydown);
      return () => {
        document.removeEventListener('click', handleDocumentClick);
        document.removeEventListener('keydown', handleKeydown);
      };
    }
  });
</script>

<div class="relative inline-block text-left">
  <button
    onclick={(e) => {
      e.stopPropagation();
      showDropdown = !showDropdown;
    }}
    class="inline-flex items-center px-3 py-2 text-sm font-medium text-gray-700 bg-white border border-gray-300 rounded-md hover:bg-gray-50 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500"
    title="Change language"
  >
    <span class="mr-2">{getCurrentLanguage().flag}</span>
    <span class="hidden sm:inline">{getCurrentLanguage().name}</span>
    <svg class="w-4 h-4 ml-2" fill="currentColor" viewBox="0 0 20 20">
      <path fill-rule="evenodd" d="M5.293 7.293a1 1 0 011.414 0L10 10.586l3.293-3.293a1 1 0 111.414 1.414l-4 4a1 1 0 01-1.414 0l-4-4a1 1 0 010-1.414z" clip-rule="evenodd" />
    </svg>
  </button>

  {#if showDropdown}
    <div
      class="absolute right-0 z-50 mt-2 w-48 bg-white border border-gray-200 rounded-md shadow-lg"
      role="menu"
      tabindex="-1"
      onclick={(e) => e.stopPropagation()}
      onkeydown={handleKeydown}
    >
      <div class="py-1">
        {#each languages as language}
          <button
            onclick={() => setLanguage(language.code)}
            class="flex items-center w-full px-4 py-2 text-sm text-gray-700 hover:bg-gray-100 hover:text-gray-900 {
              currentLocale === language.code ? 'bg-blue-50 text-blue-900' : ''
            }"
            role="menuitem"
          >
            <span class="mr-3">{language.flag}</span>
            <span>{language.name}</span>
            {#if currentLocale === language.code}
              <svg class="w-4 h-4 ml-auto text-blue-600" fill="currentColor" viewBox="0 0 20 20">
                <path fill-rule="evenodd" d="M16.707 5.293a1 1 0 010 1.414l-8 8a1 1 0 01-1.414 0l-4-4a1 1 0 011.414-1.414L8 12.586l7.293-7.293a1 1 0 011.414 0z" clip-rule="evenodd" />
              </svg>
            {/if}
          </button>
        {/each}
      </div>
    </div>
  {/if}
</div>
