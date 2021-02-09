use alga::general::AbstractMagma;
use alga::general::Identity;

use crate::FifoWindow;
use crate::ops::AggregateOperator;
use crate::ops::AggregateMonoid;

#[derive(Clone)]
struct Item<Value: Clone>
where
    Value: Clone,
{
    agg: Value,
    val: Value,
}

#[derive(Clone)]
pub struct TwoStacks<BinOp>
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone
{
    front: Vec<Item<BinOp::Partial>>,
    back: Vec<Item<BinOp::Partial>>,
}

impl<BinOp> FifoWindow<BinOp> for TwoStacks<BinOp>
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone
{
    fn new() -> Self {
        Self {
            front: Vec::new(),
            back: Vec::new(),
        }
    }
    fn name() -> &'static str {
        "two_stacks"
    }
    fn push(&mut self, v: BinOp::In) {
        let lifted = BinOp::lift(v);
        self.back.push(Item {
            agg: Self::agg(&self.back).operate(&lifted),
            val: lifted,
        });
    }
    fn pop(&mut self) -> Option<BinOp::Out> {
        if self.front.is_empty() {
            while let Some(top) = self.back.pop() {
                self.front.push(Item {
                    agg: top.val.operate(&Self::agg(&self.front)),
                    val: top.val,
                })
            }
        }
        self.front.pop().map(|item| BinOp::lower(&item.val))
    }
    fn query(&self) -> BinOp::Out {
        let f = Self::agg(&self.front);
        let b = Self::agg(&self.back);
        BinOp::lower(&f.operate(&b))
    }
    fn len(&self) -> usize {
        self.front.len() + self.back.len()
    }
    fn is_empty(&self) -> bool {
        self.front.is_empty() && self.back.is_empty()
    }
}

impl<BinOp> TwoStacks<BinOp>
where
    BinOp: AggregateMonoid<BinOp> + AggregateOperator + Clone
{
    #[inline(always)]
    fn agg(stack: &[Item<BinOp::Partial>]) -> BinOp::Partial {
        if let Some(top) = stack.last() {
            top.agg.clone()
        } else {
            BinOp::Partial::identity()
        }
    }
}
