import * as NPRPC from 'nprpc';
import { Authorizator, AuthorizationFailed, RegistrationFailed, AuthorizationError, RegistrationError } from '../npchat';
import type { UserData } from '../npchat';
import { RegisteredUser } from '../npchat';
import { authorizator } from '../index';

export interface AuthState {
  isAuthenticated: boolean;
  userData: UserData | null;
  user: RegisteredUser | null;
  sessionId: string | null;
}

export class AuthService {
  private _authState: AuthState = {
    isAuthenticated: false,
    userData: null,
    user: null,
    sessionId: null
  };

  private listeners: Set<(state: AuthState) => void> = new Set();

  constructor() {
    this.loadSessionFromStorage();
  }

  // Subscribe to auth state changes
  subscribe(listener: (state: AuthState) => void): () => void {
    this.listeners.add(listener);
    // Immediately call with current state
    listener(this._authState);
    // Return unsubscribe function
    return () => this.listeners.delete(listener);
  }

  get authState(): AuthState {
    return { ...this._authState };
  }

  private notifyListeners() {
    this.listeners.forEach(listener => listener(this._authState));
  }

  private updateAuthState(updates: Partial<AuthState>) {
    this._authState = { ...this._authState, ...updates };
    this.notifyListeners();
  }

  private saveSessionToStorage() {
    if (this._authState.sessionId && this._authState.user) {
      localStorage.setItem('npchat_session', JSON.stringify({
        sessionId: this._authState.sessionId,
        user: this._authState.user
      }));
    }
  }

  private loadSessionFromStorage() {
    try {
      const stored = localStorage.getItem('npchat_session');
      if (stored) {
        const data = JSON.parse(stored);
        this.updateAuthState({
          isAuthenticated: true,
          user: data.user,
          sessionId: data.sessionId
        });
      }
    } catch (error) {
      console.warn('Failed to load session from storage:', error);
      localStorage.removeItem('npchat_session');
    }
  }

  private clearSessionFromStorage() {
    localStorage.removeItem('npchat_session');
  }

  async login(login: string, password: string): Promise<UserData> {
    if (!authorizator) {
      throw new Error('Authentication service not available - RPC not initialized');
    }

    try {
      const userData = await authorizator.LogIn(login, password);
      console.log({userData});
      const userProxy = NPRPC.narrow(userData.registeredUser, RegisteredUser);

      this.updateAuthState({
        isAuthenticated: true,
        userData: userData,
        user: userProxy,
        sessionId: userData.sessionId
      });

      this.saveSessionToStorage();
      return userData;
    } catch (error) {
      if (error instanceof AuthorizationFailed) {
        switch (error.reason) {
          case AuthorizationError.InvalidCredentials:
            throw new Error('Invalid username or password');
          case AuthorizationError.AccessDenied:
            throw new Error('Access denied');
          default:
            throw new Error('Authentication failed');
        }
      }
      throw error;
    }
  }

  async loginWithSession(sessionId: string): Promise<UserData> {
    if (!authorizator) {
      throw new Error('Authentication service not available - RPC not initialized');
    }

    try {
      const userData = await authorizator.LogInWithSessionId(sessionId);
      const userProxy = NPRPC.narrow(userData.registeredUser, RegisteredUser);

      this.updateAuthState({
        isAuthenticated: true,
        userData: userData,
        user: userProxy,
        sessionId: userData.sessionId
      });

      this.saveSessionToStorage();
      return userData;
    } catch (error) {
      // Clear invalid session
      this.clearSessionFromStorage();
      this.updateAuthState({
        isAuthenticated: false,
        user: null,
        sessionId: null
      });
      
      if (error instanceof AuthorizationFailed) {
        throw new Error('Session expired or invalid');
      }
      throw error;
    }
  }

  async logout(): Promise<boolean> {
    if (!authorizator || !this._authState.sessionId) {
      this.updateAuthState({
        isAuthenticated: false,
        user: null,
        sessionId: null
      });
      this.clearSessionFromStorage();
      return true;
    }

    try {
      const success = await authorizator.LogOut(this._authState.sessionId);
      
      this.updateAuthState({
        isAuthenticated: false,
        user: null,
        sessionId: null
      });
      
      this.clearSessionFromStorage();
      return success;
    } catch (error) {
      console.error('Logout error:', error);
      // Even if logout fails on server, clear local state
      this.updateAuthState({
        isAuthenticated: false,
        user: null,
        sessionId: null
      });
      this.clearSessionFromStorage();
      return false;
    }
  }

  async checkUsername(username: string): Promise<boolean> {
    if (!authorizator) {
      throw new Error('Authentication service not available - RPC not initialized');
    }

    return await authorizator.CheckUsername(username);
  }

  async checkEmail(email: string): Promise<boolean> {
    if (!authorizator) {
      throw new Error('Authentication service not available - RPC not initialized');
    }

    return await authorizator.CheckEmail(email);
  }

  async registerStepOne(username: string, email: string, password: string): Promise<void> {
    if (!authorizator) {
      throw new Error('Authentication service not available - RPC not initialized');
    }

    try {
      console.log('Registering user:', username, email);
      await authorizator.RegisterStepOne(username, email, password);
    } catch (error) {
      if (error instanceof RegistrationFailed) {
        switch (error.reason) {
          case RegistrationError.UsernameAlreadyTaken:
            throw new Error('Username is already taken');
          case RegistrationError.EmailAlreadyTaken:
            throw new Error('Email is already registered');
          case RegistrationError.InvalidUsername:
            throw new Error('Invalid username format');
          default:
            throw new Error('Registration failed');
        }
      }
      throw error;
    }
  }

  async registerStepTwo(username: string, code: number): Promise<void> {
    if (!authorizator) {
      throw new Error('Authentication service not available - RPC not initialized');
    }

    try {
      await authorizator.RegisterStepTwo(username, code);
    } catch (error) {
      if (error instanceof RegistrationFailed) {
        switch (error.reason) {
          case RegistrationError.InvorrectCode:
            throw new Error('Invalid verification code');
          default:
            throw new Error('Registration verification failed');
        }
      }
      throw error;
    }
  }

  // Auto-login with stored session on app start
  async tryAutoLogin(): Promise<boolean> {
    if (this._authState.sessionId && !this._authState.isAuthenticated) {
      try {
        await this.loginWithSession(this._authState.sessionId);
        return true;
      } catch (error) {
        console.warn('Auto-login failed:', error);
        return false;
      }
    }
    return this._authState.isAuthenticated;
  }
}

// Global singleton instance
export const authService = new AuthService();
