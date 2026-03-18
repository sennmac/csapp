# AGENTS.md

## Purpose

This repository is an Obsidian vault used primarily for technical learning, knowledge management, code analysis, and note-taking.

Default assumption in this repository:
- The user is in learning mode.
- Most requests are about understanding technical concepts, analyzing code, summarizing findings, or organizing knowledge.
- Do not assume the goal is to modify production code unless the user explicitly says so.

## Default Behavior

When working in this vault:
- Prefer explanation, synthesis, and note creation over code changes.
- Prefer creating or updating Markdown notes rather than changing Obsidian app settings.
- Default response language is Chinese unless the user asks otherwise.
- Structure answers as: conclusion first, explanation second, examples third.
- If the content is worth preserving, prefer turning it into a reusable note instead of leaving it only in chat.

## Vault Structure

Use these folders consistently:
- `00 Inbox`: quick capture, unorganized notes
- `01 Daily`: daily logs, study progress, issues, next steps
- `02 Projects`: project-specific analysis, context, architecture, troubleshooting
- `03 Areas`: long-term domains such as backend, database, devops
- `04 Notes`: durable technical notes and concept explanations
- `05 Snippets`: reusable commands, SQL, scripts, code fragments
- `06 Archive`: inactive or completed material
- `07 Templates`: note templates

## Where To Write

Choose output location based on content:
- Put daily learning logs in `01 Daily`
- Put project-specific investigations in `02 Projects`
- Put reusable technical explanations in `04 Notes`
- Put commands and small reusable code in `05 Snippets`

Prefer creating new notes instead of overwriting existing notes unless the user explicitly asks to edit an existing file.

## Writing Style For Notes

When creating technical notes, prefer this structure:
- Background
- Core concept
- Mechanism / how it works
- Example
- Common pitfalls
- Practical use
- References

When creating troubleshooting notes, prefer this structure:
- Problem
- Cause
- Investigation
- Fix
- Conclusion

Use specific titles that are easy to search.
Good examples:
- `Node.js 中的 nextTick、Event Loop 与几个常见调度函数`
- `Redis 缓存击穿处理方案`
- `Mac 上 PostgreSQL 端口冲突排查`

Avoid vague titles such as:
- `缓存`
- `问题记录`
- `一些笔记`

## Research Rules

If the user asks for:
- latest information
- best practices
- official recommendations
- version-specific behavior
- current tools, APIs, or product changes

then verify with up-to-date sources, preferably official documentation first.

When browsing is used:
- include links
- distinguish facts from inference
- prefer official docs over blog posts

## Code Analysis Mode

If the user pastes code or asks to analyze code:
- explain what the code does
- identify risks, bugs, edge cases, and assumptions
- connect the code back to the relevant technical concept
- if useful, offer to turn the analysis into a note under `04 Notes` or `02 Projects`

If the user asks to sync code into this vault for analysis, prefer summarizing and extracting the learning value rather than mirroring large codebases blindly.

## Editing Boundaries

Do not do the following unless explicitly requested:
- modify `.obsidian` configuration
- install or remove plugins
- run `git commit`, `git push`, or change remotes
- rename or reorganize large parts of the vault
- overwrite user notes

## Git / Sync

This vault is version-controlled.
- Keep changes minimal and intentional.
- Preserve the user’s note structure and wording where possible.
- Use `sync.sh` only when the user explicitly asks to sync.

## Done Criteria

A task is considered complete when:
- the user’s technical question is answered clearly
- the answer is accurate and appropriately sourced when needed
- if notes were requested, they are placed in the correct folder
- the result is reusable for future learning
