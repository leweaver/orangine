# Coding Style Guide
## Forward Declarations
Don't use for model classes in 'Manager' class header files (ie, for the things that the manager class managers)
Do use when referencing manager classes in header files

## Pointers vs References
Use reference wherever you can, pointers wherever you must. Avoid pointers until you can't.

Prefer references on Public facing API; pointers typically appear in the implementation.

When passing in a parameter that may not have a value, consider passing in a std::optional<T &>.