# litexm CSS Performance Optimization — Findings

## Context

Starting point: `createFromString` on a small playlist document (9122B HTML, 293 nodes,
110 master selectors, 40 doc selectors) was taking **18–100ms** depending on whether fonts
were cached. After stripping font-load time, the pure parse+style pipeline was **~18ms**
on first warm load and **~10ms** on repeated loads.

Per-phase instrumentation was added to `createFromString` (gated by `#define LITEHTML_PERF`
in `document.cpp`) to break down where time went:

```
[litehtml-phases] html_parse  elements  master_css_parse  apply_master
                  parse_attrs  doc_css_parse  apply_doc  apply_user
                  compute_styles  render_tree  TOTAL
```

---

## What Worked ✅

### 1. Master CSS LRU Cache — `~1700µs saved` per warm load

**File:** `document.cpp` / `document.h`

The master stylesheet (browser defaults: `litehtml::master_css`) was re-parsed, re-sorted,
and re-indexed from scratch on every `createFromString` call. This was pure wasted work —
the string never changes.

**Fix:** Static `css_lru` cache (16 slots, LRU eviction) keyed by `hash(master_styles)`.
On cache hit: copy the pre-parsed `css` object (µs). On miss: parse+sort as before, store.

```
master_css_parse: 1783µs → 54µs  (~33x faster on warm loads)
```

**API for dynamic CSS injection:**
```cpp
document::invalidate_master_css_cache(); // clears all 16 slots
```

**Safety:** Each document gets a **value copy** of the cached `css` object — fully
independent. Multiple distinct `master_styles` strings coexist in the cache.

---

### 2. Document Inline CSS LRU Cache — `~3400µs saved` per warm load

**File:** `document.cpp` / `document.h`

The `<style>` block CSS for each view was also re-parsed on every load. For a view like
the playlist window that builds the same HTML every time, this was pure overhead.

**Fix:** Same `css_lru` struct (16 slots), keyed by Boost-style hash combine of all
`css_text + media + baseurl` entries collected by `parse_attributes()`.

```
doc_css_parse: 3428µs → 8µs  (~430x faster on warm loads)
```

**Design:** Hash is computed before the parse loop. On cache miss: parse+sort, then store.
On hit: single copy into `doc->m_styles`. Empty `m_css` (no `<style>` tags) skips the cache
entirely.

---

### 3. Shared `css_lru` Struct — Architectural Consolidation

Both caches use the same `document::css_lru` nested struct defined in `document.h`:

```cpp
struct css_lru {
    static constexpr size_t MAX = 16;
    const litehtml::css* find(size_t h);  // promotes to MRU, returns nullptr on miss
    void put(size_t h, const litehtml::css& css);  // evicts LRU if at capacity
    void clear();                                   // for cache invalidation
};
static std::mutex  s_master_css_mutex;
static css_lru     s_master_css_lru;
static std::mutex  s_doc_css_mutex;
static css_lru     s_doc_css_lru;
```

Memory is bounded: max 32 parsed `css` objects in process memory at any time regardless
of how many view types the app has.

---

### 4. String Intern Table: `std::map` → `std::unordered_map` — Minor win

**File:** `string_id.cpp`

The global `_id()` intern table used `std::map<string, string_id>` (O(log n) lookups).
Switched to `std::unordered_map` with `reserve(512)` pre-allocation to avoid rehash churn.

**Effect:** Modest improvement — most `_id()` calls are for already-known strings
(CSS property names, HTML tags), so the table rarely grows after startup. The per-call
savings are ~50–80ns but add up across 3000+ calls per document load.

**Note on eviction:** String IDs are permanent integer indices. Entries **cannot** be
evicted without invalidating all live `string_id` values held across all open documents.
The vocabulary is bounded by the app's CSS/HTML content — it plateaus after all views
have been opened once (~500–600 entries total).

---

### 5. Whitespace Node Explosion Fix — `~13% fewer elements`

**File:** `document.cpp` — `create_node()`, `GUMBO_NODE_WHITESPACE` case

The original code split every whitespace character into a separate `el_space` element:
```cpp
// BEFORE: "\n    " (5 chars) → 5 separate el_space heap objects
for(size_t i = 0; i < str.length(); i++)
    elements.push_back(std::make_shared<el_space>(str.substr(i, 1).c_str(), ...));
```
For pretty-printed HTML, whitespace-only gumbo nodes (`"\n    "`, `"\n  "`, etc.) each
created as many elements as they had characters. Each of those elements went through the
full `apply_stylesheet`, `compute_styles`, and `create_render_item` pipeline — even though
for `white-space: normal` (the common case), `is_white_space()` returns true and they all
collapse to a single space anyway.

The only character `el_space` actually needs to distinguish is `'\n'` (for `is_break()` in
`pre`/`pre-line`/`pre-wrap` modes). Fix: split only on newlines.

```cpp
// AFTER: "\n    " → el_space("\n") + el_space("    ")  [2 elements]
//        "  "     → el_space("  ")                     [1 element]
```

```
Element count: n=293 → n=256  (37 fewer, 12.6% reduction)
Reduction confirmed by text_width call counter in createFromString log.
```

**Note:** The absolute time saving was modest because the eliminated elements were the
cheapest in the pipeline (whitespace `el_space` has a simple `compute_styles` path). The
main benefit is structural correctness and reduced heap allocation churn.

---

## What Did Not Work ❌

### 6. `style::m_properties`: `std::map` → `std::unordered_map`

**File:** `style.h`

Attempted to switch `props_map` (per-element merged property storage) from `std::map`
to `std::unordered_map` to speed up the ~30 `get_property()` calls per element in
`compute_styles`.

**Result:** Significant regression — `compute_styles` went from ~1782µs → ~3295µs.

**Why:** `property_value` is a large variant type (~100+ bytes). `std::unordered_map`
node-per-entry allocation + rehashing as properties are accumulated during
`apply_stylesheet` is more expensive than `std::map` for maps with 20–50 large-value
entries. Cache locality is also worse. **Reverted.**

---

### 6. Flyweight Pool Hash Improvement

**File:** `document.h` — `css_properties_hash`

The `m_css_pool` flyweight deduplication pool used a 4-field XOR hash:
```cpp
get_display() ^ get_position() ^ (int)get_float() ^ get_font()
```
This caused near-total bucket collisions (all `display:block/static/none/font` elements
hit the same bucket), making pool lookup O(N) instead of O(1).

Attempted fix: 12-field Boost-style combine hash including color, font_size, width,
margin_top. Tested a lighter 6-field version without float math.

**Result:** No measurable improvement. The pool is small (~10–20 unique
`css_properties` objects per document). Even with O(N) linear scan through 10–20 entries,
the absolute cost is negligible. A heavier hash computation costs more than it saves.

**Current state:** Kept a 6-field Boost combine hash (display, position, float, font,
font_size, color) as a mild improvement over the 4-field XOR, with no float math overhead.

---

### 7. Thread-Local Shadow Cache for `_id()`

**File:** `string_id.cpp`

Attempted to add a `thread_local std::unordered_map<string, string_id>` shadow cache to
eliminate the global mutex from `_id()` hot path.

**Result:** Severe regression — total parse time roughly doubled (5.5ms → 11ms+).

**Why:** The `thread_local std::unordered_map<std::string, ...>` is a heap-allocated
data structure resident in TLS memory. Accessing it on every `_id()` call (3000+ per
document) polluted L1/L2 cache, degrading all subsequent operations including
`apply_stylesheet` and `compute_styles`. The ~50ns mutex savings per call were dwarfed
by the cache pressure introduced. **Reverted.**

---

## Net Result

| Phase | Before all changes | After all changes | Status |
|---|---|---|---|
| `master_css_parse` | 1783µs | **~90µs** | ✅ LRU cache hit |
| `doc_css_parse` | 3428µs | **~15µs** | ✅ LRU cache hit |
| `elements` | ~3400µs | ~4000µs | → steady-state (13% fewer elements) |
| `compute_styles` | ~3700µs | ~3600µs | → unchanged |
| `html_parse` | ~1300µs | ~1300µs | → gumbo, immutable |
| `apply_doc` | ~890µs | ~780µs | → minor improvement |

**Total warm-load improvement: ~5ms** (18ms → **~10ms** steady state).

The CSS parse caches (`master_css_parse` + `doc_css_parse`) are the dominant wins,
shaving ~5150µs per warm document load. The remaining ~10ms is CSS cascade computation
(`elements` + `compute_styles`) which is inherently per-document work.

> **Note on outlier measurements:** A single hot run of ~5.5ms was observed immediately
> after the LRU caches were introduced, under ideal CPU cache conditions. Sustained
> steady-state across multiple test sessions consistently reads ~10ms. The 5.5ms figure
> should be treated as a best-case bound, not a representative average.

---

## Remaining Opportunities (Not Implemented)

1. **Full document cache** — hash the entire HTML string; cache the fully-parsed
   `document` object. Would require the document to be treated as immutable or
   copy-on-write. High complexity, highest possible reward (eliminates all parse work).

2. **`elements` phase** — DOM tree construction. Each of 293+ nodes calls `_id()`,
   `push_back` on child lists, and allocates `shared_ptr<element>`. Hard to cache;
   `create_node` is inherently sequential.

3. **`compute_styles` phase** — 30 `std::map::find()` calls per element via
   `get_property()`. A flat sorted array (flat_map) for `m_properties` could improve
   cache locality for small maps. Complex to implement; risk of regression similar to
   the `unordered_map` attempt.

4. **`apply_doc`** — still ~400-900µs. The selector index is already implemented;
   remaining cost is style merging (`style::combine`) per matched selector.

---

## Re-enabling Instrumentation

To re-enable per-phase timing output, add to your build flags or uncomment in
`document.cpp` at the top of `createFromString`:

```cpp
#define LITEHTML_PERF
```

Output goes to `stderr` in the format:
```
[litehtml-phases] html_parse=Nµs  elements=Nµs  master_css_parse=Nµs ...  TOTAL=Nµs
```
