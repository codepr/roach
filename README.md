Roach
=====

Small timeseries database written in C, designed to efficiently store and
retrieve timestamped data records. Mostly to explore persistent log data
structure applications.

### Basics

Still at the very early stages, the main concepts are

- In memory segments: Data is stored in timeseries format, allowing efficient
  querying and retrieval based on timestamps, with the last slice of data in
  memory, composed by 2 segments (currently covering 15 minutes of data each)
  - The last 15 minutes of data
  - The previous 15 minutes for records out of order, totalling 30 minutes
- Commit Log: Persistence is achieved using a commit log at the base, ensuring
  durability of data on disk.
- Write-Ahead Log (WAL): In-memory segments are managed using a write-ahead
  log, providing durability and recovery in case of crashes or failures.


### TODO

- Adopt an arena for allocations
- Text based protocol
- TCP server
- Memory mapped indexes
- Schema definitions

### Usage

At the current stage, no server attached, just a tiny library with some crude APIs.

```c
#include "timeseries.h"

int main() {
    // Initialize the database, retention is not implemented yet
    // ts_new takes care of creating the metric
    Timeseries ts = ts_create("data", 0);

    // Insert records into the timeseries
    ts_set_record(&ts, 1609459200000000000, 25.5);
    ts_set_record(&ts, 1609459201000000000, 26.0);

    // Find a record by timestamp
    Record r;
    int result = ts_find_record(&ts, 1609459201000000000, &r);
    if (result == 0) {
        printf("Record found: timestamp=%" PRIu64 ", value=%.2f\n", r.timestamp, r.value);
    } else {
        printf("Record not found.\n");
    }

    // Close the database
    ts_destroy(&ts);

    return 0;
}

```
