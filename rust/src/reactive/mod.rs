pub(crate) mod flat_fat;

use alga::general::AbstractMagma;
use alga::general::Identity;

use crate::FifoWindow;
use crate::ops::AggregateOperator;
use crate::ops::AggregateMonoid;
use crate::reactive::flat_fat::{FlatFAT, FAT};

#[derive(Clone)]
pub struct Reactive<BinOp>
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone
{
    fat: FlatFAT<BinOp>,
    size: usize,
    front: usize,
    back: usize,
}

impl<BinOp> Reactive<BinOp>
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone
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
        self.front >= self.back
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

impl<BinOp> FifoWindow<BinOp> for Reactive<BinOp>
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone
{
    fn new() -> Self {
        Self {
            fat: FlatFAT::with_capacity(2),
            size: 0,
            front: 0,
            back: 0,
        }
    }
    fn name() -> &'static str {
        "reactive"
    }
    fn push(&mut self, val: BinOp::In) {
        self.fat.update([(self.back, BinOp::lift(val))].iter().cloned());
        self.size += 1;
        self.back = (self.back + 1) % self.fat.capacity;
        if self.size > (3 * self.fat.capacity) / 4 {
            self.resize(self.fat.capacity * 2);
        }
    }
    fn pop(&mut self) {
        if self.size > 0 {
            self.fat
                .update([(self.front, BinOp::Partial::identity())].iter().cloned());
            self.size -= 1;
            self.front = (self.front + 1) % self.fat.capacity;
            if self.size <= self.fat.capacity / 4 && self.size > 0 {
                self.resize(self.fat.capacity / 2);
            }
        }
    }
    fn query(&self) -> BinOp::Out {
        if self.front > self.back {
            BinOp::lower(&self.fat
                              .suffix(self.front)
                              .operate(&self.fat.prefix(self.back)))
        } else {
            BinOp::lower(&self.fat.aggregate())
        }
    }
    fn len(&self) -> usize {
        self.size
    }
    fn is_empty(&self) -> bool {
        self.size == 0
    }
}
