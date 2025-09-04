import type { RegisteredUser, ChatMessage, ChatId, MessageId, ContactList } from '../npchat';
import { _IChatListener_Servant } from '../npchat';
import { poa } from '../index';
import { authService } from './Auth';

interface SessionManager {
  // Enhanced session management with proper security
  setSecureSession(sessionId: string, userData: any): void;
  clearSecureSession(): void;
  getSessionFromSecureCookie(): string | null;
  isSessionValid(): boolean;
}

class SecureSessionManager implements SessionManager {
  private readonly SESSION_COOKIE = 'npchat_session_secure';
  private readonly CSRF_TOKEN_KEY = 'npchat_csrf_token';

  // Generate CSRF token for additional security
  private generateCSRFToken(): string {
    const array = new Uint8Array(32);
    crypto.getRandomValues(array);
    return Array.from(array, byte => byte.toString(16).padStart(2, '0')).join('');
  }

  setSecureSession(sessionId: string, userData: any): void {
    try {
      // Set HTTP-only cookie via server endpoint in production
      // For now, use secure client-side cookies with additional protections

      const csrfToken = this.generateCSRFToken();

      // Set session cookie with security flags
      const expires = new Date();
      expires.setTime(expires.getTime() + (30 * 24 * 60 * 60 * 1000)); // 30 days

      const cookieOptions = [
        `${this.SESSION_COOKIE}=${sessionId}`,
        `expires=${expires.toUTCString()}`,
        `path=/`,
        `SameSite=Strict`,
        location.protocol === 'https:' ? 'Secure' : ''
      ].filter(Boolean).join(';');

      document.cookie = cookieOptions;

      // Store CSRF token separately
      sessionStorage.setItem(this.CSRF_TOKEN_KEY, csrfToken);

      // Store non-sensitive user data
      localStorage.setItem('npchat_userdata', JSON.stringify({
        name: userData.name,
        lastLogin: Date.now(),
        csrfToken: csrfToken
      }));

      console.log('Secure session established with CSRF protection');
    } catch (error) {
      console.error('Failed to set secure session:', error);
    }
  }

  clearSecureSession(): void {
    // Clear session cookie
    document.cookie = `${this.SESSION_COOKIE}=;expires=Thu, 01 Jan 1970 00:00:00 UTC;path=/;`;

    // Clear related storage
    sessionStorage.removeItem(this.CSRF_TOKEN_KEY);
    localStorage.removeItem('npchat_userdata');

    console.log('Secure session cleared');
  }

  getSessionFromSecureCookie(): string | null {
    const nameEQ = this.SESSION_COOKIE + "=";
    const ca = document.cookie.split(';');

    for (let i = 0; i < ca.length; i++) {
      let c = ca[i];
      while (c.charAt(0) === ' ') c = c.substring(1, c.length);
      if (c.indexOf(nameEQ) === 0) {
        return c.substring(nameEQ.length, c.length);
      }
    }
    return null;
  }

  isSessionValid(): boolean {
    const sessionId = this.getSessionFromSecureCookie();
    const csrfToken = sessionStorage.getItem(this.CSRF_TOKEN_KEY);
    const userData = localStorage.getItem('npchat_userdata');

    if (!sessionId || !csrfToken) {
      return false;
    }

    // Validate CSRF token matches
    try {
      if (userData) {
        const parsedData = JSON.parse(userData);
        if (parsedData.csrfToken !== csrfToken) {
          console.warn('CSRF token mismatch detected');
          this.clearSecureSession();
          return false;
        }

        // Check session age (optional - could add server-side validation)
        const lastLogin = parsedData.lastLogin;
        const maxAge = 30 * 24 * 60 * 60 * 1000; // 30 days
        if (lastLogin && (Date.now() - lastLogin) > maxAge) {
          console.warn('Session expired');
          this.clearSecureSession();
          return false;
        }
      }
    } catch (error) {
      console.error('Session validation error:', error);
      this.clearSecureSession();
      return false;
    }

    return true;
  }

  getCSRFToken(): string | null {
    return sessionStorage.getItem(this.CSRF_TOKEN_KEY);
  }
}

// Export singleton for use in auth service
export const sessionManager = new SecureSessionManager();

// Additional security utilities
export class SecurityUtils {
  // Content Security Policy headers (for server-side implementation)
  static getCSPHeaders(): Record<string, string> {
    return {
      'Content-Security-Policy': `
        default-src 'self';
        script-src 'self' 'unsafe-inline';
        style-src 'self' 'unsafe-inline';
        connect-src 'self' ws: wss:;
        font-src 'self';
        img-src 'self' data:;
        frame-ancestors 'none';
      `.replace(/\s+/g, ' ').trim(),
      'X-Frame-Options': 'DENY',
      'X-Content-Type-Options': 'nosniff',
      'X-XSS-Protection': '1; mode=block',
      'Referrer-Policy': 'strict-origin-when-cross-origin'
    };
  }

  // Input sanitization
  static sanitizeInput(input: string): string {
    return input
      .replace(/[<>]/g, '') // Remove potential HTML tags
      .replace(/javascript:/gi, '') // Remove javascript: protocols
      .trim();
  }

  // Validate session ID format
  static isValidSessionId(sessionId: string): boolean {
    // Check if session ID follows expected format (adjust based on server implementation)
    const sessionIdRegex = /^[a-zA-Z0-9\-_]{20,}$/;
    return sessionIdRegex.test(sessionId);
  }
}

export default sessionManager;
