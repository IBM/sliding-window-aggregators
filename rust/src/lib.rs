use alga::general::Operator;
use std::ops::Range;

/// An abstract data type which maintains a time-ordered sliding window.
pub trait TimeWindow<Time, Value, BinOp>: Clone
where
    Time: Ord,
    BinOp: Operator,
{
    /// Returns an empty window.
    fn new() -> Self;
    /// Inserts the tuple `(t,v)` at time `t` into the window.
    fn insert(&mut self, t: Time, v: Value);
    /// Removes the tuple `(t,v)` at time `t` from the window (if any).
    fn evict(&mut self, t: Time);
    /// Combines the values in time order and returns the result, e.g., `1+v1+v2+...+vn`.
    fn query(&self) -> Value;
}

/// An abstract data type which maintains a fifo-ordered sliding window.
pub trait FifoWindow<Value, BinOp>: Clone
where
    BinOp: Operator,
{
    /// Returns an empty window.
    fn new() -> Self;
    /// Inserts a value at the back of the window.
    fn push(&mut self, v: Value);
    /// Removes a value at the front of the window (if any).
    fn pop(&mut self);
    /// Combines the values in fifo order and returns the result, e.g., `1+v1+v2+...+vn`.
    fn query(&self) -> Value;
    /// Returns the number of elements inside the window.
    fn len(&self) -> usize;
    /// Returns true if the window contains no elements.
    fn is_empty(&self) -> bool;
}

/// An abstract data type which maintains sliding sub-window aggregates.
pub trait SubWindow<Time, Value>
where
    Time: Ord,
{
    /// Returns the aggregate of a subwindow.
    fn range_query(&self, range: Range<Time>) -> Value;
}

/// Recalculate-From-Scratch
pub mod recalc;

/// Subtract-On-Evict
pub mod soe;

/// Two-Stacks
pub mod two_stacks;

/// Reactive-Aggregator
pub mod reactive;

