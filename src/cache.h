#include <map>
#include <string>
#include <memory>
#include <mutex>
#include <unistd.h>
#include <iostream>


class SimpleCache {
public:
    SimpleCache()
    {
        readers = 0;
    }

    ~SimpleCache()
    {
    }

    // singleton
    static std::unique_ptr<SimpleCache> instance;
    static SimpleCache* getCreateInstance() {
        if(!instance) {
            instance.reset(new SimpleCache());
        }
        return instance.get();
    }

    void get(const char* key, std::string& value) {
/*
        std::map<std::string,std::string>::iterator it = m_cache.begin();
        std::cout << "m_cache contains:\n";
        for (it=m_cache.begin(); it!=m_cache.end(); ++it)
          std::cout << it->first.c_str() << " => " << it->second.c_str() << '\n';
*/
        // readers will wait here while writer writes
        // and other readers drain out
        m_write.lock();
        readers++;
        m_write.unlock();

        auto i = m_cache.find(key);
        if (i == m_cache.end()) {
            readers--;
            return;
        }
        //printf(" --- %s %lu", std::get<1>(*i).c_str(), std::get<1>(*i).length());
        value = std::get<1>(*i);
        readers--;
    }
    void put(const char* key, const char* value) {

        // writer blocks out other writers and new readers
        m_write.lock();

        // writer waits for readers to drain out
        while(readers > 0) { sleep(0); }

        m_cache.insert(std::make_pair(key,value));
        
        m_write.unlock();

        // todo laters:

        // if item expires, tell the timer thread to deal with it here
        // we would add this to a timer data structure

        // here we would also check to see if we need to re-slab or re-hash
    }

    bool remove(const std::string& key) {
        // same as above
        m_write.lock();
        while(readers > 0) { sleep(0); }

        auto i = m_cache.find(key);
        if (i == m_cache.end()) {
            m_write.unlock();
            return false;
        }
        m_cache.erase(i);
        m_write.unlock();
        return true;
    }
private:

    // uber simple data structure
    std::map<std::string,std::string> m_cache;

    // basic read/write
    std::mutex m_write;
    std::atomic<int> readers;

};

