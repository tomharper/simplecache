clang++ listener.cpp cache.cpp -o simplecache -std=c++11 -pthread -stdlib=libc++
clang++ client.cpp -o testclient -std=c++11 -pthread
