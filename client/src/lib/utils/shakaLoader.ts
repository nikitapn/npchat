// Deferred Shaka Player loader to avoid loading heavy library unless needed
let shakaPromise: Promise<any> | null = null;
let isLoaded = false;

declare global {
  interface Window {
    shaka: any;
  }
}

export async function loadShaka() {
  // Return cached instance if already loaded
  if (isLoaded && window.shaka) {
    return window.shaka;
  }

  // Return cached promise if already loading
  if (shakaPromise) {
    return shakaPromise;
  }

  console.log('Loading Shaka Player...');

  // Create the loading promise
  shakaPromise = new Promise((resolve, reject) => {
    // Check if Shaka is already available
    if (window.shaka) {
      isLoaded = true;
      resolve(window.shaka);
      return;
    }

    // Create script element to load Shaka from CDN
    const script = document.createElement('script');
    script.src = 'https://cdnjs.cloudflare.com/ajax/libs/shaka-player/4.3.6/shaka-player.compiled.min.js';
    script.async = true;

    script.onload = () => {
      if (window.shaka) {
        // Install polyfills and configure Shaka
        try {
          window.shaka.polyfill.installAll();

          if (window.shaka.Player.isBrowserSupported()) {
            console.log('Shaka Player loaded and supported');
            isLoaded = true;
            resolve(window.shaka);
          } else {
            reject(new Error('Browser does not support Shaka Player'));
          }
        } catch (error) {
          reject(error);
        }
      } else {
        reject(new Error('Shaka Player failed to load'));
      }
    };

    script.onerror = () => {
      reject(new Error('Failed to load Shaka Player script'));
    };

    document.head.appendChild(script);
  });

  return shakaPromise;
}

export function isShakaLoaded(): boolean {
  return isLoaded && !!window.shaka;
}

export function getShakaPlayer() {
  return window.shaka;
}

// Video format detection utilities
export function isVideoFile(file: File): boolean {
  const videoTypes = [
    'video/mp4',
    'video/webm',
    'video/ogg',
    'video/avi',
    'video/mov',
    'video/wmv',
    'video/flv',
    'video/mkv'
  ];
  return videoTypes.includes(file.type);
}

export function getSupportedVideoFormats(): string[] {
  return [
    'video/mp4',
    'video/webm',
    'video/ogg'
  ];
}

export function canPlayVideo(mimeType: string): boolean {
  const video = document.createElement('video');
  return video.canPlayType(mimeType) !== '';
}

// Check if a format requires Shaka Player (adaptive streaming)
export function requiresShaka(mimeType: string, fileName: string): boolean {
  // DASH manifest
  if (mimeType === 'application/dash+xml' || fileName.endsWith('.mpd')) {
    return true;
  }
  // HLS manifest
  if (mimeType === 'application/x-mpegURL' || mimeType === 'application/vnd.apple.mpegurl' || fileName.endsWith('.m3u8')) {
    return true;
  }
  // Smooth Streaming
  if (mimeType === 'application/vnd.ms-sstr+xml' || fileName.endsWith('.ism')) {
    return true;
  }
  // Other complex formats that benefit from Shaka
  const complexFormats = ['video/mp2t', 'video/x-flv'];
  return complexFormats.includes(mimeType);
}
