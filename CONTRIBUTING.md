# Contributing Guide

Answer three questions: What changed? What's the impact? What's next?

---

## Issues

### Title Format

```
[TYPE] Component: Brief symptom
```

**Types:** `BUG | FEATURE | TASK`

**Examples:**
```
[BUG] Auth: Login fails with OAuth after 5min session
[FEATURE] Bulk operations: Multi-select with batch actions
[TASK] CI/CD: Migrate Jenkins to GitHub Actions
```

### Bug Report

```markdown
## Observed vs Expected
What happens vs what should happen.

## Reproduction
1. Step one with verification
2. Step two with verification
3. Success criteria

## Environment
Version: v2.3.1 (commit SHA if known)
OS/Browser: macOS 14 / Chrome 120
Config: Non-default settings

## Impact
Severity: P0-Critical | P1-High | P2-Medium | P3-Low
Affected: 30% of users / specific segment
Business: Login blocked, 400 support tickets

## Evidence
[Error logs in code blocks]
[Screenshots with annotations]
[Metrics dashboard links]
```

### Feature Request

```markdown
## Problem
Current limitation. State the pain. (2-3 sentences)

## Solution
Specific implementation. What you build.

## Success Metrics
- Reduce time by X% (from Ymin to Zmin)
- Support Nk records (up from M)
- Zero manual intervention for X% of cases

## User Stories
As a [role]
I want to [action]
So that [outcome]

[2-5 stories total]

## Alternatives
1. Option A: Pros / Cons / Why rejected
2. Option B: Pros / Cons / Why rejected

## Complexity
Small (<3d) | Medium (1-2w) | Large (>2w)
```

### Task

```markdown
## Objective
Binary pass/fail. Zero ambiguity.

## Context
Why now? What changed?

## Subtasks
- [ ] Deliverable 1
- [ ] Deliverable 2
- [ ] Verification step

## Dependencies
Blocks: #234
Blocked by: None

## Done
- [ ] Tests pass
- [ ] Docs updated
- [ ] Deployed to staging
```

### Labels (Required)

**Priority:**
- `P0-Critical`: System down, data loss, security breach
- `P1-High`: Core feature broken, major UX degradation
- `P2-Medium`: Partial breakage, workaround exists
- `P3-Low`: Minor, cosmetic, nice-to-have

**Type:**
- `type:bug | type:feature | type:task | type:docs | type:tech-debt`

**Component:**
- `component:auth | component:api | component:frontend | component:database`

### Response Times

| Priority | First Response | Resolution |
|----------|---------------|------------|
| P0 | 1hr | 24hr |
| P1 | 24hr | 1wk |
| P2 | 3d | 1mo |
| P3 | 1wk | Backlog |

### Auto-Close

- 90d no activity
- 48hr after info request ignored
- Duplicate (link original)
- Won't fix (explain why)

### Comments

```
**[Update/Question/Blocker]**

Statement or question.

Next action needed.
```

**Comment when:** Status changes, scope changes, blockers, test results.

**Never comment:** "+1" (use reactions), duplicates, side conversations.

---

## Commits

### Format

```
type: imperative verb + what changed

Why it matters. Problem solved. Measurable impact.

Fixes #123
```

**Types:** `fix | feat | perf | refactor | docs | test | chore`

**Rules:**
- Subject: 50 chars max, lowercase, no period
- Mood: imperative (add, fix, remove—not added, fixed)
- Body: why it matters, measurable impact
- Blank line between subject and body

**Examples:**

```
fix: resolve memory leak in cache invalidation

Previous implementation retained refs after expiry.
Now explicitly nulls refs on timeout.

Reduces baseline memory by 40MB under load.

Fixes #456
```

```
feat: add bulk export for 10k records

Users manually exported 10 batches (40min wait).
Streaming export handles 10k records in 2min.

Reduces support tickets by 30%.

Fixes #789
```

---

## Pull Requests

### Title

```
Type: Brief description (impact) (#issue)
```

**Example:** `Fix: Memory leak in cache (40MB reduction) (#456)`

### Description Template

```markdown
**Problem:** Cache held stale refs after timeout
**Root cause:** Event listener not dereg'd
**Solution:** Explicit null on expiry + WeakMap
**Impact:** 40MB reduction, 15% faster GC cycles
**Testing:** Load test with 10k concurrent users

Fixes #456
```

### Merge Criteria

- [ ] 1 approval (2 for DB/auth changes)
- [ ] All CI checks pass
- [ ] No merge conflicts
- [ ] Linked to issue
- [ ] Tests included

---

## Code Standards

### Pre-Submit Checklist

1. Solves actual problem?
2. Can delete 20% and keep functionality?
3. Scales to 10x load?
4. Edge cases tested?
5. Readable in 6 months?

### Performance

- Benchmark before optimizing
- O(n²) needs justification
- Profile, don't assume

### Testing

- Unit: business logic
- Integration: API endpoints
- Manual: UI changes
- Document test scenarios in PR

---

## Writing Rules

**DO:**
- Active voice
- State facts: "Returns 500" not "seems broken"
- Include versions: "v2.3.1" not "latest"
- Permalink to code lines
- Update issues when context changes

**DON'T:**
- Essays without action
- "Doesn't work" without specifics
- Mix multiple problems in one issue
- Close without resolution comment
- Guess—state what you know

---

## Triage

**Daily review:**
1. All `status:triage` reviewed
2. Priority assigned <24hr
3. Owner assigned or backlog
4. Dependencies identified

**Acceptance:**
- [ ] Title format correct
- [ ] Required sections complete
- [ ] Labels set
- [ ] Reproduction verified (bugs)
- [ ] Metrics defined (features)

---

## Templates

**Location:** `.github/ISSUE_TEMPLATE/`

**Files:**
- `bug_report.md`
- `feature_request.md`
- `task.md`

---

## Rejection

**Close immediately:**
- Template sections deleted
- "Doesn't work" without details
- No search for duplicates
- PRs breaking tests
- Requests for assignment

**Template:**

```
Closing: [specific reason]

To reopen: [what needs to change]
```

---

## Recognition

Quality over quantity.

Maintainer consideration for:
- Clear, complete issues
- Working reproduction steps
- Tests with PRs
- Triage help
