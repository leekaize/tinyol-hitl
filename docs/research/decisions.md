# Design Decisions

**Track non-obvious choices. Enable reversibility.**

---

## D001: Development Environment Architecture

**Date:** 2025-10-12 | **Status:** Active

**Choice:** Silverblue immutable base + Toolbox isolation + host hardware access

**Why:**
- Immutable base prevents toolchain corruption
- Container isolation protects system stability
- Host hardware access avoids USB passthrough complexity
- Balance: safety (containerized builds) + speed (direct hardware)

**Components:**
- System layer: minicom (serial monitor, reboot once)
- Container: Build tools, ARM GCC, CMake
- Host: Hardware I/O, flashing, monitoring

**Trade:** One system reboot for minicom. Worth permanent solution.

**Reversibility:** High (containers disposable, system layer removable).

---

## D002: Static Memory Only

**Date:** 2025-10-12 | **Status:** Active

**Choice:** No malloc. All static allocation.

**Why:** Predictable memory. No fragmentation. Deterministic behavior. Industry best practice.

**Trade:** Cluster count fixed at compile time.

**Reversibility:** Medium (can add dynamic later).

---

## D003: Online K-Means

**Date:** 2025-10-12 | **Status:** Active

**Choice:** Streaming k-means over neural nets/GMM.

**Why:** Simplest (200 lines). Lowest memory (5KB). Proven convergence.

**Trade:** Less expressive than neural nets. Acceptable for degree project.

**Reversibility:** High (neural layer can augment later).

**Baseline to beat:** TinyOL (2021) - 256KB model, batch learning.

---

## D004: Public Apache-2.0

**Date:** 2025-10-12 | **Status:** Active

**Choice:** Public repo with Apache-2.0 license.

**Why:** Timestamps work. Portfolio evidence. Industrial-friendly. Attracts contributors.

**Risk:** Ideas visible to competitors.

**Reversibility:** Low (cannot unpublish).

---

## Template for New Decisions

```markdown
## DXXX: [Title]

**Date:** YYYY-MM-DD | **Status:** Active/Superseded

**Choice:** [What was chosen]

**Why:** [2-3 bullet reasoning]

**Trade:** [Main disadvantage]

**Reversibility:** High/Medium/Low ([how to undo])
```