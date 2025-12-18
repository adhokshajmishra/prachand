DO $$ DECLARE
r RECORD;
BEGIN
FOR r IN (SELECT tablename FROM pg_tables WHERE schemaname = current_schema()) LOOP
        EXECUTE 'DROP TABLE IF EXISTS ' || quote_ident(r.tablename) || ' CASCADE';
END LOOP;
END $$;

create table controllers (
    id bigserial,
    controller_identifier varchar(40) PRIMARY KEY,
    controller_key    varchar(40),
    controller_invalid bool
);

create table nodes (
    id bigserial,
    host_identifier varchar(40) PRIMARY KEY,
    os_arch TEXT,
    os_build    TEXT,
    os_major    TEXT,
    os_minor    TEXT,
    os_name TEXT,
    os_platform TEXT,
    agent_version TEXT,
    node_key    varchar(40) UNIQUE,
    node_invalid boolean,
    hardware_vendor TEXT,
    hardware_model TEXT,
    hardware_version TEXT,
    hostname TEXT,
    enrolled_on bigint,
    last_seen bigint,
    hardware_cpu_logical_core TEXT,
    hardware_cpu_type TEXT,
    hardware_physical_memory TEXT
);

create table command_queue (
    id bigserial PRIMARY KEY,
    host_identifier varchar(40),
    command TEXT,
    queue_time bigint,
    sent_time bigint,
    sent boolean
);

create table response_queue (
    id bigserial PRIMARY KEY,
    host_identifier varchar(40),
    command_id bigint,
    timestamp bigint,
    response TEXT
);