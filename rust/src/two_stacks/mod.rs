use crate::FifoWindow;
use alga::general::AbstractMonoid;
use alga::general::Operator;
use std::marker::PhantomData;
use std::fmt;

#[derive(Clone)]
struct Item<Value>
where
    Value: Clone,
{
    agg: Value,
    val: Value,
}

#[derive(Clone)]
pub struct TwoStacks<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    front: Vec<Item<Value>>,
    back: Vec<Item<Value>>,
    op: PhantomData<BinOp>,
}

impl<Value, BinOp> fmt::Display for TwoStacks<Value, BinOp> 
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", TwoStacks::<Value, BinOp>::name())
    }
}

impl<Value, BinOp> FifoWindow<Value, BinOp> for TwoStacks<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    fn new() -> Self {
        Self {
            front: Vec::new(),
            back: Vec::new(),
            op: PhantomData,
        }
    }
    fn name() -> &'static str {
        "two_stacks"
    }
    fn push(&mut self, v: Value) {
        self.back.push(Item {
            agg: Self::agg(&self.back).operate(&v),
            val: v,
        });
    }
    fn pop(&mut self) -> Option<Value> {
        if self.front.is_empty() {
            while let Some(top) = self.back.pop() {
                self.front.push(Item {
                    agg: top.val.operate(&Self::agg(&self.front)),
                    val: top.val,
                })
            }
        }
        self.front.pop().map(|item| item.val)
    }
    fn query(&self) -> Value {
        Self::agg(&self.front).operate(&Self::agg(&self.back))
    }
    fn len(&self) -> usize {
        self.front.len() + self.back.len()
    }
    fn is_empty(&self) -> bool {
        self.front.is_empty() && self.back.is_empty()
    }
}

impl<Value, BinOp> TwoStacks<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    #[inline(always)]
    fn agg(stack: &[Item<Value>]) -> Value {
        if let Some(top) = stack.last() {
            top.agg.clone()
        } else {
            Value::identity()
        }
    }
}
