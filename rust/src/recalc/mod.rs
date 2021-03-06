use std::collections::VecDeque;

use alga::general::AbstractMagma;
use alga::general::Identity;

use crate::FifoWindow;
use crate::ops::AggregateOperator;
use crate::ops::AggregateMonoid;

#[derive(Clone)]
pub struct ReCalc<BinOp>
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone
{
    stack: VecDeque<BinOp::Partial>,
}

impl<BinOp> FifoWindow<BinOp> for ReCalc<BinOp>
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone
{
    fn new() -> Self {
        Self {
            stack: VecDeque::new(),
        }
    }
    fn name() -> &'static str {
        "recalc"
    }
    fn push(&mut self, val: BinOp::In) {
        self.stack.push_back(BinOp::lift(val));
    }
    fn pop(&mut self) {
        self.stack.pop_front();
    }
    fn query(&self) -> BinOp::Out {
        let agg = self.stack
                      .iter()
                      .fold(BinOp::Partial::identity(), |acc, elem| acc.operate(&elem));
        BinOp::lower(&agg)
    }
    fn len(&self) -> usize {
        self.stack.len()
    }
    fn is_empty(&self) -> bool {
        self.stack.is_empty()
    }
}

