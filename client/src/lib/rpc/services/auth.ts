import * as NPRPC from 'nprpc';
import { Authorizator, AuthorizationFailed, RegistrationFailed, AuthorizationError, RegistrationError } from '../npchat';
import type { UserData } from '../npchat';
import { RegisteredUser } from '../npchat';
import { authorizator } from '../index';
import { sessionManager, SecurityUtils } from './session';

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
  private initializationPromise: Promise<void> | null = null;

  constructor() {
    // Don't auto-login in constructor - wait for explicit initialization
    this.loadSessionDataOnly();
  }

  // Load session data without attempting RPC calls
  private loadSessionDataOnly() {
    try {
      if (sessionManager.isSessionValid()) {
        const sessionId = sessionManager.getSessionFromSecureCookie();
        if (sessionId && SecurityUtils.isValidSessionId(sessionId)) {
          this.updateAuthState({
            sessionId: sessionId
          });
        }
      }
    } catch (error) {
      console.warn('Failed to load session data:', error);
      this.clearSessionFromCookies();
    }
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

  // Check if RPC is available
  private isRPCAvailable(): boolean {
    return authorizator !== null && authorizator !== undefined;
  }

  // Wait for RPC to be available with timeout
  private async waitForRPC(timeoutMs: number = 5000): Promise<boolean> {
    const startTime = Date.now();
    
    while (!this.isRPCAvailable() && (Date.now() - startTime) < timeoutMs) {
      await new Promise(resolve => setTimeout(resolve, 100)); // Wait 100ms
    }
    
    return this.isRPCAvailable();
  }

  // Enhanced cookie utility functions using secure session manager
  private saveSessionToCookies() {
    if (this._authState.sessionId && this._authState.userData) {
      sessionManager.setSecureSession(this._authState.sessionId, this._authState.userData);
    }
  }

  private loadSessionFromCookies() {
    try {
      if (sessionManager.isSessionValid()) {
        const sessionId = sessionManager.getSessionFromSecureCookie();
        if (sessionId && SecurityUtils.isValidSessionId(sessionId)) {
          this.updateAuthState({
            sessionId: sessionId
          });
        }
      }
    } catch (error) {
      console.warn('Failed to load secure session:', error);
      this.clearSessionFromCookies();
    }
  }

  private clearSessionFromCookies() {
    sessionManager.clearSecureSession();
  }

  async login(login: string, password: string): Promise<UserData> {
    // Wait for RPC to be available
    if (!this.isRPCAvailable()) {
      const rpcReady = await this.waitForRPC();
      if (!rpcReady) {
        throw new Error('Authentication service not available - RPC connection timeout');
      }
    }

    // Sanitize inputs for security
    const sanitizedLogin = SecurityUtils.sanitizeInput(login);
    
    if (!sanitizedLogin || !password) {
      throw new Error('Login and password are required');
    }

    try {
      const userData = await authorizator.LogIn(sanitizedLogin, password);
      console.log({userData});
      const userProxy = NPRPC.narrow(userData.registeredUser, RegisteredUser);

      this.updateAuthState({
        isAuthenticated: true,
        userData: userData,
        user: userProxy,
        sessionId: userData.sessionId
      });

      this.saveSessionToCookies();
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
    // Wait for RPC to be available
    if (!this.isRPCAvailable()) {
      const rpcReady = await this.waitForRPC();
      if (!rpcReady) {
        throw new Error('Authentication service not available - RPC connection timeout');
      }
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

      this.saveSessionToCookies();
      return userData;
    } catch (error) {
      // Clear invalid session
      this.clearSessionFromCookies();
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
    if (this.isRPCAvailable() && this._authState.sessionId) {
      try {
        const success = await authorizator.LogOut(this._authState.sessionId);
        
        this.updateAuthState({
          isAuthenticated: false,
          user: null,
          sessionId: null
        });
        
        this.clearSessionFromCookies();
        return success;
      } catch (error) {
        console.error('Logout error:', error);
        // Even if logout fails on server, clear local state
        this.updateAuthState({
          isAuthenticated: false,
          user: null,
          sessionId: null
        });
        this.clearSessionFromCookies();
        return false;
      }
    } else {
      // Clear local state even if RPC is not available
      this.updateAuthState({
        isAuthenticated: false,
        user: null,
        sessionId: null
      });
      
      this.clearSessionFromCookies();
      return true;
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

    // Sanitize and validate inputs
    const sanitizedUsername = SecurityUtils.sanitizeInput(username);
    const sanitizedEmail = SecurityUtils.sanitizeInput(email);
    
    if (!sanitizedUsername || !sanitizedEmail || !password) {
      throw new Error('Username, email, and password are required');
    }
    
    // Basic email validation
    const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
    if (!emailRegex.test(sanitizedEmail)) {
      throw new Error('Invalid email format');
    }

    try {
      console.log('Registering user:', sanitizedUsername, sanitizedEmail);
      await authorizator.RegisterStepOne(sanitizedUsername, sanitizedEmail, password);
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

    // Sanitize and validate inputs
    const sanitizedUsername = SecurityUtils.sanitizeInput(username);
    
    if (!sanitizedUsername || !code || code < 0 || code > 999999) {
      throw new Error('Valid username and verification code are required');
    }

    try {
      await authorizator.RegisterStepTwo(sanitizedUsername, code);
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

  // Initialize auth service and attempt auto-login
  async initialize(): Promise<boolean> {
    if (this.initializationPromise) {
      await this.initializationPromise;
      return this._authState.isAuthenticated;
    }

    this.initializationPromise = this._initialize();
    await this.initializationPromise;
    return this._authState.isAuthenticated;
  }

  private async _initialize(): Promise<void> {
    try {
      // Wait for RPC to be available
      const rpcReady = await this.waitForRPC(10000); // 10 second timeout
      if (!rpcReady) {
        console.warn('RPC connection not available for auto-login');
        return;
      }
      // Try to load session from cookies
      this.loadSessionFromCookies();
      // Attempt auto-login if we have a session
      if (this._authState.sessionId && !this._authState.isAuthenticated) {
        await this.tryAutoLogin();
      }
    } catch (error) {
      console.error('Auth service initialization failed:', error);
    }
  }

  // Method to get current user ID
  // TODO: Implement proper user ID retrieval from session or server
  // Currently returns a hardcoded value until we store user ID during authentication
  getCurrentUserId(): number {
    if (!this._authState.isAuthenticated) {
      throw new Error('User is not authenticated');
    }
    
    // For now, return a hardcoded user ID until we implement proper user ID storage
    // In a real app, this would be stored during login or retrieved from the server
    return 1;
  }
}

// Global singleton instance
export const authService = new AuthService();
