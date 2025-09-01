import { init, register } from 'svelte-i18n';

const defaultLocale = 'en';

// Register locales
register('en', () => import('./locales/en.json'));
register('ru', () => import('./locales/ru.json'));
register('es', () => import('./locales/es.json'));
register('fr', () => import('./locales/fr.json'));
register('de', () => import('./locales/de.json'));

// Detect browser locale
const getBrowserLocale = (): string => {
  if (typeof window !== 'undefined' && window.navigator) {
    const locale = window.navigator.language.split('-')[0];
    return ['en', 'ru', 'es', 'fr', 'de'].includes(locale) ? locale : defaultLocale;
  }
  return defaultLocale;
};

// Get saved language preference
const getSavedLocale = (): string => {
  if (typeof localStorage !== 'undefined') {
    const savedLang = localStorage.getItem('preferred-language');
    if (savedLang && ['en', 'ru', 'es', 'fr', 'de'].includes(savedLang)) {
      return savedLang;
    }
  }
  return getBrowserLocale();
};

// Initialize i18n and return a Promise
export const initI18n = (): Promise<void> => {
  const initialLocale = getSavedLocale();

  return Promise.resolve(init({
    fallbackLocale: defaultLocale,
    initialLocale: initialLocale,
  }));
};

// Export commonly used functions for easier imports
export { _, locale, locales } from 'svelte-i18n';
