#include <map>
#include <string>
#include <memory>
#include <mutex>
#include <unistd.h>


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

    std::string get(const std::string& key) {

        // readers will wait here while writer writes
        // and other readers drain out
        m_write.lock();
        readers++;
        m_write.unlock();

        auto i = m_cache.find(key);
        if (i == m_cache.end()) {
            readers--;
            return nullptr;
        }
        auto& outpair = *i;
        readers--;
        return std::get<1>(outpair);
    }

    void put(const std::string& key, std::string& value) {
        // writer blocks out other writers and new readers
        m_write.lock();

        // writer waits for readers to drain out
        while(readers > 0) { sleep(0); }

        auto insert_result = m_cache.insert(std::pair<std::string,std::string>(key,value));

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

