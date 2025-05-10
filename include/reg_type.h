#pragma once
#include <optional>
#include <stdexcept>

/**
 * @brief A flexible template class representing a register for CPU simulation
 *
 * @tparam T The type of data to be stored in the register
 *
 * The reg template class models a simple sequential register with the following key features:
 * - Two-phase operation with current and next values
 * - Clock-based updates that move next to current
 * - Enable/disable functionality for pipeline control
 * - Type-safe storage for any data type
 * - Pipeline flow operator (>>) for connecting stages
 *
 * @note This class is designed for functional simulation, not accurate
 *       hardware modeling.
 *
 * Usage Example:
 * @code
 * reg<int> counter(0);     // Create register with initial value 0
 * counter.setNext(5);      // Set next value to 5
 * counter.clock();         // Update current to 5
 *
 * reg<MyData> stage1(data);
 * reg<MyData> stage2;
 * stage1 >> stage2;        // Copy stage1's current to stage2's next
 * stage2.clock();          // Update stage2
 * @endcode
 */
template <typename T>
class reg {
   private:
    std::optional<T> current_value;
    std::optional<T> next_value;
    bool enable;

   public:
    // Default constructor - creates empty/invalid register if not initialized with an object/value
    reg() : current_value(std::nullopt), next_value(std::nullopt), enable(true) {}

    // Constructor with initial value
    explicit reg(const T& initial_value)
        : current_value(initial_value), next_value(std::nullopt), enable(true) {}

    // Move constructor for efficiency to prevent copying objects when not needed
    explicit reg(T&& initial_value)
        : current_value(std::move(initial_value)), next_value(std::nullopt), enable(true) {}

    // Get current value (throws if not valid)
    const T& current() const {
        if (!current_value.has_value()) {
            throw std::runtime_error("Register has no valid current value");
        }
        return *current_value;
    }

    // Get next value
    const std::optional<T>& next() const { return next_value; }

    // Check if register has valid current value
    bool isValid() const { return current_value.has_value(); }

    // Set next value (copy version - used for lvalues)
    void setNext(const T& value) {
        if (enable) {
            next_value = value;
        }
    }

    // Set next value (move version - used for rvalues and std::move)
    void setNext(T&& value) {
        if (enable) {
            next_value = std::move(value);
        }
    }

    // Clock the register
    void clock() {
        if (enable && next_value.has_value()) {
            current_value = std::move(*next_value);
            next_value.reset();
        }
    }

    // Clear the register (removes current value)
    void clear() {
        current_value.reset();
        next_value.reset();
    }

    // Enable/disable the register
    void setEnable(bool enabled) { enable = enabled; }

    bool isEnabled() const { return enable; }

    // Pipeline flow operator
    reg& operator>>(reg& downstream) {
        if (isValid() && enable) {
            downstream.setNext(*current_value);
        }
        return downstream;
    }

    // Assignment operator (sets next value)
    reg& operator=(const T& value) {
        setNext(value);
        return *this;
    }

    reg& operator=(T&& value) {
        setNext(std::move(value));
        return *this;
    }

    // Update template function which allows for custom operations
    // This function takes a callable (like a lambda) and applies it to the current value.
    // Might be useful for some complex operations in the pipeline.
    template <typename Func>
    void update(Func&& func) {
        if (isValid() && enable) {
            setNext(func(*current_value));
        }
    }

    // Safe access with default
    T value_or(const T& default_value) const { return current_value.value_or(default_value); }
};
