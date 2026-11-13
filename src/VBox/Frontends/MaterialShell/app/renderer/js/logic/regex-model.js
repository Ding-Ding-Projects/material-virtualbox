/*
 * Regular expression builder model.
 *
 * A pattern is an ordered list of constructs picked from CONSTRUCTS plus a
 * set of flags.  The model compiles it, times it against a sample under a
 * hard budget, and describes it in plain words.  Every search field in the
 * shell owns one of these through its ".*" button.
 */

export const CONSTRUCTS = [
    { id: 'start',      src: '^',               label: 'Start of text' },
    { id: 'end',        src: '$',               label: 'End of text' },
    { id: 'digits',     src: '\\d+',            label: 'One or more digits' },
    { id: 'letters',    src: '[A-Za-z]+',       label: 'Letters' },
    { id: 'space',      src: '\\s',             label: 'A space' },
    { id: 'optSpace',   src: '\\s*',            label: 'Optional spaces' },
    { id: 'any',        src: '.*',              label: 'Anything' },
    { id: 'boundary',   src: '\\b',             label: 'Word boundary' },
    { id: 'noSlash',    src: '[^\\\\/]+',       label: 'No slashes' },
    { id: 'uuid',       src: '[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}', label: 'A machine UUID' },
    { id: 'ipv4',       src: '(?:\\d{1,3}\\.){3}\\d{1,3}', label: 'An IPv4 address' }
];

export const FLAGS = [
    { id: 'i', label: 'Ignore case' },
    { id: 'm', label: 'Multiline' },
    { id: 's', label: 'Dot matches newline' },
    { id: 'u', label: 'Unicode' }
];

/** Budget in milliseconds for evaluating a pattern against the sample. */
export const PREVIEW_BUDGET_MS = 300;

/** Escapes literal text so it can be inserted as a construct. */
export function escapeLiteral(text) {
    return String(text).replace(/[.*+?^${}()|[\]\\/]/g, '\\$&');
}

/**
 * Builds the source of a pattern.  Parts are either construct ids, literal
 * objects ({ literal: 'text' }) or alternations ({ either: ['a', 'b'] }).
 */
export function buildSource(parts) {
    return parts.map((part) => {
        if (typeof part === 'string') {
            const c = CONSTRUCTS.find((x) => x.id === part);
            if (!c) throw new Error('Unknown construct: ' + part);
            return c.src;
        }
        if (part && typeof part.literal === 'string') return escapeLiteral(part.literal);
        if (part && Array.isArray(part.either)) return '(?:' + part.either.map(escapeLiteral).join('|') + ')';
        if (part && typeof part.raw === 'string') return part.raw;
        throw new Error('Unsupported part');
    }).join('');
}

export function buildFlags(flags) {
    return FLAGS.map((f) => f.id).filter((id) => flags.includes(id)).join('');
}

/** Compiles a pattern; returns { ok, regex, error }. */
export function compile(source, flags = '') {
    try {
        return { ok: true, regex: new RegExp(source, flags), error: null };
    } catch (e) {
        return { ok: false, regex: null, error: e.message };
    }
}

/**
 * Tests every sample line, stopping when the budget runs out so a runaway
 * pattern can never freeze the window.  Returns per-line results and timing.
 */
export function preview(source, flags, lines, now = () => performance.now()) {
    const c = compile(source, flags);
    if (!c.ok) return { ok: false, error: c.error, results: [], elapsedMs: 0, truncated: false };
    const t0 = now();
    const results = [];
    let truncated = false;
    for (const line of lines) {
        if (now() - t0 > PREVIEW_BUDGET_MS) { truncated = true; break; }
        c.regex.lastIndex = 0;
        const m = c.regex.exec(line);
        results.push(m ? { line, hit: true, index: m.index, match: m[0] } : { line, hit: false });
    }
    return { ok: true, error: null, results, elapsedMs: now() - t0, truncated };
}

/** A plain-language description of the pattern, for the "In plain words" card. */
export function describe(parts) {
    if (!parts.length) return 'Add a construct to start.';
    const words = parts.map((part) => {
        if (typeof part === 'string') return (CONSTRUCTS.find((x) => x.id === part) || { label: part }).label.toLowerCase();
        if (part.literal !== undefined) return '"' + part.literal + '"';
        if (part.either) return 'either ' + part.either.map((w) => '"' + w + '"').join(' or ');
        return 'a custom piece';
    });
    return 'Matches ' + words.join(', then ') + '.';
}

/**
 * Turns a free-text query into a matcher.  "/pat/flags" is a regex, anything
 * else is a case-insensitive substring.  Invalid regexes never throw.
 */
export function makeMatcher(query) {
    const q = String(query || '').trim();
    if (!q) return () => true;
    const m = /^\/(.*)\/([imsu]*)$/.exec(q);
    if (m) {
        const c = compile(m[1], m[2]);
        if (c.ok) return (text) => { c.regex.lastIndex = 0; return c.regex.test(String(text)); };
        return () => false;
    }
    const needle = q.toLowerCase();
    return (text) => String(text).toLowerCase().includes(needle);
}
