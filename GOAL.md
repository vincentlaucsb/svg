# Goal: Remove RTTI From Element Typing

## Context

The current uncommitted v0.3.0 work introduces new typed lookup and root-owner logic that uses `dynamic_cast`. The existing code already used `typeid` in child traversal helpers, so the library was not fully RTTI-free before this change. Still, the new implementation expands the dependency and makes RTTI harder for downstream users to avoid.

We want the library to remain friendly to projects that build with RTTI disabled, such as `-fno-rtti` or MSVC `/GR-`.

## Direction

Replace RTTI-based element identification with an explicit element kind facility:

- Add an `ElementKind` enum covering built-in SVG element classes.
- Include a `Custom` kind for user-defined elements.
- Give each built-in element class a `static_kind` constant and a virtual `kind() const` override.
- Keep `tag()` overridable so user-defined elements can serialize SVG tags that the library has not implemented yet.
- Make the default `tag()` implementation delegate to enum-based tag lookup for built-in element kinds.
- Centralize SVG tag names in one enum-to-string function, for example `tag_name(ElementKind kind)`.
- Update typed traversal helpers and typed `get_element_by_id<T>()` to compare `element->kind()` with `T::static_kind`, then use `static_cast<T*>`.

Built-in elements should normally only need to provide `static_kind` and `kind()`. Custom elements should return `ElementKind::Custom` and override `tag()` with their SVG tag name.

## Root Ownership

Do not use `dynamic_cast<SVG*>` to discover the root SVG element. Prefer one of these RTTI-free approaches:

- Store an `SVG* owner_` pointer on `Element` and propagate it through subtrees when children are inserted or moved.
- Or provide a virtual root accessor where `SVG` returns `this` and non-root elements return their propagated owner pointer.

This keeps ID indexing and parent traversal independent of RTTI.

## Tradeoffs

The enum approach gives exact built-in element kind matching. It does not automatically treat arbitrary downstream subclasses as their base SVG element type unless those subclasses intentionally return the base kind. That is acceptable for the current library shape and is preferable to requiring RTTI.

Custom elements can serialize correctly by overriding `tag()`, but typed lookup for arbitrary custom subclasses is intentionally limited without RTTI. If every custom element returns `ElementKind::Custom`, `get_element_by_id<CustomType>()` cannot safely distinguish one custom subclass from another. Custom elements should remain retrievable through untyped APIs such as `Element* get_element_by_id(...)`. A future extension could add user-provided custom type keys if typed custom lookup becomes important.

## Acceptance Criteria

- No `dynamic_cast` remains in `src/svg.hpp`.
- No `typeid` remains in `src/svg.hpp`.
- Typed child traversal and typed ID lookup continue to work for built-in element classes.
- Custom element classes can return `ElementKind::Custom`, override `tag()`, and serialize unimplemented SVG tags.
- Untyped lookup continues to support custom elements.
- Existing render, traversal, class list, geometry, layout, and style tests pass.
- New or updated tests cover enum-based typed lookup/traversal, root ownership behavior, and custom element serialization.
