/*
 *Copyright (c) 2013-2013, yinqiwen <yinqiwen@gmail.com>
 *All rights reserved.
 *
 *Redistribution and use in source and binary forms, with or without
 *modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of Redis nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CACHE_SERVICE_HPP_
#define CACHE_SERVICE_HPP_

#include "common.hpp"
#include "data_format.hpp"
#include "util/atomic.hpp"
#include "util/lru.hpp"
#include "util/thread/thread_mutex.hpp"
#include "util/thread/thread_mutex_lock.hpp"
#include "util/thread/thread_local.hpp"

namespace ardb
{
    class CacheItem
    {
        protected:
            uint32 m_estimate_mem_size;
        private:
            uint32 m_ref;
            uint8 m_type;
        public:
            CacheItem(uint8 type) :
                    m_estimate_mem_size(0), m_ref(1), m_type(type)
            {
            }
            virtual ~CacheItem()
            {
            }
            uint8 GetType()
            {
                return m_type;
            }
            uint32 GetEstimateMemorySize()
            {
                return m_estimate_mem_size;
            }
            void IncRef()
            {
                atomic_add_uint32(&m_ref, 1);
            }
            void DecRef()
            {
                if (atomic_sub_uint32(&m_ref, 1) == 0)
                {
                    delete this;
                }
            }
    };

    struct ZSetCaheElement
    {
            double score;
            std::string value;
            std::string attr;
            ZSetCaheElement(double s = 0, const std::string v = "") :
                    score(s), value(v)
            {
            }
            bool operator<(const ZSetCaheElement& other) const
            {
                if (score < other.score)
                {
                    return true;
                }
                if (score > other.score)
                {
                    return false;
                }
                return value < other.value;
            }
    };
    typedef std::deque<ZSetCaheElement> ZSetCaheElementDeque;
    class L1Cache;
    class ZSetCache: public CacheItem
    {
        private:
            ZSetCaheElementDeque m_cache;
            //ThreadMutex m_cache_mutex;
            friend class L1Cache;
            void DirectAdd(ValueData& score, ValueData& v, ValueData& attr);
        public:
            ZSetCache();
            void GetRange(const ZRangeSpec& range, bool with_scores, bool  with_attrs, ValueDataArray& res);

    };

    /*
     * Only zset cache supported now
     */
    class Ardb;
    class L1Cache
    {
        private:
            Ardb* m_db;
            uint32_t m_estimate_mem_size;
            LRUCache<DBItemKey, CacheItem*> m_cache;
            ThreadMutex m_cache_mutex;

            void LoadZSetCache(const DBItemKey& key, ZSetCache* item);
        public:
            L1Cache(Ardb* db);
            int Evict(const DBID& dbid, const Slice& key);
            int Load(const DBID& dbid, const Slice& key);
            bool IsCached(const DBID& dbid, const Slice& key);
            CacheItem* Get(const DBID& dbid, const Slice& key, KeyType type);
            void Recycle(CacheItem* item);
            uint32 GetEstimateMemorySize()
            {
                return m_estimate_mem_size;
            }
            uint32 GetEntrySize()
            {
                return m_cache.Size();
            }
            ~L1Cache();
    };
}

#endif /* CACHE_SERVICE_HPP_ */
