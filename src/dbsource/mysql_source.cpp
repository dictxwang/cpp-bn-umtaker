#include "mysql_source.h"

namespace db_source {

    MYSQL* MySQLConnectionPool::createConnection() {
        MYSQL* conn = mysql_init(nullptr);
        if (!conn) {
            throw std::runtime_error("mysql_init failed");
        }

        // Set connection options
        mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");

        // Connect
        if (!mysql_real_connect(conn,
                                conn_info.host.c_str(),
                                conn_info.user.c_str(),
                                conn_info.password.c_str(),
                                conn_info.database.c_str(),
                                conn_info.port,
                                nullptr, 0)) {
            std::string error = mysql_error(conn);
            mysql_close(conn);
            throw std::runtime_error("Connection failed: " + error);
        }

        return conn;
    }

    MySQLConnectionPool::MySQLConnectionPool(const std::string& host,
                        const std::string& user,
                        const std::string& password,
                        const std::string& database,
                        unsigned int port = 3306,
                        size_t initial_size = 5,
                        size_t max_size = 20)
        : pool_size(0), max_pool_size(max_size), is_shutdown(false) {

        conn_info = {host, user, password, database, port};

        // Create initial connections
        for (size_t i = 0; i < initial_size; ++i) {
            try {
                connections.push(createConnection());
                pool_size++;
            } catch (const std::exception& e) {
                throw e;
            }
        }
    }
    // Get connection from pool
    MYSQL* MySQLConnectionPool::getConnection() {
        std::unique_lock<std::mutex> lock(pool_mutex);

        // Wait for available connection
        pool_cv.wait(lock, [this] {
            return !connections.empty() || is_shutdown || pool_size < max_pool_size;
        });

        if (is_shutdown) {
            return nullptr;
        }

        // Try to get existing connection
        if (!connections.empty()) {
            MYSQL* conn = connections.front();
            connections.pop();

            // Check if connection is still alive
            if (mysql_ping(conn) != 0) {
                mysql_close(conn);
                pool_size--;

                // Create new connection
                try {
                    conn = createConnection();
                    pool_size++;
                } catch (const std::exception& e) {
                    throw std::runtime_error("Failed to create replacement connection");
                }
            }

            return conn;
        }

        // Create new connection if pool not at max
        if (pool_size < max_pool_size) {
            try {
                MYSQL* conn = createConnection();
                pool_size++;
                return conn;
            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to create new connection");
            }
        }

        // Should not reach here
        throw std::runtime_error("No connections available");
    }

    void MySQLConnectionPool::releaseConnection(MYSQL* conn) {
        if (!conn) return;

        std::lock_guard<std::mutex> lock(pool_mutex);

        if (is_shutdown) {
            mysql_close(conn);
            pool_size--;
            return;
        }

        connections.push(conn);
        pool_cv.notify_one();
    }

    // Shutdown pool
    void MySQLConnectionPool::shutdown() {
        std::lock_guard<std::mutex> lock(pool_mutex);
        is_shutdown = true;

        while (!connections.empty()) {
            MYSQL* conn = connections.front();
            connections.pop();
            mysql_close(conn);
        }

        pool_cv.notify_all();
    }

    size_t MySQLConnectionPool::getAvailableConnections() {
        std::lock_guard<std::mutex> lock(pool_mutex);
        return connections.size();
    }

    size_t MySQLConnectionPool::getTotalConnections() const {
        return pool_size;
    }
}