use crate::FifoWindow;
use alga::general::AbstractMonoid;
use alga::general::Operator;
use std::collections::VecDeque;
use std::marker::PhantomData;
use std::fmt;

#[derive(Clone)]
pub struct ReCalc<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    stack: VecDeque<Value>,
    op: PhantomData<BinOp>,
}

impl<Value, BinOp> fmt::Display for ReCalc<Value, BinOp> 
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", ReCalc::<Value, BinOp>::name())
    }
}

impl<Value, BinOp> FifoWindow<Value, BinOp> for ReCalc<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    fn new() -> Self {
        Self {
            stack: VecDeque::new(),
            op: PhantomData,
        }
    }
    fn name() -> &'static str {
        "recalc"
    }
    fn push(&mut self, v: Value) {
        self.stack.push_back(v);
    }
    fn pop(&mut self) -> Option<Value> {
        self.stack.pop_front()
    }
    fn query(&self) -> Value {
        self.stack
            .iter()
            .fold(Value::identity(), |acc, elem| acc.operate(&elem))
    }
    fn len(&self) -> usize {
        self.stack.len()
    }
    fn is_empty(&self) -> bool {
        self.stack.is_empty()
    }
}
