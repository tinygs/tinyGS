---
applyTo: "**"
---

## General MCP Rules
- Actively use available MCP tools **before** making assumptions.
- Prefer tool-derived knowledge over implicit model knowledge.
- If relevant information might be missing, **check the memory first**.

## Persistent Memory: memvid

You have access to a persistent memory called `memvid`.

### Available Tools
- `memvid_search`: Search for relevant project or contextual knowledge
- `memvid_ask`: Ask natural-language questions to the memory
- `memvid_timeline`: Retrieve information in chronological order
- `memvid_store`: Store new, important information
- `memvid_stats`: Get an overview of the memory

---

## When to QUERY memvid?

Use `memvid_search` or `memvid_ask` when:
- the question is project- or domain-specific
- past decisions, conventions, or architecture may be relevant
- the user refers to "how we have done this before"
- consistency with previous answers is important

**Prefer querying over guessing.**

---

## When to UPDATE memvid?

Use `memvid_store` **only** when the information:
- is long-term relevant
- is project-specific
- is likely to be useful again in the future

### Good Examples to Store
- Architectural decisions
- Conventions (e.g. naming, patterns)
- Tooling guidelines
- Workflows
- User preferences
- Project goals or scope definitions

### Do NOT Store
- Temporary tasks
- One-off errors
- Pure chat phrasing
- Obvious general knowledge

---

## Decision Logic (Important)

1. **Understand the request**
2. **Check: Could memvid contain relevant knowledge?**
   - Yes → query it
   - No → answer directly
3. **When learning new, stable facts → store them**

---

## Quality & Transparency
- Do not explicitly mention tool names in user-facing responses.
- Use tools internally, respond naturally externally.
- If the memory is empty or unclear, proceed cautiously.

---

## Anti-Patterns
- ❌ Writing to memvid on every response
- ❌ Storing duplicate information
- ❌ Ignoring tool usage when context is required
- ❌ Answering uncertainly when a query would be possible

---

## Example (Internal Behavior)

**User:** "Why did we choose Redis?"  
→ `memvid_search("Redis decision")`  
→ Respond based on the stored decision

**User explains a new architecture rule**  
→ `memvid_store(...)`
