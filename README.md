# optional_task
Optional contained value management interface implementation from C++17 standard, excluding:
 - satisfying all `is_trivially_*` traits, except for `is_trivially_copyable` and `is_trivially_destructible`
 - `value` and `value_or` functions
 - `swap`
 - checking validity overhead before accessing with `operator*` (no exceptions)
 - `make_optional`
 - `hash`
