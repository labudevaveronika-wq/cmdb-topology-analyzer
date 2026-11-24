CREATE TABLE IF NOT EXISTS configuration_items (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL UNIQUE,
    ci_type VARCHAR(50) NOT NULL,
    ip_address VARCHAR(45) NOT NULL UNIQUE,
    port INTEGER DEFAULT 0,
    status VARCHAR(20) NOT NULL DEFAULT 'ACTIVE'
);

CREATE TABLE IF NOT EXISTS dependencies (
    id SERIAL PRIMARY KEY,
    source_id INTEGER NOT NULL REFERENCES configuration_items(id) ON DELETE CASCADE,
    target_id INTEGER NOT NULL REFERENCES configuration_items(id) ON DELETE CASCADE,
    protocol VARCHAR(30) NOT NULL DEFAULT 'TCP',
    is_critical BOOLEAN NOT NULL DEFAULT TRUE,
    CONSTRAINT unique_link UNIQUE (source_id, target_id),
    CONSTRAINT no_self_link CHECK (source_id <> target_id)
);

CREATE TABLE IF NOT EXISTS api_tokens (
    id SERIAL PRIMARY KEY,
    token VARCHAR(128) NOT NULL UNIQUE,
    description VARCHAR(100),
    is_active BOOLEAN NOT NULL DEFAULT TRUE
);

INSERT INTO api_tokens (token, description) 
VALUES ('secret-cmdb-token-2026', 'Default external integration token')
ON CONFLICT (token) DO NOTHING;
