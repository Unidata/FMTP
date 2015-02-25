/**
 * Copyright 2015 University Corporation for Atmospheric Research. All rights
 * reserved. See the the file COPYRIGHT in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: ProdIndexDelayQueue.h
 * @author: Steven R. Emmerson
 *
 * This file declares the API of a thread-safe delay-queue of product-indexes.
 */

#ifndef PRODINDEXDELAYQUEUE_H_
#define PRODINDEXDELAYQUEUE_H_

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <sys/types.h>
#include <vector>

class ProdIndexDelayQueue {
public:
    /**
     * Constructs an instance.
     */
    ProdIndexDelayQueue();
    /**
     * Adds an element to the queue.
     *
     * @param[in] index    The product-index.
     * @param[in] seconds  The duration, in seconds, to the reveal-time of the
     *                     product-index (i.e., until the element can be
     *                     retrieved via `pop()`).
     */
    void      push(u_int32_t index, double seconds);
    /**
     * Returns the product-index whose reveal-time is the earliest and not later
     * than the current time and removes it from the queue. Blocks until such a
     * product-index exists.
     *
     * @return  The product-index with the earliest reveal-time that's not later
     *          than the current time.
     */
    u_int32_t pop();
    /**
     * Unconditionally returns the product-index whose reveal-time is the
     * earliest and removes it from the queue. Undefined behavior results if the
     * queue is empty.
     *
     * @return  The product-index with the earliest reveal-time.
     */
    u_int32_t get();
    /**
     * Returns the number of product-indexes in the queue.
     *
     * @return  The number of product-indexes in the queue.
     */
    size_t size();

private:
    /**
     * An element in the priority-queue of a `ProdIndexDelayQueue` instance.
     */
    class Element {
    public:
        /**
         * Constructs an instance.
         *
         * @param[in] index    The product-index.
         * @param[in] seconds  The duration, in seconds, from the current time
         *                     until the reveal-time of the element. May be
         *                     negative.
         */
        Element(uint32_t index, double seconds);
        /**
         * Constructs an instance.
         *
         * @param[in] index    The product-index.
         * @param[in] seconds  The duration, in seconds, from the current time
         *                     until the reveal-time of the element. May be
         *                     negative.
         */
        bool      isLaterThan(const Element& that) const;
        /**
         * Returns the product-index.
         *
         * @return  The product-index.
         */
        u_int32_t getIndex() const {return index;}
        /**
         * Returns the reveal-time.
         *
         * @return  The reveal-time.
         */
        const std::chrono::system_clock::time_point&
                  getTime() const {return when;}
    private:
        /**
         * The product-index.
         */
        u_int32_t                             index;
        /**
         * The reveal-time.
         */
        std::chrono::system_clock::time_point when;
    };

    /**
     * Constructs an instance.
     *
     * @param[in] index    The product-index.
     * @param[in] seconds  The duration, in seconds, from the current time until
     *                     the reveal-time of the element. May be negative.
     */
    static bool
            isLowerPriority(const ProdIndexDelayQueue::Element& a,
                const ProdIndexDelayQueue::Element& b);
    /**
     * Returns the time associated with the highest-priority element in the
     * queue.
     *
     * @pre        The instance is locked.
     * @param[in]  The lock on the instance.
     * @return     The time at which the earliest element will be ready.
     */
    const std::chrono::system_clock::time_point&
            getEarliestTime(std::unique_lock<std::mutex>& lock);

    /**
     * The mutex for protecting the priority-queue.
     */
    std::mutex                          mutex;
    /**
     * The condition variable for signaling when the priority-queue has been
     * modified.
     */
    std::condition_variable             cond;
    /**
     * The priority-queue.
     */
    std::priority_queue<Element, std::vector<Element>,
            decltype(&isLowerPriority)> priQ;
};

#endif /* PRODINDEXDELAYQUEUE_H_ */
