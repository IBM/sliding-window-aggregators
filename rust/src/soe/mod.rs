use std::collections::VecDeque;

use alga::general::AbstractMagma;
use alga::general::Identity;
use alga::general::TwoSidedInverse;

use crate::FifoWindow;
use crate::ops::AggregateOperator;
use crate::ops::AggregateGroup;

#[derive(Clone)]
pub struct SoE<BinOp>
where
    BinOp: AggregateGroup<BinOp> + AggregateOperator + Clone
{
    stack: VecDeque<BinOp::Partial>,
    agg: BinOp::Partial,
}

impl<BinOp> FifoWindow<BinOp> for SoE<BinOp>
where
    BinOp: AggregateGroup<BinOp> + AggregateOperator + Clone
{
    fn new() -> Self {
        Self {
            stack: VecDeque::new(),
            agg: BinOp::Partial::identity(),
        }
    }
    fn name() -> &'static str {
        "soe"
    }
    fn push(&mut self, val: BinOp::In) {
        let lifted = BinOp::lift(val);
        self.agg = self.agg.operate(&lifted);
        self.stack.push_back(lifted);
    }
    fn pop(&mut self) {
        if let Some(top) = self.stack.pop_front() {
            self.agg = self.agg.operate(&top.two_sided_inverse());
        }
    }
    fn query(&self) -> BinOp::Out {
        BinOp::lower(&self.agg.clone())
    }
    fn len(&self) -> usize {
        self.stack.len()
    }
    fn is_empty(&self) -> bool {
        self.stack.is_empty()
    }
}
