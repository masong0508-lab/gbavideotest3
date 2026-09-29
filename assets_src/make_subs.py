#!/usr/bin/env python3
"""Turn an .srt subtitle file into subs.h for main.c.

    python3 tools/make_subs.py assets_src/subs.srt            # writes ./subs.h
    python3 tools/make_subs.py assets_src/subs.srt --report   # ...and prints every cue as it will appear
    python3 tools/make_subs.py assets_src/subs.srt --offset-ms -300   # show every cue 0.3 s EARLIER

Timing: the player counts GBA vblanks, 59.7275 per second (280896 cycles at 16.78 MHz) - NOT 60.
Converting at 60 makes every cue drift late by 0.45 % (about 3.4 s by the end of a 12-minute video).

Text: mapped onto the built-in font (A-Z, 0-9 and  . , ! ? ' -): upper-cased, curly quotes
straightened, : ; turned into commas, dashes into '-', everything else dropped.

Clean-up of "rolling" captions (YouTube/auto-caption .srt files), applied in this order:
  1. cues with no letters or digits (e.g. "...") are dropped
  2. fragments are joined: a cue that does not end a sentence is merged with the next one when the
     next starts within 0.6 s AND neither is a "held" cue AND the result fits on 2 lines
  3. a cue whose text is far shorter than its time span is HELD (the caption stayed up while nothing
     was said: "Damn." for 52 s).  Every cue is trimmed to  1.5 s + 0.09 s per character
  4. an identical line that overlaps the previous cue is collapsed (rolling duplicates); repeated
     lines that follow each other (a sung chorus) are kept
  5. tiny cues (< 0.5 s) that are followed within 0.4 s by another cue are joined onto it as a prefix
     ("Uh." + "Are you ready...?"); other tiny cues are shown for at least 0.9 s
  6. overlaps are removed by ending the earlier cue where the next one starts
  7. every cue is wrapped to 34 columns (balanced), at most 3 lines; a longer cue is split into
     consecutive cues that share its time span, so no text is ever cut off
"""
import argparse, re, sys

TICKS_PER_SEC = 59.7275
COLS, MAXLINES, MERGE_LINES = 34, 3, 2
GAP_MERGE = 0.6            # s: max silence between fragments that are joined
TINY = 0.5                 # s: cues shorter than this are "tiny"
TINY_GAP = 0.4             # s: a tiny cue this close to the next one is prefixed onto it
MIN_SHOW = 0.9             # s: minimum time a lone tiny cue stays up
CAP_BASE, CAP_PER_CHAR = 1.5, 0.09   # s: longest a cue may stay up = base + per_char * len(text)
FONT = set("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,!?'-")


def parse_time(t):
    h, m, rest = t.strip().split(':')
    s, ms = re.split('[,.]', rest)
    return ((int(h) * 60 + int(m)) * 60 + int(s)) + int(ms.ljust(3, '0')[:3]) / 1000.0


def parse_srt(text):
    text = text.replace('\r\n', '\n').replace('\r', '\n').lstrip('\ufeff')
    cues = []
    for block in re.split(r'\n\s*\n', text.strip()):
        lines = [l for l in block.split('\n') if l.strip()]
        for i, l in enumerate(lines):
            m = re.match(r'\s*(\d+:\d+:\d+[,.]\d+)\s*-->\s*(\d+:\d+:\d+[,.]\d+)', l)
            if m:
                cues.append([parse_time(m.group(1)), parse_time(m.group(2)), ' '.join(lines[i + 1:])])
                break
    return cues


def to_font(s):
    s = re.sub(r'<[^>]+>|\{[^}]*\}', '', s)
    s = s.replace('\u2019', "'").replace('\u2018', "'").replace('\u201c', '').replace('\u201d', '').replace('"', '')
    s = s.replace('\u2026', '...').replace('\u2014', ' - ').replace('\u2013', '-').replace(':', ',').replace(';', ',')
    s = ''.join(c if c in FONT else ' ' for c in s.upper())
    return re.sub(r'\s+', ' ', s).strip()


def greedy(words, width):
    lines, cur = [], ''
    for w in words:
        while len(w) > width:                                   # a word longer than a line is hard-split
            if cur:
                lines.append(cur); cur = ''
            lines.append(w[:width]); w = w[width:]
        if not cur:
            cur = w
        elif len(cur) + 1 + len(w) <= width:
            cur += ' ' + w
        else:
            lines.append(cur); cur = w
    if cur:
        lines.append(cur)
    return lines


def wrap(s):
    """Fewest lines that fit COLS, then the narrowest width that keeps that line count (balanced lines)."""
    words = s.split(' ')
    best = greedy(words, COLS)
    for w in range(COLS - 1, 0, -1):
        cand = greedy(words, w)
        if len(cand) != len(best):
            break
        best = cand
    return best


def ends_sentence(s):
    return bool(re.search(r"[.!?]$", s.rstrip("'- ")) or re.search(r"[.!?]['\"]?$", s))


def cap(text):
    return CAP_BASE + CAP_PER_CHAR * len(text)


def clean(cues):
    # 1. font-map, drop cues with nothing to show
    cs = []
    for a, b, t in cues:
        t = to_font(t)
        if re.search(r'[A-Z0-9]', t):
            cs.append([a, b, t])
    cs.sort(key=lambda c: c[0])

    # 2. join sentence fragments (never across a held cue, never into more than MERGE_LINES lines)
    def held(c):
        return (c[1] - c[0]) > cap(c[2])
    out = []
    for c in cs:
        if out:
            p = out[-1]
            if (not ends_sentence(p[2]) and 0 <= c[0] - p[1] <= GAP_MERGE and not held(p) and not held(c)
                    and len(wrap(p[2] + ' ' + c[2])) <= MERGE_LINES):
                p[1] = max(p[1], c[1]); p[2] += ' ' + c[2]
                continue
        out.append(c)
    cs = out

    # 3. trim held cues
    for c in cs:
        c[1] = min(c[1], c[0] + cap(c[2]))

    # 4. collapse an identical line that OVERLAPS the previous one (auto-captions emit rolling duplicates)
    out = []
    for c in cs:
        if out and out[-1][2] == c[2] and c[0] < out[-1][1]:      # true overlap only: sung repeats are separate cues
            out[-1][1] = max(out[-1][1], c[1]); continue
        out.append(c)
    cs = out

    # 5. tiny cues: prefix onto the next cue when it follows immediately, else guarantee a minimum time
    out, i = [], 0
    while i < len(cs):
        c = cs[i]
        if (c[1] - c[0]) < TINY and i + 1 < len(cs) and cs[i + 1][0] - c[1] < TINY_GAP:
            n = cs[i + 1]
            if n[2].startswith(c[2]):                       # the next cue already says it: just drop the stub
                cs[i + 1] = [c[0], n[1], n[2]]
            else:
                cs[i + 1] = [c[0], n[1], c[2] + ' ' + n[2]]
                if held(n):
                    cs[i + 1][1] = min(cs[i + 1][1], cs[i + 1][0] + cap(cs[i + 1][2]))
        else:
            if (c[1] - c[0]) < MIN_SHOW:
                nxt = cs[i + 1][0] if i + 1 < len(cs) else 1e9
                c[1] = min(c[0] + MIN_SHOW, max(c[1], nxt))
            out.append(c)
        i += 1
    out = [c for c in out]

    # 6. remove overlaps by ending the earlier cue where the next one starts
    for i in range(len(out) - 1):
        if out[i][1] > out[i + 1][0]:
            out[i][1] = out[i + 1][0]
    return [c for c in out if c[1] > c[0]]


def build(cues, offset_ms):
    res = []
    for a, b, text in clean(cues):
        lines = wrap(text)
        ng = -(-len(lines) // MAXLINES)                 # 6. fewest cues that fit, lines spread evenly
        base, extra = divmod(len(lines), ng)
        groups, i = [], 0
        for k in range(ng):
            n = base + (1 if k < extra else 0)
            groups.append(lines[i:i + n]); i += n
        weights = [sum(len(l) for l in g) for g in groups]
        total, acc, span = sum(weights), 0, b - a
        for g, w in zip(groups, weights):
            t0 = a + span * acc / total
            acc += w
            t1 = a + span * acc / total
            res.append([t0 + offset_ms / 1000.0, t1 + offset_ms / 1000.0, '\n'.join(g)])
    ticks, last_end = [], 0
    for t0, t1, txt in res:
        s = max(last_end, round(t0 * TICKS_PER_SEC))
        e = max(s + 1, round(t1 * TICKS_PER_SEC))
        ticks.append((s, e, txt)); last_end = e
    return ticks


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('srt')
    ap.add_argument('-o', '--out', default='subs.h')
    ap.add_argument('--offset-ms', type=int, default=0, help='shift all cues (negative = earlier)')
    ap.add_argument('--report', action='store_true', help='print every cue as it will be shown')
    a = ap.parse_args()

    cues = parse_srt(open(a.srt, encoding='utf-8-sig', errors='replace').read())
    if not cues:
        sys.exit('no cues found in ' + a.srt)
    res = build(cues, a.offset_ms)
    if max(e for _, e, _ in res) > 65535:
        sys.exit('a cue ends after tick 65535 (~18 min); sub_start/sub_end are u16 in main.c')

    L = ['/* subs.h - GENERATED by tools/make_subs.py from %s (offset %d ms). Do not edit by hand.' % (a.srt.replace('\\', '/').split('/')[-1], a.offset_ms),
         '   Times are vblank ticks (59.7275 per second); \'\\n\' separates the display lines of a cue. */',
         '#define SUB_N %d' % len(res), 'static const u16 sub_start[SUB_N] = {']
    L += ['  %d,' % s for s, _, _ in res]
    L += ['};', 'static const u16 sub_end[SUB_N] = {'] + ['  %d,' % e for _, e, _ in res]
    L += ['};', 'static const char *const sub_text[SUB_N] = {'] + ['  "%s",' % t.replace('\n', '\\n') for _, _, t in res] + ['};', '']
    open(a.out, 'w').write('\n'.join(L))

    print('%d cues in -> %d cues out -> %s' % (len(cues), len(res), a.out))
    if a.report:
        for s, e, t in res:
            print('%6d-%6d  %7.2fs +%.2fs  %s' % (s, e, s / TICKS_PER_SEC, (e - s) / TICKS_PER_SEC, t.replace('\n', ' / ')))


if __name__ == '__main__':
    main()
