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

#### As a library

Build the `libtimeseries.so` first

```bash
make
```

In the target project, a generic hello world

```c
#include <stdio.h>
#include "timeseries.h"

int main(void) {
    // Example usage of timeseries library functions
    Timeseries *ts = ts_create("example_ts");
    // Use timeseries functions...
    ts_destroy(ts);
    return 0;
}

```

Build it linking the library

```bash
gcc -o my_project main.c -I/path/to/timeseries/include -L/path/to/timeseries -ltimeseries
```

#### Quickstart

```c
#include "timeseries.h"

int main() {
    // Initialize the database
    Timeseries_DB *db = tsdb_init("testdb");
    if (!db)
        abort();

    // Create a timeseries, retention is not implemented yet
    Timeseries *ts = ts_create(db, "temperatures", 0);
    if (!ts)
        abort();

    // Insert records into the timeseries
    ts_set_record(&ts, 1710033421702081792, 25.5);
    ts_set_record(&ts, 1710033422047657984, 26.0);

    // Find a record by timestamp
    Record r;
    int result = ts_find_record(&ts, 1710033422047657984, &r);
    if (result == 0) {
        printf("Record found: timestamp=%lu, value=%.2lf\n", r.timestamp, r.value);
    } else {
        printf("Record not found.\n");
    }

    // Release the timeseries
    ts_destroy(&ts);

    // Close the database
    tsdb_close(db);

    return 0;
}

```
