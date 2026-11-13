/*
 * Export of section/key/value rows into every text format that can carry
 * them.  Each format states its loss before anything is written.
 */

export const FORMATS = [
    { ext: 'json',       label: 'JSON',              loss: null },
    { ext: 'csv',        label: 'Comma separated',   loss: null },
    { ext: 'tsv',        label: 'Tab separated',     loss: null },
    { ext: 'yaml',       label: 'YAML',              loss: null },
    { ext: 'toml',       label: 'TOML',              loss: null },
    { ext: 'xml',        label: 'XML',               loss: null },
    { ext: 'md',         label: 'Markdown table',    loss: null },
    { ext: 'html',       label: 'HTML table',        loss: 'Re-import reads the table only.' },
    { ext: 'ini',        label: 'INI',               loss: 'Line breaks inside values are escaped as \\n.' },
    { ext: 'properties', label: 'Java properties',   loss: 'Sections flatten into dotted keys.' },
    { ext: 'ndjson',     label: 'JSON lines',        loss: null },
    { ext: 'sql',        label: 'SQL inserts',       loss: null },
    { ext: 'txt',        label: 'Plain text',        loss: 'Not re-importable.' }
];

export const SCHEMA_VERSION = 3;

function csvCell(v, sep) {
    const s = String(v ?? '');
    return /["\r\n]/.test(s) || s.includes(sep) ? '"' + s.replace(/"/g, '""') + '"' : s;
}

function xmlEscape(s) {
    return String(s ?? '').replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}

function yamlScalar(v) {
    const s = String(v ?? '');
    return /^[\w .\-/()]+$/.test(s) && !/^(true|false|null|yes|no|~|-?\d[\d.]*)$/i.test(s) && s.trim() === s ? s : JSON.stringify(s);
}

function tomlKey(k) {
    return /^[A-Za-z0-9_-]+$/.test(k) ? k : JSON.stringify(k);
}

function grouped(rows) {
    const out = new Map();
    for (const r of rows) {
        if (!out.has(r.section)) out.set(r.section, []);
        out.get(r.section).push(r);
    }
    return out;
}

/** rows: [{ section, key, value }]; meta: { source }.  Returns text. */
export function exportRows(ext, rows, meta = {}) {
    const groups = grouped(rows);
    switch (ext) {
        case 'json': {
            const obj = { schema: SCHEMA_VERSION, source: meta.source || '' };
            for (const [sec, rs] of groups) obj[sec] = Object.fromEntries(rs.map((r) => [r.key, r.value]));
            return JSON.stringify(obj, null, 2) + '\n';
        }
        case 'ndjson':
            return rows.map((r) => JSON.stringify(r)).join('\n') + '\n';
        case 'csv':
        case 'tsv': {
            const sep = ext === 'csv' ? ',' : '\t';
            return ['section', 'key', 'value'].join(sep) + '\n' + rows.map((r) => [r.section, r.key, r.value].map((v) => csvCell(v, sep)).join(sep)).join('\n') + '\n';
        }
        case 'yaml': {
            let out = 'schema: ' + SCHEMA_VERSION + '\nsource: ' + yamlScalar(meta.source || '') + '\n';
            for (const [sec, rs] of groups) out += yamlScalar(sec) + ':\n' + rs.map((r) => '  ' + yamlScalar(r.key) + ': ' + yamlScalar(r.value)).join('\n') + '\n';
            return out;
        }
        case 'toml': {
            let out = 'schema = ' + SCHEMA_VERSION + '\nsource = ' + JSON.stringify(meta.source || '') + '\n';
            for (const [sec, rs] of groups) out += '\n[' + tomlKey(sec) + ']\n' + rs.map((r) => tomlKey(r.key) + ' = ' + JSON.stringify(String(r.value ?? ''))).join('\n') + '\n';
            return out;
        }
        case 'xml':
            return '<?xml version="1.0" encoding="UTF-8"?>\n<export schema="' + SCHEMA_VERSION + '" source="' + xmlEscape(meta.source || '') + '">\n' +
                [...groups].map(([sec, rs]) => '  <section name="' + xmlEscape(sec) + '">\n' + rs.map((r) => '    <item key="' + xmlEscape(r.key) + '">' + xmlEscape(r.value) + '</item>').join('\n') + '\n  </section>').join('\n') + '\n</export>\n';
        case 'md':
            return '| Section | Key | Value |\n|---|---|---|\n' + rows.map((r) => '| ' + [r.section, r.key, r.value].map((v) => String(v ?? '').replace(/\|/g, '\\|').replace(/\n/g, '<br>')).join(' | ') + ' |').join('\n') + '\n';
        case 'html':
            return '<table>\n<thead><tr><th>Section</th><th>Key</th><th>Value</th></tr></thead>\n<tbody>\n' + rows.map((r) => '<tr><td>' + xmlEscape(r.section) + '</td><td>' + xmlEscape(r.key) + '</td><td>' + xmlEscape(r.value) + '</td></tr>').join('\n') + '\n</tbody>\n</table>\n';
        case 'ini':
            return [...groups].map(([sec, rs]) => '[' + sec + ']\n' + rs.map((r) => r.key + '=' + String(r.value ?? '').replace(/\\/g, '\\\\').replace(/\n/g, '\\n')).join('\n')).join('\n\n') + '\n';
        case 'properties':
            return rows.map((r) => (r.section + '.' + r.key).replace(/[ :=]/g, '\\$&') + '=' + String(r.value ?? '').replace(/\\/g, '\\\\').replace(/\n/g, '\\n')).join('\n') + '\n';
        case 'sql':
            return 'CREATE TABLE IF NOT EXISTS export (section TEXT, key TEXT, value TEXT);\n' + rows.map((r) => 'INSERT INTO export VALUES (' + [r.section, r.key, r.value].map((v) => "'" + String(v ?? '').replace(/'/g, "''") + "'").join(', ') + ');').join('\n') + '\n';
        case 'txt':
            return [...groups].map(([sec, rs]) => sec + '\n' + rs.map((r) => '  ' + r.key + ': ' + r.value).join('\n')).join('\n\n') + '\n';
        default:
            throw new Error('Unknown export format: ' + ext);
    }
}

/** Parses the formats that promise a round trip back into rows. */
export function importRows(ext, text) {
    switch (ext) {
        case 'json': {
            const obj = JSON.parse(text);
            const rows = [];
            for (const [sec, v] of Object.entries(obj)) {
                if (sec === 'schema' || sec === 'source' || typeof v !== 'object' || v === null) continue;
                for (const [key, value] of Object.entries(v)) rows.push({ section: sec, key, value: String(value) });
            }
            return rows;
        }
        case 'ndjson':
            return text.split('\n').filter(Boolean).map((l) => { const r = JSON.parse(l); return { section: r.section, key: r.key, value: String(r.value) }; });
        case 'csv':
        case 'tsv': {
            const sep = ext === 'csv' ? ',' : '\t';
            const out = [];
            let row = [], cell = '', q = false;
            for (let i = 0; i < text.length; i++) {
                const ch = text[i];
                if (q) {
                    if (ch === '"' && text[i + 1] === '"') { cell += '"'; i++; } else if (ch === '"') q = false; else cell += ch;
                } else if (ch === '"') q = true;
                else if (ch === sep) { row.push(cell); cell = ''; } else if (ch === '\n') { row.push(cell); out.push(row); row = []; cell = ''; } else if (ch !== '\r') cell += ch;
            }
            if (cell || row.length) { row.push(cell); out.push(row); }
            return out.slice(1).map((r) => ({ section: r[0], key: r[1], value: r[2] }));
        }
        default:
            throw new Error('No importer for .' + ext);
    }
}
