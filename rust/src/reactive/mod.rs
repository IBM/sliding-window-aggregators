pub(crate) mod flat_fat;

use crate::reactive::flat_fat::{FlatFAT, FAT};
use crate::FifoWindow;
use alga::general::AbstractMonoid;
use alga::general::Operator;

#[derive(Clone)]
pub struct Reactive<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    fat: FlatFAT<Value, BinOp>,
    size: usize,
    front: usize,
    back: usize,
}

impl<Value, BinOp> Reactive<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    /// Returns a Reactive Aggregator with a pre-allocated `capacity`
    pub fn with_capacity(capacity: usize) -> Self {
        Self {
            fat: FlatFAT::with_capacity(capacity),
            size: 0,
            front: 0,
            back: 0,
        }
    }
    fn inverted(&self) -> bool {
        self.front > self.back
    }
    fn resize(&mut self, capacity: usize) {
        let leaves = self.fat.leaves();
        let mut fat = FlatFAT::with_capacity(capacity);
        if self.inverted() {
            fat.update_ordered(
                leaves[self.front..]
                    .iter()
                    .chain(leaves[..self.back].iter())
                    .cloned(),
            );
        } else {
            fat.update_ordered(leaves[self.front..self.back].iter().cloned());
        }
        self.fat = fat;
        self.front = 0;
        self.back = self.size;
    }
}

impl<Value, BinOp> FifoWindow<Value, BinOp> for Reactive<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    fn new() -> Self {
        Self {
            fat: FlatFAT::with_capacity(2),
            size: 0,
            front: 0,
            back: 0,
        }
    }
    fn push(&mut self, v: Value) {
        self.fat.update([(self.back, v)].iter().cloned());
        self.size += 1;
        self.back += 1;
        if self.size > (3 * self.fat.capacity) / 4 {
            self.resize(self.fat.capacity * 2);
        }
    }
    fn pop(&mut self) {
        if self.size > 0 {
            self.fat
                .update([(self.front, Value::identity())].iter().cloned());
            self.size -= 1;
            self.front += 1;
            if self.size <= self.fat.capacity / 4 && self.size > 0 {
                self.resize(self.fat.capacity / 2);
            }
        }
    }
    fn query(&self) -> Value {
        if self.front > self.back {
            self.fat
                .suffix(self.front)
                .operate(&self.fat.prefix(self.back))
        } else {
            self.fat.aggregate()
        }
    }
    fn len(&self) -> usize {
        self.size
    }
    fn is_empty(&self) -> bool {
        self.size == 0
    }
}
