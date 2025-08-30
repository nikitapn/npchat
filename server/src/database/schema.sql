-- NPChat Database Schema
-- SQLite DDL for chat application with authentication, contacts, and messaging

-- Users table for authentication
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    email TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    created_at INTEGER NOT NULL,
    last_login INTEGER,
    is_active INTEGER DEFAULT 1,
    CONSTRAINT username_length CHECK (length(username) >= 3 AND length(username) <= 32),
    CONSTRAINT email_format CHECK (email LIKE '%@%.%')
);

-- Pending registrations for two-step verification
CREATE TABLE IF NOT EXISTS pending_registrations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL,
    email TEXT NOT NULL,
    password_hash TEXT NOT NULL,
    verification_code TEXT NOT NULL,
    created_at INTEGER NOT NULL,
    expires_at INTEGER NOT NULL,
    CONSTRAINT unique_pending_email UNIQUE (email),
    CONSTRAINT unique_pending_username UNIQUE (username)
);

-- User sessions for authentication management
CREATE TABLE IF NOT EXISTS user_sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    session_token TEXT UNIQUE NOT NULL,
    created_at INTEGER NOT NULL,
    expires_at INTEGER NOT NULL,
    last_activity INTEGER NOT NULL,
    user_agent TEXT,
    ip_address TEXT,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Contacts/friends relationship
CREATE TABLE IF NOT EXISTS contacts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    owner_id INTEGER NOT NULL,
    contact_id INTEGER NOT NULL,
    added_at INTEGER NOT NULL,
    blocked INTEGER DEFAULT 0,
    FOREIGN KEY (owner_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (contact_id) REFERENCES users(id) ON DELETE CASCADE,
    CONSTRAINT unique_contact UNIQUE (owner_id, contact_id),
    CONSTRAINT no_self_contact CHECK (owner_id != contact_id)
);

-- Chat rooms/conversations
CREATE TABLE IF NOT EXISTS chats (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    created_by INTEGER NOT NULL,
    created_at INTEGER NOT NULL,
    FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE SET NULL
);

-- Chat participants - many-to-many relationship
CREATE TABLE IF NOT EXISTS chat_participants (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    chat_id INTEGER NOT NULL,
    user_id INTEGER NOT NULL,
    joined_at INTEGER NOT NULL,
    left_at INTEGER NULL,
    FOREIGN KEY (chat_id) REFERENCES chats(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    CONSTRAINT unique_participant UNIQUE (chat_id, user_id)
);

-- File attachments for messages
CREATE TABLE IF NOT EXISTS attachments (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    type INTEGER NOT NULL, -- ChatAttachmentType enum
    name TEXT NOT NULL,
    data BLOB NOT NULL
);

-- Chat messages
CREATE TABLE IF NOT EXISTS messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    chat_id INTEGER NOT NULL,
    sender_id INTEGER NOT NULL,
    content TEXT NOT NULL,
    timestamp INTEGER NOT NULL,
    attachment_id INTEGER NULL,
    FOREIGN KEY (chat_id) REFERENCES chats(id) ON DELETE CASCADE,
    FOREIGN KEY (sender_id) REFERENCES users(id) ON DELETE SET NULL,
    FOREIGN KEY (attachment_id) REFERENCES attachments(id) ON DELETE SET NULL
);

-- Message delivery tracking
CREATE TABLE IF NOT EXISTS message_delivery (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    message_id INTEGER NOT NULL,
    user_id INTEGER NOT NULL,
    delivered_at INTEGER NOT NULL,
    FOREIGN KEY (message_id) REFERENCES messages(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    CONSTRAINT unique_delivery UNIQUE (message_id, user_id)
);

-- Message read status tracking
CREATE TABLE IF NOT EXISTS message_read (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    message_id INTEGER NOT NULL,
    user_id INTEGER NOT NULL,
    read_at INTEGER NOT NULL,
    FOREIGN KEY (message_id) REFERENCES messages(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    CONSTRAINT unique_read UNIQUE (message_id, user_id)
);

-- Indexes for performance optimization
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
CREATE INDEX IF NOT EXISTS idx_users_active ON users(is_active);

CREATE INDEX IF NOT EXISTS idx_pending_registrations_email ON pending_registrations(email);
CREATE INDEX IF NOT EXISTS idx_pending_registrations_expires ON pending_registrations(expires_at);

CREATE INDEX IF NOT EXISTS idx_sessions_token ON user_sessions(session_token);
CREATE INDEX IF NOT EXISTS idx_sessions_user_expires ON user_sessions(user_id, expires_at);

CREATE INDEX IF NOT EXISTS idx_contacts_owner ON contacts(owner_id);
CREATE INDEX IF NOT EXISTS idx_contacts_contact ON contacts(contact_id);
CREATE INDEX IF NOT EXISTS idx_contacts_blocked ON contacts(owner_id, blocked);

CREATE INDEX IF NOT EXISTS idx_chat_participants_chat ON chat_participants(chat_id);
CREATE INDEX IF NOT EXISTS idx_chat_participants_user ON chat_participants(user_id);

CREATE INDEX IF NOT EXISTS idx_messages_chat_timestamp ON messages(chat_id, timestamp);
CREATE INDEX IF NOT EXISTS idx_messages_sender ON messages(sender_id);
CREATE INDEX IF NOT EXISTS idx_messages_content ON messages(content); -- For search

CREATE INDEX IF NOT EXISTS idx_message_delivery_user ON message_delivery(user_id);
CREATE INDEX IF NOT EXISTS idx_message_delivery_message ON message_delivery(message_id);

CREATE INDEX IF NOT EXISTS idx_message_read_user ON message_read(user_id);
CREATE INDEX IF NOT EXISTS idx_message_read_message ON message_read(message_id);

-- Views for common queries
CREATE VIEW IF NOT EXISTS active_users AS
SELECT id, username, email, created_at, last_login
FROM users 
WHERE is_active = 1;

CREATE VIEW IF NOT EXISTS chat_summary AS
SELECT 
    c.id as chat_id,
    c.created_by,
    c.created_at,
    COUNT(cp.user_id) as participant_count,
    MAX(m.timestamp) as last_message_time
FROM chats c
LEFT JOIN chat_participants cp ON c.id = cp.chat_id AND cp.left_at IS NULL
LEFT JOIN messages m ON c.id = m.chat_id
GROUP BY c.id, c.created_by, c.created_at;

-- Triggers for data consistency
CREATE TRIGGER IF NOT EXISTS cleanup_expired_registrations
AFTER INSERT ON pending_registrations
BEGIN
    DELETE FROM pending_registrations 
    WHERE expires_at < strftime('%s', 'now');
END;

CREATE TRIGGER IF NOT EXISTS cleanup_expired_sessions
AFTER INSERT ON user_sessions
BEGIN
    DELETE FROM user_sessions 
    WHERE expires_at < strftime('%s', 'now');
END;

-- Initial data cleanup (remove expired records)
DELETE FROM pending_registrations WHERE expires_at < strftime('%s', 'now');
DELETE FROM user_sessions WHERE expires_at < strftime('%s', 'now');
