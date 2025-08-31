// Simple hash-based router for Svelte 5
export function createRouter() {
  let currentPath = $state(window.location.hash.slice(1) || '/');
  
  function updatePath() {
    currentPath = window.location.hash.slice(1) || '/';
  }
  
  // Set up event listeners
  if (typeof window !== 'undefined') {
    window.addEventListener('hashchange', updatePath);
    window.addEventListener('load', updatePath);
  }
  
  function navigate(path: string) {
    window.location.hash = path;
  }
  
  return {
    get currentPath() { return currentPath; },
    navigate
  };
}

// Create global router instance
export const router = createRouter();
