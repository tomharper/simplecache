General

1. I really liked the approach jaysonsantos took with using twisted & python, seemed like a good way to quickly prototype something like this out for a simple demo server

2. I used c/c++ because I thought it would be fun, and most servers for media are written in c/c++

3. I chose a simple threading model mostly because I wanted to do something quick that would not involve IPC, and could speak to the read/write issues, and would let me easily access my simple cache

4. I really liked the seastar project approach to doing this, if I had more time I would have done it that way: non-blocking, asynch and lightweight- but it would take more time to think through

Building and Running

  - I only built/tested this on OSX, so it probably won't work on linux or windows or anywhere else

  - there is no make file but a ./make.sh script in src will create the listener program

  - run the listener program as ./simplecache once you build it- it should then be listening on port 11211

  - don't run the testclient program except to see what happens with bad input. 

  - otherwise, from a separate command line or run your other favorite memcached test to port 11211 -
  this will fail every test other than get/set basic

Python Setup

  - made a test script for bmemcached python, python crashes on the get ... didn't have time to debug this but it is expecting zlib compression support

  - run setup_python.sh to setup virtualenv 

  - source memc/bin/activate

  - python testscript.py

Notes on Implementation

  This cache is uber simple: it would be fun to add:

  - better locking for reader/writers- this would take a little more time and understanding usage patterns- right now this is the minimum safe implementation.  Ideally we could isolate locking to subareas of memory or even get to key level locks

  - better cache- std::map is not the fastest or most efficient way to store lots of data- boost::unordered_map is faster for example- but that would just be the start- use slabs, etc

  - write a distributed cache

  - run the cache itself possibly in another thread context with better isolation

  - still need to do some basic stuff like deletes, expiration, auth, multi-read, multi-write

  - write a more efficient protocol- the current one is a little awkward but good enough

