-- MINI max: initial PostgreSQL schema (foundation).
BEGIN;

CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE users (
    id            BIGSERIAL PRIMARY KEY,
    phone         TEXT        NOT NULL UNIQUE,
    username      TEXT        UNIQUE,
    first_name    TEXT        NOT NULL,
    last_name     TEXT        NOT NULL DEFAULT '',
    bio           TEXT        NOT NULL DEFAULT '',
    password_hash TEXT,                       -- Argon2id (libsodium) encoded string
    created_at    TIMESTAMPTZ NOT NULL DEFAULT now(),
    last_seen_at  TIMESTAMPTZ
);

CREATE TABLE sessions (
    id          UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id     BIGINT      NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    device      TEXT        NOT NULL DEFAULT '',
    ip          INET,
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    last_active TIMESTAMPTZ NOT NULL DEFAULT now(),
    revoked_at  TIMESTAMPTZ
);
CREATE INDEX sessions_user_idx ON sessions(user_id) WHERE revoked_at IS NULL;

CREATE TYPE chat_kind AS ENUM ('private', 'group', 'channel', 'secret');

CREATE TABLE chats (
    id         BIGSERIAL PRIMARY KEY,
    kind       chat_kind   NOT NULL,
    title      TEXT,
    username   TEXT UNIQUE,
    owner_id   BIGINT REFERENCES users(id) ON DELETE SET NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TYPE member_role AS ENUM ('owner', 'admin', 'moderator', 'member');

CREATE TABLE chat_members (
    chat_id   BIGINT      NOT NULL REFERENCES chats(id) ON DELETE CASCADE,
    user_id   BIGINT      NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    role      member_role NOT NULL DEFAULT 'member',
    pinned    BOOLEAN     NOT NULL DEFAULT false,
    muted     BOOLEAN     NOT NULL DEFAULT false,
    archived  BOOLEAN     NOT NULL DEFAULT false,
    joined_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    PRIMARY KEY (chat_id, user_id)
);
CREATE INDEX chat_members_user_idx ON chat_members(user_id);

CREATE TABLE messages (
    id          BIGSERIAL PRIMARY KEY,
    chat_id     BIGINT      NOT NULL REFERENCES chats(id) ON DELETE CASCADE,
    sender_id   BIGINT      REFERENCES users(id) ON DELETE SET NULL,
    reply_to_id BIGINT      REFERENCES messages(id) ON DELETE SET NULL,
    body        TEXT        NOT NULL DEFAULT '',
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    edited_at   TIMESTAMPTZ,
    deleted_at  TIMESTAMPTZ
);
CREATE INDEX messages_chat_idx ON messages(chat_id, id DESC);

COMMIT;
