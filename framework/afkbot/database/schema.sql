CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    local_user_id TEXT UNIQUE NOT NULL,
    display_name TEXT NOT NULL,
    avatar_path TEXT,
    telegram_chat_id TEXT,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS bot_accounts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    owner_user_id INTEGER NOT NULL,
    slot INTEGER NOT NULL,
    sandbox_name TEXT NOT NULL,
    dota_id INTEGER,
    steamid64 INTEGER,
    steam3 TEXT,
    has_dota_plus INTEGER DEFAULT 0,
    state TEXT DEFAULT 'OFFLINE',
    hwnd TEXT,
    is_active INTEGER DEFAULT 1,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS windows (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    bot_account_id INTEGER,
    hwnd TEXT,
    process_id INTEGER,
    title TEXT,
    x INTEGER,
    y INTEGER,
    width INTEGER,
    height INTEGER,
    state TEXT,
    responsive INTEGER DEFAULT 0,
    last_seen_at TEXT
);

CREATE TABLE IF NOT EXISTS parties (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    leader_bot_account_id INTEGER,
    party_id TEXT,
    group_name TEXT,
    party_size INTEGER,
    status TEXT,
    created_at TEXT NOT NULL,
    ended_at TEXT
);

CREATE TABLE IF NOT EXISTS party_members (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    party_db_id INTEGER,
    bot_account_id INTEGER,
    dota_id INTEGER,
    steamid64 INTEGER,
    is_leader INTEGER DEFAULT 0,
    ready_state TEXT DEFAULT 'UNKNOWN',
    joined_at TEXT
);

CREATE TABLE IF NOT EXISTS events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    event_type TEXT NOT NULL,
    bot_account_id INTEGER,
    message TEXT,
    data_json TEXT,
    created_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS settings (
    key TEXT PRIMARY KEY,
    value TEXT,
    updated_at TEXT NOT NULL
);
