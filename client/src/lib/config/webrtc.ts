export type IceServer = {
  urls: string | string[];
  username?: string;
  credential?: string;
};

export function getRtcConfig(): RTCConfiguration {
  const servers: IceServer[] = [];

  // Highest priority: a JSON array in VITE_WEBRTC_ICE_SERVERS
  const raw = (import.meta as any).env?.VITE_WEBRTC_ICE_SERVERS as string | undefined;
  if (raw) {
    try {
      const parsed = JSON.parse(raw);
      if (Array.isArray(parsed)) {
        parsed.forEach((s) => {
          if (s && s.urls) servers.push(s);
        });
      }
    } catch (e) {
      console.warn('VITE_WEBRTC_ICE_SERVERS is not valid JSON; falling back to individual vars');
    }
  }

  // Fallback: individual env vars
  const stunList = (import.meta as any).env?.VITE_WEBRTC_STUN_URLS as string | undefined; // comma-separated
  const turnUrl = (import.meta as any).env?.VITE_WEBRTC_TURN_URL as string | undefined;
  const turnUser = (import.meta as any).env?.VITE_WEBRTC_TURN_USERNAME as string | undefined;
  const turnCred = (import.meta as any).env?.VITE_WEBRTC_TURN_CREDENTIAL as string | undefined;

  if (stunList && servers.length === 0) {
    stunList
      .split(',')
      .map((s) => s.trim())
      .filter(Boolean)
      .forEach((u) => servers.push({ urls: u }));
  }

  if (turnUrl && turnUser && turnCred && !servers.some((s) => String(s.urls).includes('turn:'))) {
    servers.push({ urls: turnUrl, username: turnUser, credential: turnCred });
  }

  if (servers.length === 0) {
    // Safe default
    servers.push({ urls: 'stun:stun.l.google.com:19302' });
  }

  return { iceServers: servers } as RTCConfiguration;
}
