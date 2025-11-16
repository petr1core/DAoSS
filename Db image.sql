-- Включаем генератор UUID (PostgreSQL 13+: gen_random_uuid() в pgcrypto)
CREATE EXTENSION IF NOT EXISTS pgcrypto;

-- =========================
-- Таблица: users
-- =========================
CREATE TABLE public.users (
  id              uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  login           varchar(320) NOT NULL,
  password_hash   varchar NOT NULL,
  auth_provider   varchar(32) NOT NULL,
  is_active       boolean NOT NULL DEFAULT true,
  notify_enabled  boolean NOT NULL DEFAULT true,
  last_seen       timestamptz NOT NULL DEFAULT now(),
  created_at      timestamptz NOT NULL DEFAULT now(),
  UNIQUE (login)
);

-- =========================
-- Таблица: language
-- =========================
CREATE TABLE public.language (
  id                   uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  code                 varchar(8)  NOT NULL,
  name                 varchar(32) NOT NULL,
  versioning_hint      varchar(32),
  file_extensions      varchar(256),
  comment_marker_open  varchar(64),
  comment_marker_close varchar(64),
  UNIQUE (code)
);

-- =========================
-- Таблица: projects
-- =========================
CREATE TABLE public.projects (
  id          uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  name        varchar(320) NOT NULL,
  description varchar      NOT NULL,
  owner_id    uuid NOT NULL,
  created_at  timestamptz NOT NULL DEFAULT now(),
  CONSTRAINT fk_projects_owner
    FOREIGN KEY (owner_id) REFERENCES public.users(id) ON DELETE RESTRICT
);
CREATE INDEX idx_projects_owner ON public.projects(owner_id);

-- =========================
-- Таблица: project_members
-- =========================
CREATE TABLE public.project_members (
  id          uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  project_id  uuid NOT NULL,
  user_id     uuid NOT NULL,
  role        varchar(16) NOT NULL,      -- 'owner' | 'reviewer'
  assigned_by uuid NULL,                 -- FK -> users.id
  CONSTRAINT uq_project_members UNIQUE (project_id, user_id),
  CONSTRAINT fk_pm_project  FOREIGN KEY (project_id) REFERENCES public.projects(id) ON DELETE CASCADE,
  CONSTRAINT fk_pm_user     FOREIGN KEY (user_id)    REFERENCES public.users(id)    ON DELETE CASCADE,
  CONSTRAINT fk_pm_assigned FOREIGN KEY (assigned_by) REFERENCES public.users(id)   ON DELETE SET NULL
);
CREATE INDEX idx_pm_project ON public.project_members(project_id);
CREATE INDEX idx_pm_user    ON public.project_members(user_id);

-- =========================
-- Таблица: source_files
-- =========================
CREATE TABLE public.source_files (
  id                uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  project_id        uuid NOT NULL,
  path              varchar(1024) NOT NULL,
  language_id       uuid NOT NULL,
  latest_version_id uuid, -- FK добавим после создания source_file_versions
  created_at        timestamptz NOT NULL DEFAULT now(),
  CONSTRAINT uq_source_files_project_path UNIQUE (project_id, path),
  CONSTRAINT fk_sf_project  FOREIGN KEY (project_id)  REFERENCES public.projects(id) ON DELETE CASCADE,
  CONSTRAINT fk_sf_language FOREIGN KEY (language_id) REFERENCES public.language(id) ON DELETE RESTRICT
);
CREATE INDEX idx_sf_project  ON public.source_files(project_id);
CREATE INDEX idx_sf_language ON public.source_files(language_id);

-- =========================
-- Таблица: source_file_versions
-- =========================
CREATE TABLE public.source_file_versions (
  id            uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  source_file_id uuid NOT NULL,
  version_index  integer NOT NULL,
  content        text NOT NULL,
  author_id      uuid NOT NULL,
  created_at     timestamptz DEFAULT now(),
  message        varchar(300),
  CONSTRAINT uq_sfv UNIQUE (source_file_id, version_index),
  CONSTRAINT fk_sfv_source_file FOREIGN KEY (source_file_id) REFERENCES public.source_files(id) ON DELETE CASCADE,
  CONSTRAINT fk_sfv_author      FOREIGN KEY (author_id)      REFERENCES public.users(id)        ON DELETE RESTRICT
);
CREATE INDEX idx_sfv_source_file ON public.source_file_versions(source_file_id);
CREATE INDEX idx_sfv_author      ON public.source_file_versions(author_id);

-- Связь на latest_version_id (чтобы избежать цикла — отдельно, деферрируемая)
ALTER TABLE public.source_files
  ADD CONSTRAINT fk_sf_latest_version
  FOREIGN KEY (latest_version_id)
  REFERENCES public.source_file_versions(id)
  DEFERRABLE INITIALLY DEFERRED;

-- =========================
-- Таблица: code_regions
-- =========================
CREATE TABLE public.code_regions (
  id             uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  source_file_id uuid NOT NULL,
  start_line     integer NOT NULL,
  end_line       integer NOT NULL,
  tag_name       varchar(100),
  region_type    varchar(16), -- 'flowchart' и т.п.
  CONSTRAINT fk_cr_source_file FOREIGN KEY (source_file_id) REFERENCES public.source_files(id) ON DELETE CASCADE
);
CREATE INDEX idx_cr_source_file ON public.code_regions(source_file_id);

-- =========================
-- Таблица: diagrams
-- =========================
CREATE TABLE public.diagrams (
  id                uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  project_id        uuid NOT NULL,
  region_id         uuid NOT NULL,
  name              varchar(200) NOT NULL,
  latest_version_id uuid, -- FK добавим после создания diagram_versions
  created_at        timestamptz NOT NULL DEFAULT now(),
  updated_at        timestamptz NOT NULL DEFAULT now(),
  CONSTRAINT uq_diagrams_region UNIQUE (region_id),
  CONSTRAINT fk_di_project  FOREIGN KEY (project_id) REFERENCES public.projects(id)     ON DELETE CASCADE,
  CONSTRAINT fk_di_region   FOREIGN KEY (region_id)  REFERENCES public.code_regions(id) ON DELETE RESTRICT
);
CREATE INDEX idx_di_project ON public.diagrams(project_id);
CREATE INDEX idx_di_region  ON public.diagrams(region_id);

-- =========================
-- Таблица: diagram_versions
-- =========================
CREATE TABLE public.diagram_versions (
  id            uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  diagram_id    uuid NOT NULL,
  version_index integer NOT NULL,
  json_snapshot jsonb NOT NULL,
  author_id     uuid NOT NULL,
  created_at    timestamptz NOT NULL DEFAULT now(),
  message       varchar(300),
  CONSTRAINT uq_dv UNIQUE (diagram_id, version_index),
  CONSTRAINT fk_dv_diagram FOREIGN KEY (diagram_id) REFERENCES public.diagrams(id) ON DELETE CASCADE,
  CONSTRAINT fk_dv_author  FOREIGN KEY (author_id)  REFERENCES public.users(id)    ON DELETE RESTRICT
);
CREATE INDEX idx_dv_diagram ON public.diagram_versions(diagram_id);
CREATE INDEX idx_dv_author  ON public.diagram_versions(author_id);

-- Связь на latest_version_id (деферрируемая, чтобы не ловить цикл)
ALTER TABLE public.diagrams
  ADD CONSTRAINT fk_di_latest_version
  FOREIGN KEY (latest_version_id)
  REFERENCES public.diagram_versions(id)
  DEFERRABLE INITIALLY DEFERRED;

-- =========================
-- Таблица: reviews
-- =========================
CREATE TABLE public.reviews (
  id          uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  project_id  uuid NOT NULL,
  target_type varchar(24) NOT NULL, -- 'diagram_version' | 'source_file_version'
  target_id   uuid NOT NULL,        -- полиморфный FK (ограничиваем CHECK)
  status      varchar(16) NOT NULL DEFAULT 'open', -- 'open' | 'closed' | 'rejected'
  created_by  uuid NOT NULL,
  created_at  timestamptz NOT NULL DEFAULT now(),
  CONSTRAINT fk_reviews_project  FOREIGN KEY (project_id) REFERENCES public.projects(id) ON DELETE CASCADE,
  CONSTRAINT fk_reviews_creator  FOREIGN KEY (created_by) REFERENCES public.users(id)    ON DELETE RESTRICT,
  CONSTRAINT ck_reviews_target_type CHECK (target_type IN ('diagram_version','source_file_version'))
);
CREATE INDEX idx_reviews_project ON public.reviews(project_id);
CREATE INDEX idx_reviews_creator ON public.reviews(created_by);
CREATE INDEX idx_reviews_target  ON public.reviews(target_type, target_id);

-- (опционально) можно добавить триггеры для строгой валидации полиморфного target_id

-- =========================
-- Таблица: review_items
-- =========================
CREATE TABLE public.review_items (
  id          uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  review_id   uuid NOT NULL,
  kind        varchar(16) NOT NULL,    -- 'comment' | 'issue'
  anchor_type varchar(8)  NOT NULL,    -- 'code' | 'diagram'
  anchor_ref  varchar(512) NOT NULL,   -- file:{id}@v{version}:{...} | diagram:{id}@v{version}:{element}
  body        text NOT NULL,
  status      varchar(16) NOT NULL DEFAULT 'open', -- 'open' | 'resolved'
  created_by  uuid NOT NULL,
  created_at  timestamptz NOT NULL DEFAULT now(),
  CONSTRAINT fk_ri_review  FOREIGN KEY (review_id)  REFERENCES public.reviews(id) ON DELETE CASCADE,
  CONSTRAINT fk_ri_creator FOREIGN KEY (created_by) REFERENCES public.users(id)   ON DELETE RESTRICT
);
CREATE INDEX idx_ri_review  ON public.review_items(review_id);
CREATE INDEX idx_ri_creator ON public.review_items(created_by);

-- =========================
-- Таблица: comments
-- =========================
CREATE TABLE public.comments (
  id            uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  review_item_id uuid NOT NULL,
  body          text NOT NULL,
  author_id     uuid NOT NULL,
  created_at    timestamptz NOT NULL DEFAULT now(),
  CONSTRAINT fk_c_item   FOREIGN KEY (review_item_id) REFERENCES public.review_items(id) ON DELETE CASCADE,
  CONSTRAINT fk_c_author FOREIGN KEY (author_id)     REFERENCES public.users(id)        ON DELETE RESTRICT
);
CREATE INDEX idx_c_item   ON public.comments(review_item_id);
CREATE INDEX idx_c_author ON public.comments(author_id);

-- =========================
-- Таблица: audit_log
-- =========================
CREATE TABLE public.audit_log (
  id          uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  project_id  uuid,
  actor_id    uuid,
  action      varchar(64) NOT NULL,
  target_type varchar(24) NOT NULL,  -- полиморфный
  target_id   uuid,
  at          timestamptz NOT NULL DEFAULT now(),
  meta_json   jsonb,
  CONSTRAINT fk_al_project FOREIGN KEY (project_id) REFERENCES public.projects(id) ON DELETE SET NULL,
  CONSTRAINT fk_al_actor   FOREIGN KEY (actor_id)   REFERENCES public.users(id)    ON DELETE SET NULL
);
CREATE INDEX idx_al_project ON public.audit_log(project_id);
CREATE INDEX idx_al_actor   ON public.audit_log(actor_id);
CREATE INDEX idx_al_target  ON public.audit_log(target_type, target_id);