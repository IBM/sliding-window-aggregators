use crate::FifoWindow;
use alga::general::AbstractMonoid;
use alga::general::Operator;
use std::marker::PhantomData;

#[derive(Debug)]
struct Item<Value>
where
    Value: Clone,
{
    agg: Value,
    val: Value,
}

#[derive(Debug)]
pub struct TwoStacks<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    front: Vec<Item<Value>>,
    back: Vec<Item<Value>>,
    op: PhantomData<BinOp>,
}

impl<Value, BinOp> FifoWindow<Value, BinOp> for TwoStacks<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    fn new() -> TwoStacks<Value, BinOp> {
        TwoStacks {
            front: Vec::new(),
            back: Vec::new(),
            op: PhantomData,
        }
    }
    fn push(&mut self, v: Value) {
        self.back.push(Item {
            agg: Self::agg(&self.back).operate(&v),
            val: v,
        });
    }
    fn pop(&mut self) {
        if self.front.is_empty() {
            while let Some(top) = self.back.pop() {
                self.front.push(Item {
                    agg: top.val.operate(&Self::agg(&self.front)),
                    val: top.val,
                })
            }
        }
        self.front.pop();
    }
    fn query(&self) -> Value {
        Self::agg(&self.front).operate(&Self::agg(&self.back))
    }
}

impl<Value, BinOp> TwoStacks<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    #[inline(always)]
    fn agg(stack: &Vec<Item<Value>>) -> Value {
        if let Some(top) = stack.last() {
            top.agg.clone()
        } else {
            Value::identity()
        }
    }
}
