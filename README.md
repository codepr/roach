Roach
=====

Small timeseries database written in C, designed to efficiently store and
retrieve timestamped data records. Mostly to explore persistent log data
structure applications.

## Basics

Still at a very early stage, the main concepts are

- Fixed size records, to keep things simple each record is represented by just
  a timestamp with nanoseconds precision and a double
- In memory segments: Data is stored in timeseries format, allowing efficient
  querying and retrieval based on timestamps, with the last slice of data in
  memory, composed by two segments (currently covering 15 minutes of data each)
  - The last 15 minutes of data
  - The previous 15 minutes for records out of order, totalling 30 minutes
- Commit Log: Persistence is achieved using a commit log at the base, ensuring
  durability of data on disk.
- Write-Ahead Log (WAL): In-memory segments are managed using a write-ahead
  log, providing durability and recovery in case of crashes or failures.


## TODO

- Duplicate points policy
- CRC32 of records for data integrity
- Adopt an arena for memory allocations
- Memory mapped indexes, above a threshold enable binary search
- Schema definitions
- Server: Text based protocol, a simplified SQL-like would be cool

## Usage

At the current stage, no server attached, just a tiny library with some crude APIs;

- `tsdb_init(1)` creates a new database
- `tsdb_close(1)` closes the database
- `ts_create(3)` creates a new timeseries in a given database
- `ts_get(2)` retrieve an existing timeseries from a database
- `ts_insert(3)` inserts a new point into the timeseries
- `ts_find(3)` finds a point inside the timeseries
- `ts_range(4)` finds a range of points in the timeseries, returning a vector
  with the results
- `ts_close(1)` closes a timeseries

Plus a few other helpers.

### As a library

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
    Timeseries_DB *db = tsdb_init("example_ts");
    // Use timeseries functions...
    tsdb_close(db);
    return 0;
}

```

Build it linking the library

```bash
gcc -o my_project main.c -I/path/to/timeseries/include -L/path/to/timeseries -ltimeseries
```

To run it

```bash
LD_LIBRARY_PATH=/path/to/timeseries.so ./my_project
```

### Quickstart

```c
#include "timeseries.h"

int main() {
    // Initialize the database
    Timeseries_DB *db = tsdb_init("testdb");
    if (!db)
        abort();

    // Create a timeseries, retention is not implemented yet
    Timeseries *ts = ts_create(db, "temperatures", 0, DP_IGNORE);
    if (!ts)
        abort();

    // Insert records into the timeseries
    ts_insert(&ts, 1710033421702081792, 25.5);
    ts_insert(&ts, 1710033422047657984, 26.0);

    // Find a record by timestamp
    Record r;
    int result = ts_find(&ts, 1710033422047657984, &r);
    if (result == 0) {
        printf("Record found: timestamp=%lu, value=%.2lf\n", r.timestamp, r.value);
    } else {
        printf("Record not found.\n");
    }

    // Release the timeseries
    ts_close(&ts);

    // Close the database
    tsdb_close(db);

    return 0;
}

```

## Roach server draft

Event based server (rely on [ev](https://github.com/codepr/ev.git) at least
initially), TCP as the main transport protocol, text-based custom protocol
inspired by RESP but simpler.

### Simple query language

Definition of a simple, text-based format for clients to interact with the
server, allowing them to send commands and receive responses.

#### Basic outline

- **Text-Based Format:** Use a text-based format where each command and
  response is represented as a single line of text.
- **Commands:** Define a set of commands that clients can send to the server to
  perform various operations such as inserting data, querying data, and
  managing the database.
- **Responses:** Define the format of responses that the server sends back to
  clients after processing commands. Responses should provide relevant
  information or acknowledge the completion of the requested operation.

#### Core commands

Define the basic operations in a SQL-like query language

- **CREATE** creates a database or a timeseries

  `CREATE <database name>`
  
  `CREATE <timeseries name> INTO <database name> [<retention period>] [<duplication policy>]`

- **INSERT** insertion of point(s) in a timeseries

  `INSERT <timeseries name> INTO <database name> <timestamp | *> <value>, ...`

- **SELECT** query a timeseries, selection of point(s) and aggregations

  `SELECT <timeseries name> FROM <database name> RANGE <start_timestamp> TO <end_timestamp> WHERE value [>|<|=|<=|>=|!=] <literal> AGGREGATE [AVG|MIN|MAX] BY <literal>`

- **DELETE** delete a timeseries or a database

  `DELETE <database name>`
  `DELETE <timeseries name> FROM <database name>`

#### Flow:

1. **Client Sends Command:** Clients send commands to the server in the
       specified text format.

2. **Server Parses Command:** The server parses the received command and
       executes the corresponding operation on the timeseries database.

3. **Server Sends Response:** After processing the command, the server sends a
       response back to the client indicating the result of the operation or
       providing requested data.

4. **Client Processes Response:** Clients receive and process the response from
       the server, handling success or error conditions accordingly.
