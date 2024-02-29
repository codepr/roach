#include "persistent_index.h"

static const size_t ENTRY_SIZE = 16;

int p_index_init(Persistent_Index *pi, const char *path,
                 uint64_t base_timestamp, uint32_t interval);

int p_index_close(Persistent_Index *pi);

int p_index_from_disk(Persistent_Index *pi, const char *path,
                      uint64_t base_timestamp, uint32_t interval);

int p_index_append_offset(Persistent_Index *pi, uint64_t relative_ts,
                          uint64_t offset);

int p_index_find_offset(const Persistent_Index *pi, uint64_t timestamp,
                        Range *r);
