# Repository Rules

- Use LF line endings for all text files.
- Do not introduce RTTI dependencies. Avoid `dynamic_cast`, `typeid`, and APIs that require RTTI unless the change is explicitly approved.
- Keep comments and Doxygen useful but sparse: do not document behavior that is obvious from a name or signature.
- Always add concise comments for non-obvious public API behavior, invariants, ownership rules, escaping rules, defaults, or compatibility constraints.
