#pragma once

namespace ct
{
struct scope;
struct chunk;

namespace detail
{
void push_scope(scope& s);
void pop_scope(scope& s);

chunk* add_chunk(scope& s, chunk&& c);

// sets size of current chunk correct
void update_current_chunk_size();

void mark_as_orphaned(scope& s);
}
}
