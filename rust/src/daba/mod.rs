use crate::FifoWindow;
use alga::general::AbstractMonoid;
use alga::general::Operator;
use std::collections::VecDeque;
use std::marker::PhantomData;

#[derive(Clone)]
pub struct DABA<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    // ith oldest value in FIFO order stored at vi = vals[i]
    vals: VecDeque<Value>,
    aggs: VecDeque<Value>,
    // 0 ≤ l ≤ r ≤ a ≤ b ≤ aggs.len()
    l: usize, // Left,  ∀p ∈ l...r−1 : aggs[p] = vals[p] ⊕ ... ⊕ vals[r−1]
    r: usize, // Right, ∀p ∈ r...a−1 : aggs[p] = vals[R] ⊕ ... ⊕ vals[p]
    a: usize, // Accum, ∀p ∈ a...b−1 : aggs[p] = vals[p] ⊕ ... ⊕ vals[b−1]
    b: usize, // Back,  ∀p ∈ b...e−1 : aggs[p] = vals[B] ⊕ ... ⊕ vals[p]
    op: PhantomData<BinOp>,
}

impl<Value, BinOp> FifoWindow<Value, BinOp> for DABA<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    fn new() -> Self {
        Self {
            vals: VecDeque::new(),
            aggs: VecDeque::new(),
            l: 0,
            r: 0,
            a: 0,
            b: 0,
            op: PhantomData,
        }
    }
    fn push(&mut self, v: Value) {
        self.aggs.push_back(self.agg_b().operate(&v));
        self.vals.push_back(v);
        self.fixup();
    }
    fn pop(&mut self) {
        if self.vals.pop_front().is_some() {
            self.aggs.pop_front();
            self.l -= 1;
            self.r -= 1;
            self.a -= 1;
            self.b -= 1;
            self.fixup();
        }
    }
    fn query(&self) -> Value {
        self.agg_f().operate(&self.agg_b())
    }
    fn len(&self) -> usize {
        self.vals.len()
    }
    fn is_empty(&self) -> bool {
        self.vals.is_empty()
    }
}

impl<Value, BinOp> DABA<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    #[inline(always)]
    fn agg_f(&self) -> Value {
        if self.aggs.is_empty() {
            Value::identity()
        } else {
            self.aggs.front().unwrap().clone()
        }
    }
    #[inline(always)]
    fn agg_b(&self) -> Value {
        if self.b == self.aggs.len() {
            Value::identity()
        } else {
            self.aggs.back().unwrap().clone()
        }
    }
    #[inline(always)]
    fn agg_l(&self) -> Value {
        if self.l == self.r {
            Value::identity()
        } else {
            self.aggs[self.l].clone()
        }
    }
    #[inline(always)]
    fn agg_r(&self) -> Value {
        if self.r == self.a {
            Value::identity()
        } else {
            self.aggs[self.a - 1].clone()
        }
    }
    #[inline(always)]
    fn agg_a(&self) -> Value {
        if self.a == self.b {
            Value::identity()
        } else {
            self.aggs[self.a].clone()
        }
    }
    fn fixup(&mut self) {
        if self.b == 0 {
            self.singleton()
        } else {
            if self.l == self.b {
                self.flip()
            }
            if self.l == self.r {
                self.shift()
            } else {
                self.shrink()
            }
        }
    }
    #[inline(always)]
    fn singleton(&mut self) {
        self.l = self.aggs.len();
        self.r = self.l;
        self.a = self.l;
        self.b = self.l;
    }
    #[inline(always)]
    fn flip(&mut self) {
        self.l = 0;
        self.a = self.aggs.len();
        self.b = self.a;
    }
    #[inline(always)]
    fn shift(&mut self) {
        self.a += 1;
        self.r += 1;
        self.l += 1;
    }
    #[inline(always)]
    fn shrink(&mut self) {
        self.aggs[self.l] = self.agg_l().operate(&self.agg_r()).operate(&self.agg_a());
        self.l += 1;
        self.aggs[self.a - 1] = self.vals[self.a - 1].operate(&self.agg_a());
        self.a -= 1;
    }
}
