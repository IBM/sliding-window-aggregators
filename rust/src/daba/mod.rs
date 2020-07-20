use crate::FifoWindow;
use alga::general::AbstractMonoid;
use alga::general::Operator;
use std::collections::VecDeque;
use std::marker::PhantomData;

#[derive(Clone)]
struct Item<Value> {
    val: Value,
    agg: Value,
}

impl<Value> Item<Value> {
    fn new(val: Value, agg: Value) -> Self {
        Self { val, agg }
    }
}

#[derive(Clone)]
pub struct DABA<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    // ith oldest value in FIFO order stored at vi = vals[i]
    items: VecDeque<Item<Value>>,
    // 0 ≤ l ≤ r ≤ a ≤ b ≤ items.len()
    l: usize, // Left,  ∀p ∈ l...r−1 : items[p].agg = items[p].val ⊕ ... ⊕ items[r−1].val
    r: usize, // Right, ∀p ∈ r...a−1 : items[p].agg = items[R].val ⊕ ... ⊕ items[p].val
    a: usize, // Accum, ∀p ∈ a...b−1 : items[p].agg = items[p].val ⊕ ... ⊕ items[b−1].val
    b: usize, // Back,  ∀p ∈ b...e−1 : items[p].agg = items[B].val ⊕ ... ⊕ items[p].val
    op: PhantomData<BinOp>,
}

impl<Value, BinOp> FifoWindow<Value, BinOp> for DABA<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    fn new() -> Self {
        Self {
            items: VecDeque::new(),
            l: 0,
            r: 0,
            a: 0,
            b: 0,
            op: PhantomData,
        }
    }
    fn push(&mut self, v: Value) {
        let agg = self.agg_b().operate(&v);
        self.items.push_back(Item::new(v, agg));
        self.fixup();
    }
    fn pop(&mut self) {
        if self.items.pop_front().is_some() {
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
        self.items.len()
    }
    fn is_empty(&self) -> bool {
        self.items.is_empty()
    }
}

impl<Value, BinOp> DABA<Value, BinOp>
where
    Value: AbstractMonoid<BinOp> + Clone,
    BinOp: Operator,
{
    #[inline(always)]
    fn agg_f(&self) -> Value {
        if self.items.is_empty() {
            Value::identity()
        } else {
            self.items.front().unwrap().agg.clone()
        }
    }
    #[inline(always)]
    fn agg_b(&self) -> Value {
        if self.b == self.items.len() {
            Value::identity()
        } else {
            self.items.back().unwrap().agg.clone()
        }
    }
    #[inline(always)]
    fn agg_l(&self) -> Value {
        if self.l == self.r {
            Value::identity()
        } else {
            self.items[self.l].agg.clone()
        }
    }
    #[inline(always)]
    fn agg_r(&self) -> Value {
        if self.r == self.a {
            Value::identity()
        } else {
            self.items[self.a - 1].agg.clone()
        }
    }
    #[inline(always)]
    fn agg_a(&self) -> Value {
        if self.a == self.b {
            Value::identity()
        } else {
            self.items[self.a].agg.clone()
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
        self.l = self.items.len();
        self.r = self.l;
        self.a = self.l;
        self.b = self.l;
    }
    #[inline(always)]
    fn flip(&mut self) {
        self.l = 0;
        self.a = self.items.len();
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
        self.items[self.l].agg = self.agg_l().operate(&self.agg_r()).operate(&self.agg_a());
        self.l += 1;
        self.items[self.a - 1].agg = self.items[self.a - 1].val.operate(&self.agg_a());
        self.a -= 1;
    }
}
