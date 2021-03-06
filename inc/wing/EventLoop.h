#pragma once

#include "wing/QueryPool.h"
#include "wing/ConnectionInfo.h"

#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

#include <uv.h>

namespace wing
{

class EventLoop
{
    friend QueryHandle; ///< Required for libuv callbacks.
public:
    /**
     * Creates an event loop to execute asynchronous MySQL queries.
     * @param connection The connection information for the MySQL Server.
     */
    explicit EventLoop(
        ConnectionInfo connection
    );

    ~EventLoop();

    EventLoop(const EventLoop& copy) = delete;                              ///< No copying
    EventLoop(EventLoop&& move) = default;                                  ///< Can move
    auto operator = (const EventLoop& copy_assign) -> EventLoop& = delete;  ///< No copy assign
    auto operator = (EventLoop&& move_assign) -> EventLoop& = default;      ///< Can move assign

    /**
     * @return True if the query and connect threads are running.
     */
    auto IsRunning() -> bool;

    /**
     * This count does not include the number of queries that
     * require a new connection to the MySQL server and are
     * queued to connect.
     * @return Gets an active query count that is being executed.
     */
    auto GetActiveQueryCount() const -> uint64_t;

    /**
     * Stops the event loop and shuts down all current queries.
     * Queries not finished will be terminated via the IQueryCallback
     * with a QueryStatus::SHUTDOWN_IN_PROGRESS.
     */
    auto Stop() -> void;

    /**
     * All queries run through this event loop will save the
     * connection to the MySQL server in this pool for future use
     * reducing the need to re-connect.
     * @return Gets the QueryPool handle.
     */
    auto GetQueryPool() -> QueryPool&;

    /**
     * Starts an asynchronous query.
     * @param query The query to start.
     * @return True if the query started.
     */
    auto StartQuery(
        Query query
    ) -> bool;

private:
    QueryPool m_query_pool;

    std::atomic<bool> m_is_query_running;
    std::atomic<bool> m_is_stopping;
    std::atomic<uint64_t> m_active_query_count;

    std::thread m_background_query_thread;
    uv_loop_t* m_query_loop;
    uv_async_t m_query_async;
    std::atomic<bool> m_query_async_closed;
    std::mutex m_pending_queries_lock;
    std::vector<Query> m_pending_queries;
    std::vector<Query> m_grabbed_queries;

    auto run_queries() -> void;

    auto callOnComplete(
        Query query
    ) -> void;
    auto callOnComplete(
        std::unique_ptr<QueryHandle> query_handle
    ) -> void;

    auto onClose(
        uv_handle_t* handle
    ) -> void;

    auto requestsAcceptForQueryAsync(
        uv_async_t* async
    ) -> void;

    friend auto uv_close_event_loop_callback(
        uv_handle_t* handle
    ) -> void;

    friend auto on_complete_uv_query_execute_callback(
        uv_work_t* req,
        int status
    ) -> void;

    friend auto requests_accept_for_query_async(
        uv_async_t* async
    ) -> void;
};

} // wing
