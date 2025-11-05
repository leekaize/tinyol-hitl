# Contributing

Research project. <30 days to delivery. Move fast.

## Issues

**Format:** `[TYPE] Component: What's broken or needed`

**Types:** BUG | FEATURE | TASK

Use templates in `.github/ISSUE_TEMPLATE/`.

## Pull Requests

**Before coding:**
1. Create issue first
2. Get confirmation (comment "I'll take this")
3. Fork, branch, code

**Commit format:**
```
type: what changed (50 chars max)

Why it matters.

Fixes #123
```

**Types:** fix | feat | perf | refactor | docs | test

**Merge criteria:**
- Arduino sketch compiles (ESP32, RP2350, Arduino)
- Tests pass (core/clustering/test_kmeans)
- Linked to issue

## Code Standards

**Three rules:**
1. Delete > add. Simplify ruthlessly.
2. Fast > clever. Measure, don't assume.
3. Readable > compact. Clear wins.

**Static allocation only.** No malloc. Ever.

**Fixed-point math.** Q16.16 format. Document range limits.

**Platform abstraction.** Core algorithm stays pure. Platform layer handles I/O.

## Testing

**Required:**
- Unit test for new algorithms
- Test on real hardware before PR
- Include Arduino IDE verification screenshot

**CWRU dataset:** Validate on public data. Post confusion matrix.

## Priority

**Critical path (ship first):**
1. Arduino sketch working (3 platforms)
2. CWRU dataset streaming
3. Hardware test rig validation

**Nice-to-have (defer if time short):**
- Additional platforms
- Extra features
- Documentation polish

## Questions

Open issue. Tag with `question` label. Response within 48 hours.

---

**Remember:** Research project. Perfect is enemy of done. Ship working code.