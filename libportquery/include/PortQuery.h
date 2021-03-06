#pragma once

#include <string>
#include <functional>
#include <any>
#include <variant>
#include <memory>


namespace PortQuery {

    enum class PQ_QUERY_RESULT { 
            OPEN = 0,
            CLOSED = 1,
            REJECTED = 2
        };

    using PQ_COLUMN = std::variant<uint16_t, PQ_QUERY_RESULT>;
    using PQ_ROW = std::vector<PQ_COLUMN>;
    using PQCallback = std::function<void(std::any, PQ_ROW)>;


    class SelectStatement;

    class PQConn {

        public:

            using PQ_PORT = uint16_t;
            enum PQ_QUERY_RESULT { 
                OPEN = 0,
                CLOSED = 1,
                REJECTED = 2
            };

            using PQ_COLUMN = std::variant<PQ_PORT, PQ_QUERY_RESULT>;
            using PQ_ROW = std::vector<PQ_COLUMN>;
            using PQCallback = std::function<void(std::any, PQ_ROW)>;

            PQConn(PQCallback const callback=nullptr, 
                    const std::any context=nullptr, 
                    const int timeout=TIMEOUT_DEFAULT,
                    const int threadCount=THREADCOUNT_DEFAULT,
                    const int delayMS=DELAYMS_DEFAULT);

            ~PQConn();
            PQConn(PQConn&&);
            PQConn &operator=(PQConn&&);

            bool prepare(std::string queryString);
            bool run();
            bool finalize();
            bool execute(std::string queryString);


            void setUserCallback(const PQCallback userCallback) {

                m_userCallback = userCallback;
            }

            void setUserData(const std::any userContext) {

                m_userContext = userContext;
            }

            void setTimeout(const int timeout) {

                m_timeout = timeout;
            }

            std::string getErrorString() const {

                return m_errorString;
            }

        private:

            static constexpr int TIMEOUT_DEFAULT = 2;
            int m_timeout;
            static constexpr int THREADCOUNT_DEFAULT = 0;
            int m_threadCount;

            // this should be increased
            static constexpr int DELAYMS_DEFAULT = 0;
            int m_delayMS;

            PQCallback m_userCallback;
            std::any m_userContext;


            using SOSQLSelectStatement = std::unique_ptr<SelectStatement>;
            SOSQLSelectStatement m_selectStatement;
            std::string m_errorString;
    };
}
