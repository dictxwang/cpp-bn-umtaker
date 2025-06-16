#ifndef _DBSOURCE_MYSQL_H_
#define _DBSOURCE_MYSQL_H_

 #include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <string>
#include <chrono>

namespace db_source {

    class MySQLConnectionPool {
    private:
        struct ConnectionInfo {
            std::string host;
            std::string user;
            std::string password;
            std::string database;
            unsigned int port;
        } conn_info;

        std::queue<MYSQL*> connections;
        std::mutex pool_mutex;
        std::condition_variable pool_cv;
        size_t pool_size;
        size_t max_pool_size;
        bool is_shutdown;

        // Create new connection
        MYSQL* createConnection();

    public:
        MySQLConnectionPool(const std::string& host,
                            const std::string& user,
                            const std::string& password,
                            const std::string& database,
                            unsigned int port = 3306,
                            size_t initial_size = 5,
                            size_t max_size = 20);

        ~MySQLConnectionPool() {
            shutdown();
        }

        // Get connection from pool
        MYSQL* getConnection();

        // Return connection to pool
        void releaseConnection(MYSQL* conn);

        // Shutdown pool
        void shutdown();

        // Get pool statistics
        size_t getAvailableConnections();

        size_t getTotalConnections() const;
    };
}

#endif