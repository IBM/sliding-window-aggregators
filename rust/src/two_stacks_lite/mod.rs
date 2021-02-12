use std::collections::VecDeque;

use alga::general::AbstractMagma;
use alga::general::Identity;

use crate::FifoWindow;
use crate::ops::AggregateOperator;
use crate::ops::AggregateMonoid;

#[derive(Clone)]
pub struct TwoStacksLite<BinOp>
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone
{
    queue: VecDeque<BinOp::Partial>,
    agg_back: BinOp::Partial,
    front_len: usize,
}

impl<BinOp> FifoWindow<BinOp> for TwoStacksLite<BinOp>
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone
{
    fn new() -> Self {
        Self {
            queue: VecDeque::new(),
            agg_back: BinOp::Partial::identity(),
            front_len: 0,
        }
    }
    fn name() -> &'static str {
        "two_stacks_lite"
    }
    fn push(&mut self, v: BinOp::In) {
        let lifted = BinOp::lift(v);
        self.queue.push_back(lifted.clone());
        self.agg_back = self.agg_back.operate(&lifted);
    }
    fn pop(&mut self) -> Option<BinOp::Out> {
        if self.queue.is_empty() {
            None
        } else {
            let end = self.queue.len() - 1;
            if self.front_len == 0 {
                let mut i = end;
                while i != 0 {
                   i -= 1;
                   self.queue[i] = self.queue[i].operate(&self.queue[i+1]);
                }
                self.front_len = end + 1;
                self.agg_back = BinOp::Partial::identity();
            }
            self.front_len -= 1;
            self.queue.pop_front().map(|item| BinOp::lower(&item))
        }
    }
    fn query(&self) -> BinOp::Out {
        let f = &self.agg_front();
        let b = &self.agg_back;
        BinOp::lower(&f.operate(&b))
    }
    fn len(&self) -> usize {
        self.queue.len()
    }
    fn is_empty(&self) -> bool {
        self.queue.is_empty()
    }
}

impl<BinOp> TwoStacksLite<BinOp>
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone
{
    #[inline(always)]
    fn agg_front(&self) -> BinOp::Partial {
        if self.front_len == 0 || self.queue.is_empty() {
            BinOp::Partial::identity()
        } else {
            self.queue.front().unwrap().clone()
        }
    }
}
