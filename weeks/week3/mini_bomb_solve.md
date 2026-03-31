# Mini Bomb Solve Record

## Goal
- Practice tracing control flow, stack frames, loops, and recursive calls in `lldb`.
- Solve the custom `mini_bomb` without relying on source while reviewing.

## Binary
- Build: `make`
- Run: `./mini_bomb`
- Disassemble: `make disasm`

## Suggested LLDB Workflow
1. `lldb ./mini_bomb`
2. `b main`
3. `run`
4. `disassemble -n phase_1`
5. `thread step-inst`
6. `register read`
7. `memory read -f x -s1 -c32 <addr>`

## Phase Notes
### Phase 1
- Input shape:
- Key observations:
- Final answer:

### Phase 2
- Input shape:
- Key observations:
- Final answer:

### Phase 3
- Input shape:
- Key observations:
- Final answer:

### Phase 4
- Input shape:
- Key observations:
- Final answer:

## Postmortem
- Which phase took the longest:
- Which assembly pattern became clearer:
- Which LLDB commands were most useful:
